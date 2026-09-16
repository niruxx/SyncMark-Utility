#include "ContactEditDialog.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QJsonArray>

namespace {

QPlainTextEdit *makeEntryEditor(QWidget *parent, const QString &placeholder) {
    auto *edit = new QPlainTextEdit(parent);
    edit->setPlaceholderText(placeholder);
    edit->setMaximumHeight(70);
    return edit;
}

QJsonArray parseTypeValueLines(const QString &text) {
    QJsonArray arr;
    for (const QString &lineRaw : text.split('\n', Qt::SkipEmptyParts)) {
        const QString line = lineRaw.trimmed();
        if (line.isEmpty()) continue;
        const int sep = line.indexOf(':');
        QString type = sep >= 0 ? line.left(sep).trimmed() : QStringLiteral("other");
        QString value = sep >= 0 ? line.mid(sep + 1).trimmed() : line;
        if (value.isEmpty()) continue;
        arr.append(QJsonObject{{"type", type}, {"value", value}});
    }
    return arr;
}

QString formatTypeValueLines(const QJsonArray &arr) {
    QStringList lines;
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        const QString type = o.value("type").toString();
        const QString value = o.value("value").toString();
        lines << (type.isEmpty() ? value : QStringLiteral("%1: %2").arg(type, value));
    }
    return lines.join('\n');
}

} // namespace

ContactEditDialog::ContactEditDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Contact"));
    setMinimumWidth(440);

    m_firstName = new QLineEdit(this);
    m_lastName = new QLineEdit(this);
    m_organization = new QLineEdit(this);
    m_title = new QLineEdit(this);
    m_emails = makeEntryEditor(this, tr("home: jane@example.com"));
    m_phones = makeEntryEditor(this, tr("mobile: +1 555 0100"));
    m_tags = new QLineEdit(this);
    m_tags->setPlaceholderText(tr("comma, separated, tags"));
    m_notes = makeEntryEditor(this, {});
    m_favorite = new QCheckBox(tr("Favorite"), this);

    m_error = new QLabel(this);
    m_error->setStyleSheet("color: #b33;");
    m_error->setWordWrap(true);

    auto *form = new QFormLayout;
    form->addRow(tr("First name:"), m_firstName);
    form->addRow(tr("Last name:"), m_lastName);
    form->addRow(tr("Organization:"), m_organization);
    form->addRow(tr("Title:"), m_title);
    form->addRow(tr("Emails (one per line):"), m_emails);
    form->addRow(tr("Phones (one per line):"), m_phones);
    form->addRow(tr("Tags:"), m_tags);
    form->addRow(tr("Notes:"), m_notes);
    form->addRow(QString(), m_favorite);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ContactEditDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_error);
    layout->addWidget(buttons);
}

void ContactEditDialog::setContact(const QJsonObject &contact) {
    m_original = contact;
    m_firstName->setText(contact.value("first_name").toString());
    m_lastName->setText(contact.value("last_name").toString());
    m_organization->setText(contact.value("organization").toString());
    m_title->setText(contact.value("title").toString());
    m_emails->setPlainText(formatTypeValueLines(contact.value("emails").toArray()));
    m_phones->setPlainText(formatTypeValueLines(contact.value("phones").toArray()));
    QJsonArray tags = contact.value("tags").toArray();
    QStringList tagList;
    for (const QJsonValue &t : tags) tagList << t.toString();
    m_tags->setText(tagList.join(", "));
    m_notes->setPlainText(contact.value("notes").toString());
    m_favorite->setChecked(contact.value("favorite").toInt() != 0 || contact.value("favorite").toBool());
}

QJsonObject ContactEditDialog::formData() const {
    QJsonObject data = m_original; // preserves addresses/socialProfiles/messagingHandles/customFields/keyDates/relationships
    data["firstName"] = m_firstName->text().trimmed();
    data["lastName"] = m_lastName->text().trimmed();
    data["fullName"] = (m_firstName->text().trimmed() + " " + m_lastName->text().trimmed()).trimmed();
    data["organization"] = m_organization->text().trimmed();
    data["title"] = m_title->text().trimmed();
    data["emails"] = parseTypeValueLines(m_emails->toPlainText());
    data["phones"] = parseTypeValueLines(m_phones->toPlainText());

    QJsonArray tags;
    for (const QString &t : m_tags->text().split(',', Qt::SkipEmptyParts))
        tags.append(t.trimmed());
    data["tags"] = tags;
    data["notes"] = m_notes->toPlainText();
    data["favorite"] = m_favorite->isChecked();

    if (!data.contains("addresses")) data["addresses"] = QJsonArray();
    if (!data.contains("socialProfiles")) data["socialProfiles"] = QJsonArray();
    if (!data.contains("messagingHandles")) data["messagingHandles"] = QJsonArray();
    if (!data.contains("customFields")) data["customFields"] = QJsonArray();
    if (!data.contains("keyDates")) data["keyDates"] = QJsonArray();
    if (!data.contains("relationships")) data["relationships"] = QJsonArray();
    return data;
}

void ContactEditDialog::validateAndAccept() {
    if (m_firstName->text().trimmed().isEmpty() && m_lastName->text().trimmed().isEmpty()) {
        m_error->setText(tr("Enter at least a first or last name."));
        return;
    }
    accept();
}
