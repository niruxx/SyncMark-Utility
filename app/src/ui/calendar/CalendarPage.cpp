#include "CalendarPage.h"
#include "EventEditDialog.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"

#include <QCalendarWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrlQuery>
#include <QJsonArray>
#include <QTimer>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QTextCharFormat>
#include <QBrush>
#include <QColor>
#include <algorithm>

CalendarPage::CalendarPage(ApiClient *api, QWidget *parent) : PageWidget(parent), m_api(api) {
    m_calendar = new QCalendarWidget(this);
    connect(m_calendar, &QCalendarWidget::selectionChanged, this, &CalendarPage::applyDateFilter);
    connect(m_calendar, &QCalendarWidget::currentPageChanged, this, [this](int, int) { highlightEventDates(); });

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search events..."));
    auto *searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(300);
    connect(m_search, &QLineEdit::textChanged, searchTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(searchTimer, &QTimer::timeout, this, &CalendarPage::reloadEvents);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("Title"), tr("Start"), tr("End"), tr("Location")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(30);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &CalendarPage::onContextMenu);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) { editEventAt(row); });

    auto *addBtn = new QPushButton(tr("New Event"), this);
    connect(addBtn, &QPushButton::clicked, this, &CalendarPage::addEvent);
    auto *importBtn = new QPushButton(tr("Import ICS..."), this);
    connect(importBtn, &QPushButton::clicked, this, &CalendarPage::importEvents);
    auto *exportBtn = new QPushButton(tr("Export ICS..."), this);
    connect(exportBtn, &QPushButton::clicked, this, &CalendarPage::exportEvents);

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(addBtn);
    toolbar->addStretch();
    toolbar->addWidget(importBtn);
    toolbar->addWidget(exportBtn);

    auto *rightLayout = new QVBoxLayout;
    rightLayout->addWidget(m_search);
    rightLayout->addWidget(m_table);
    auto *rightWidget = new QWidget(this);
    rightWidget->setLayout(rightLayout);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(m_calendar);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(splitter);
}

void CalendarPage::reload() { reloadEvents(); }

void CalendarPage::reloadEvents() {
    QUrlQuery query;
    if (!m_search->text().trimmed().isEmpty())
        query.addQueryItem("q", m_search->text().trimmed());

    m_api->get("/events", query, [this](const ApiResult &result) {
        if (!result.ok || !result.body.isArray()) {
            if (!result.ok) Notify::error(this, tr("Failed to load events: %1").arg(result.error));
            return;
        }
        m_allEvents.clear();
        for (const QJsonValue &v : result.body.array()) m_allEvents << v.toObject();
        highlightEventDates();
        populateTableForDate(m_calendar->selectedDate());
    });
}

bool CalendarPage::occurrenceOnDate(const QJsonObject &event, const QDate &date, QDateTime &start, QDateTime &end) {
    const QDateTime baseStart = QDateTime::fromString(event.value("startAt").toString(), Qt::ISODate).toLocalTime();
    QDateTime baseEnd = QDateTime::fromString(event.value("endAt").toString(), Qt::ISODate).toLocalTime();
    if (!baseStart.isValid()) return false;
    if (!baseEnd.isValid()) baseEnd = baseStart;
    const qint64 durationSecs = baseStart.secsTo(baseEnd);

    const QString rrule = event.value("recurrence").toString();
    if (rrule.isEmpty()) {
        if (date < baseStart.date() || date > baseEnd.date()) return false;
        start = baseStart;
        end = baseEnd;
        return true;
    }

    if (date < baseStart.date()) return false;

    QString freq;
    QDate until;
    for (const QString &part : rrule.split(';')) {
        if (part.startsWith("FREQ=")) freq = part.mid(5);
        else if (part.startsWith("UNTIL=")) until = QDate::fromString(part.mid(6).left(8), "yyyyMMdd");
    }
    if (until.isValid() && date > until) return false;

    bool aligned = false;
    if (freq == "DAILY") aligned = true;
    else if (freq == "WEEKLY") aligned = (baseStart.date().daysTo(date) % 7 == 0);
    else if (freq == "MONTHLY") aligned = (date.day() == baseStart.date().day());
    else aligned = (date == baseStart.date());

    if (!aligned) return false;
    start = QDateTime(date, baseStart.time());
    end = start.addSecs(durationSecs);
    return true;
}

void CalendarPage::highlightEventDates() {
    m_calendar->setDateTextFormat(QDate(), QTextCharFormat());
    QTextCharFormat fmt;
    fmt.setFontWeight(QFont::Bold);
    fmt.setForeground(QBrush(QColor("#3a6ea5")));

    const int year = m_calendar->yearShown();
    const int month = m_calendar->monthShown();
    const QDate firstOfMonth(year, month, 1);
    const int daysInMonth = firstOfMonth.daysInMonth();

    for (int day = 1; day <= daysInMonth; ++day) {
        const QDate date(year, month, day);
        QDateTime s, e;
        for (const QJsonObject &ev : m_allEvents) {
            if (occurrenceOnDate(ev, date, s, e)) {
                m_calendar->setDateTextFormat(date, fmt);
                break;
            }
        }
    }
}

