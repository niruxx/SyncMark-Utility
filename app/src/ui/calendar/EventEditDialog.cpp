#include "EventEditDialog.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QDateTimeEdit>
#include <QDateEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>

EventEditDialog::EventEditDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Event"));
    setMinimumWidth(420);

    m_title = new QLineEdit(this);
    m_location = new QLineEdit(this);
    m_description = new QPlainTextEdit(this);
    m_description->setMaximumHeight(80);

    m_start = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_start->setCalendarPopup(true);
    m_start->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_end = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(3600), this);
    m_end->setCalendarPopup(true);
    m_end->setDisplayFormat("yyyy-MM-dd HH:mm");

    m_allDay = new QCheckBox(tr("All day"), this);
    connect(m_allDay, &QCheckBox::toggled, this, [this](bool on) {
        m_start->setDisplayFormat(on ? "yyyy-MM-dd" : "yyyy-MM-dd HH:mm");
        m_end->setDisplayFormat(on ? "yyyy-MM-dd" : "yyyy-MM-dd HH:mm");
    });

    m_recurrence = new QComboBox(this);
    m_recurrence->addItem(tr("Does not repeat"), "");
    m_recurrence->addItem(tr("Daily"), "DAILY");
    m_recurrence->addItem(tr("Weekly"), "WEEKLY");
    m_recurrence->addItem(tr("Monthly"), "MONTHLY");
    connect(m_recurrence, &QComboBox::currentIndexChanged, this, &EventEditDialog::updateRecurrenceState);

    m_until = new QDateEdit(QDate::currentDate().addMonths(1), this);
    m_until->setCalendarPopup(true);

    auto *recurrenceRow = new QHBoxLayout;
    recurrenceRow->addWidget(m_recurrence);
    recurrenceRow->addWidget(new QLabel(tr("until")));
    recurrenceRow->addWidget(m_until);

    m_error = new QLabel(this);
    m_error->setStyleSheet("color: #b33;");
    m_error->setWordWrap(true);

    auto *form = new QFormLayout;
    form->addRow(tr("Title:"), m_title);
    form->addRow(tr("Location:"), m_location);
    form->addRow(tr("Description:"), m_description);
    form->addRow(tr("Starts:"), m_start);
    form->addRow(tr("Ends:"), m_end);
    form->addRow(QString(), m_allDay);
    form->addRow(tr("Repeats:"), recurrenceRow);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &EventEditDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_error);
    layout->addWidget(buttons);

    updateRecurrenceState();
}

void EventEditDialog::updateRecurrenceState() {
    m_until->setEnabled(!m_recurrence->currentData().toString().isEmpty());
}

void EventEditDialog::setEvent(const QJsonObject &event) {
    m_title->setText(event.value("title").toString());
    m_location->setText(event.value("location").toString());
    m_description->setPlainText(event.value("description").toString());

    QDateTime start = QDateTime::fromString(event.value("startAt").toString(), Qt::ISODate);
    QDateTime end = QDateTime::fromString(event.value("endAt").toString(), Qt::ISODate);
    if (start.isValid()) m_start->setDateTime(start.toLocalTime());
    if (end.isValid()) m_end->setDateTime(end.toLocalTime());

    const bool allDay = event.value("allDay").toInt() != 0 || event.value("allDay").toBool();
    m_allDay->setChecked(allDay);

    const QString rrule = event.value("recurrence").toString();
    if (!rrule.isEmpty()) {
        const auto parts = rrule.split(';');
        for (const QString &part : parts) {
            if (part.startsWith("FREQ=")) {
                const QString freq = part.mid(5);
                int idx = m_recurrence->findData(freq);
                if (idx >= 0) m_recurrence->setCurrentIndex(idx);
            } else if (part.startsWith("UNTIL=")) {
                QString untilStr = part.mid(6);
                QDate d = QDate::fromString(untilStr.left(8), "yyyyMMdd");
                if (d.isValid()) m_until->setDate(d);
            }
        }
    } else {
        m_recurrence->setCurrentIndex(0);
    }
    updateRecurrenceState();
}

QJsonObject EventEditDialog::formData() const {
    QJsonObject data{
        {"title", m_title->text().trimmed()},
        {"location", m_location->text().trimmed()},
        {"description", m_description->toPlainText()},
        {"startAt", m_start->dateTime().toUTC().toString(Qt::ISODate)},
        {"endAt", m_end->dateTime().toUTC().toString(Qt::ISODate)},
        {"allDay", m_allDay->isChecked()},
    };
    const QString freq = m_recurrence->currentData().toString();
    if (freq.isEmpty()) {
        data["recurrence"] = QJsonValue::Null;
    } else {
        data["recurrence"] = QJsonObject{
            {"freq", freq},
            {"until", m_until->date().toString("yyyy-MM-dd")},
        };
    }
    return data;
}

void EventEditDialog::validateAndAccept() {
    if (m_title->text().trimmed().isEmpty()) {
        m_error->setText(tr("Title is required."));
        return;
    }
    if (m_end->dateTime() < m_start->dateTime()) {
        m_error->setText(tr("End time must be after start time."));
        return;
    }
    accept();
}