void CalendarPage::applyDateFilter() {
    populateTableForDate(m_calendar->selectedDate());
}

void CalendarPage::populateTableForDate(const QDate &date) {
    QVector<Occurrence> dayEvents;
    for (const QJsonObject &ev : m_allEvents) {
        QDateTime s, e;
        if (occurrenceOnDate(ev, date, s, e))
            dayEvents << Occurrence{ev, s, e};
    }
    std::sort(dayEvents.begin(), dayEvents.end(), [](const Occurrence &a, const Occurrence &b) {
        return a.start < b.start;
    });

    m_table->setRowCount(dayEvents.size());
    for (int row = 0; row < dayEvents.size(); ++row) {
        const Occurrence &occ = dayEvents[row];
        const bool isRecurring = !occ.event.value("recurrence").toString().isEmpty();
        auto *item = new QTableWidgetItem((isRecurring ? QStringLiteral("↻ ") : QString()) + occ.event.value("title").toString());
        item->setData(Qt::UserRole, occ.event.value("id").toVariant());
        m_table->setItem(row, 0, item);
        const bool allDay = occ.event.value("allDay").toInt() != 0 || occ.event.value("allDay").toBool();
        const QString fmt = allDay ? "yyyy-MM-dd" : "yyyy-MM-dd HH:mm";
        m_table->setItem(row, 1, new QTableWidgetItem(occ.start.toString(fmt)));
        m_table->setItem(row, 2, new QTableWidgetItem(occ.end.toString(fmt)));
        m_table->setItem(row, 3, new QTableWidgetItem(occ.event.value("location").toString()));
    }
}

int CalendarPage::selectedRow() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

void CalendarPage::onContextMenu(const QPoint &pos) {
    if (m_table->itemAt(pos)) m_table->selectRow(m_table->itemAt(pos)->row());
    if (selectedRow() < 0) return;
    QMenu menu(this);
    QAction *editAction = menu.addAction(tr("Edit..."));
    QAction *deleteAction = menu.addAction(tr("Delete"));
    QAction *chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (chosen == editAction) editSelectedEvent();
    else if (chosen == deleteAction) deleteSelectedEvent();
}

void CalendarPage::addEvent() {
    EventEditDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    m_api->postJson("/events", dialog.formData(), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not create event: %1").arg(result.error)); return; }
        reloadEvents();
    });
}

void CalendarPage::editEventAt(int row) {
    if (row < 0 || row >= m_table->rowCount()) return;
    const qlonglong id = m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
    const auto it = std::find_if(m_allEvents.begin(), m_allEvents.end(), [id](const QJsonObject &e) {
        return e.value("id").toVariant().toLongLong() == id;
    });
    if (it == m_allEvents.end()) return;

    EventEditDialog dialog(this);
    dialog.setEvent(*it);
    if (dialog.exec() != QDialog::Accepted) return;
    m_api->putJson(QStringLiteral("/events/%1").arg(id), dialog.formData(), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not update event: %1").arg(result.error)); return; }
        reloadEvents();
    });
}

void CalendarPage::editSelectedEvent() { editEventAt(selectedRow()); }

void CalendarPage::deleteSelectedEvent() {
    int row = selectedRow();
    if (row < 0) return;
    if (!Notify::confirm(this, tr("Delete Event"), tr("Delete this event?"))) return;
    const qlonglong id = m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
    m_api->del(QStringLiteral("/events/%1").arg(id), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not delete event: %1").arg(result.error)); return; }
        reloadEvents();
    });
}

void CalendarPage::importEvents() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Import Calendar"), {}, tr("iCalendar (*.ics)"));
    if (path.isEmpty()) return;
    auto *file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) { Notify::error(this, tr("Could not open file.")); delete file; return; }

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QVariant(QStringLiteral("form-data; name=\"file\"; filename=\"%1\"").arg(QFileInfo(path).fileName())));
    file->setParent(multiPart);
    part.setBodyDevice(file);
    multiPart->append(part);

    m_api->postMultipart("/events/import", multiPart, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Import failed: %1").arg(result.error)); return; }
        reloadEvents();
    });
}

void CalendarPage::exportEvents() {
    m_api->getRaw("/events/export", {}, [this](const ApiRawResult &result) {
        if (!result.ok) { Notify::error(this, tr("Export failed: %1").arg(result.error)); return; }
        const QString path = QFileDialog::getSaveFileName(this, tr("Save Calendar"), "syncmark-calendar.ics");
        if (path.isEmpty()) return;
        QFile out(path);
        if (!out.open(QIODevice::WriteOnly)) { Notify::error(this, tr("Could not write file.")); return; }
        out.write(result.data);
    });
}
