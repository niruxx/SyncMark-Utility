#include "ContactEditDialog.h"
#include "../common/RepeatingTableEditor.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QRegularExpression>

namespace {
QString contactChoiceLabel(const QJsonObject &c) {
    QString name = c.value("full_name").toString();
    if (name.trimmed().isEmpty())
        name = (c.value("first_name").toString() + " " + c.value("last_name").toString()).trimmed();
    if (name.isEmpty()) name = QObject::tr("(unnamed)");
    return QStringLiteral("%1 (#%2)").arg(name).arg(c.value("id").toVariant().toLongLong());
}

qlonglong parseContactIdFromChoice(const QString &choice) {
    static const QRegularExpression re("\\(#(\\d+)\\)\\s*$");
    auto m = re.match(choice);
    return m.hasMatch() ? m.captured(1).toLongLong() : 0;
}
}

ContactEditDialog::ContactEditDialog(const QVector<QJsonObject> &allContacts, QWidget *parent)
    : QDialog(parent), m_allContacts(allContacts) {
    setWindowTitle(tr("Contact"));
    setMinimumSize(560, 520);

    m_firstName = new QLineEdit(this);
    m_lastName = new QLineEdit(this);
    m_organization = new QLineEdit(this);
    m_title = new QLineEdit(this);
    m_tags = new QLineEdit(this);
    m_tags->setPlaceholderText(tr("comma, separated, tags"));
    m_notes = new QPlainTextEdit(this);
    m_notes->setMaximumHeight(90);
    m_favorite = new QCheckBox(tr("Favorite"), this);

    auto *generalForm = new QFormLayout;
    generalForm->addRow(tr("First name:"), m_firstName);
    generalForm->addRow(tr("Last name:"), m_lastName);
    generalForm->addRow(tr("Organization:"), m_organization);
    generalForm->addRow(tr("Title:"), m_title);
    generalForm->addRow(tr("Tags:"), m_tags);
    generalForm->addRow(tr("Notes:"), m_notes);
    generalForm->addRow(QString(), m_favorite);
    auto *generalTab = new QWidget(this);
    generalTab->setLayout(generalForm);

    const QStringList contactTypeChoices = {"home", "work", "mobile", "other"};

    m_emails = new RepeatingTableEditor({tr("Type"), tr("Email")}, this);
    m_emails->setColumnChoices(0, contactTypeChoices);
    m_emails->setColumnPlaceholder(1, "jane@example.com");

    m_phones = new RepeatingTableEditor({tr("Type"), tr("Phone")}, this);
    m_phones->setColumnChoices(0, contactTypeChoices);
    m_phones->setColumnPlaceholder(1, "+1 555 0100");

    auto *contactInfoLayout = new QVBoxLayout;
    contactInfoLayout->addWidget(new QLabel(tr("Emails")));
    contactInfoLayout->addWidget(m_emails);
    contactInfoLayout->addWidget(new QLabel(tr("Phones")));
    contactInfoLayout->addWidget(m_phones);
    auto *contactInfoTab = new QWidget(this);
    contactInfoTab->setLayout(contactInfoLayout);

    m_addresses = new RepeatingTableEditor(
        {tr("Type"), tr("Street"), tr("City"), tr("State"), tr("Postal Code"), tr("Country")}, this);
    m_addresses->setColumnChoices(0, {"home", "work", "other"});
    auto *addressesLayout = new QVBoxLayout;
    addressesLayout->addWidget(m_addresses);
    auto *addressesTab = new QWidget(this);
    addressesTab->setLayout(addressesLayout);

    m_socialProfiles = new RepeatingTableEditor({tr("Network"), tr("Value")}, this);
    m_socialProfiles->setColumnChoices(0, {"twitter", "linkedin", "instagram", "facebook", "mastodon", "website", "other"});
    m_messagingHandles = new RepeatingTableEditor({tr("App"), tr("Handle")}, this);
    m_messagingHandles->setColumnChoices(0, {"signal", "whatsapp", "telegram", "discord", "skype", "other"});
    auto *onlineLayout = new QVBoxLayout;
    onlineLayout->addWidget(new QLabel(tr("Social profiles")));
    onlineLayout->addWidget(m_socialProfiles);
    onlineLayout->addWidget(new QLabel(tr("Messaging handles")));
    onlineLayout->addWidget(m_messagingHandles);
    auto *onlineTab = new QWidget(this);
    onlineTab->setLayout(onlineLayout);

    m_customFields = new RepeatingTableEditor({tr("Label"), tr("Value")}, this);
    m_keyDates = new RepeatingTableEditor({tr("Label"), tr("Date (YYYY-MM-DD)")}, this);
    m_keyDates->setColumnChoices(0, {"birthday", "anniversary", "other"});
    auto *customLayout = new QVBoxLayout;
    customLayout->addWidget(new QLabel(tr("Custom fields")));
    customLayout->addWidget(m_customFields);
    customLayout->addWidget(new QLabel(tr("Key dates")));
    customLayout->addWidget(m_keyDates);
    auto *customTab = new QWidget(this);
    customTab->setLayout(customLayout);

    QStringList relatedChoices;
    for (const QJsonObject &c : m_allContacts)
        relatedChoices << contactChoiceLabel(c);
    m_relationships = new RepeatingTableEditor({tr("Relationship"), tr("Related Contact")}, this);
    m_relationships->setColumnChoices(0, {"spouse", "partner", "parent", "child", "sibling", "friend", "colleague", "assistant", "other"});
    m_relationships->setColumnChoices(1, relatedChoices);
    auto *relationshipsLayout = new QVBoxLayout;
    relationshipsLayout->addWidget(m_relationships);
    auto *relationshipsTab = new QWidget(this);
    relationshipsTab->setLayout(relationshipsLayout);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(generalTab, tr("General"));
    tabs->addTab(contactInfoTab, tr("Contact Info"));
    tabs->addTab(addressesTab, tr("Addresses"));
    tabs->addTab(onlineTab, tr("Online"));
    tabs->addTab(customTab, tr("Custom"));
    tabs->addTab(relationshipsTab, tr("Relationships"));

    m_error = new QLabel(this);
    m_error->setObjectName("errorLabel");
    m_error->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ContactEditDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(m_error);
    layout->addWidget(buttons);
}

QVector<QStringList> ContactEditDialog::arrayToRows(const QJsonArray &arr, const QStringList &keys) {
    QVector<QStringList> rows;
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        QStringList row;
        for (const QString &key : keys)
            row << o.value(key).toVariant().toString();
        rows << row;
    }
    return rows;
}

QJsonArray ContactEditDialog::rowsToArray(const QVector<QStringList> &rows, const QStringList &keys) {
    QJsonArray arr;
    for (const QStringList &row : rows) {
        QJsonObject o;
        for (int i = 0; i < keys.size() && i < row.size(); ++i)
            o[keys[i]] = row[i];
        arr.append(o);
    }
    return arr;
}

void ContactEditDialog::setContact(const QJsonObject &contact) {
    m_editingId = contact.value("id").toVariant().toLongLong();

    m_firstName->setText(contact.value("first_name").toString());
    m_lastName->setText(contact.value("last_name").toString());
    m_organization->setText(contact.value("organization").toString());
    m_title->setText(contact.value("title").toString());

    QJsonArray tags = contact.value("tags").toArray();
    QStringList tagList;
    for (const QJsonValue &t : tags) tagList << t.toString();
    m_tags->setText(tagList.join(", "));
    m_notes->setPlainText(contact.value("notes").toString());
    m_favorite->setChecked(contact.value("favorite").toInt() != 0 || contact.value("favorite").toBool());

    m_emails->setRows(arrayToRows(contact.value("emails").toArray(), {"type", "value"}));
    m_phones->setRows(arrayToRows(contact.value("phones").toArray(), {"type", "value"}));
    m_addresses->setRows(arrayToRows(contact.value("addresses").toArray(),
                                      {"type", "street", "city", "state", "postalCode", "country"}));
    m_socialProfiles->setRows(arrayToRows(contact.value("socialProfiles").toArray(), {"type", "value"}));
    m_messagingHandles->setRows(arrayToRows(contact.value("messagingHandles").toArray(), {"type", "value"}));
    m_customFields->setRows(arrayToRows(contact.value("customFields").toArray(), {"label", "value"}));
    m_keyDates->setRows(arrayToRows(contact.value("keyDates").toArray(), {"label", "date"}));

    QVector<QStringList> relRows;
    for (const QJsonValue &v : contact.value("relationships").toArray()) {
        const QJsonObject o = v.toObject();
        const qlonglong targetId = o.value("contactId").toVariant().toLongLong();
        QString targetLabel;
        for (const QJsonObject &c : m_allContacts) {
            if (c.value("id").toVariant().toLongLong() == targetId) {
                targetLabel = contactChoiceLabel(c);
                break;
            }
        }
        relRows << QStringList{o.value("type").toString(), targetLabel};
    }
    m_relationships->setRows(relRows);
}

QJsonObject ContactEditDialog::formData() const {
    QJsonObject data;
    data["firstName"] = m_firstName->text().trimmed();
    data["lastName"] = m_lastName->text().trimmed();
    data["fullName"] = (m_firstName->text().trimmed() + " " + m_lastName->text().trimmed()).trimmed();
    data["organization"] = m_organization->text().trimmed();
    data["title"] = m_title->text().trimmed();

    QJsonArray tags;
    for (const QString &t : m_tags->text().split(',', Qt::SkipEmptyParts))
        tags.append(t.trimmed());
    data["tags"] = tags;
    data["notes"] = m_notes->toPlainText();
    data["favorite"] = m_favorite->isChecked();

    data["emails"] = rowsToArray(m_emails->rows(), {"type", "value"});
    data["phones"] = rowsToArray(m_phones->rows(), {"type", "value"});
    data["addresses"] = rowsToArray(m_addresses->rows(), {"type", "street", "city", "state", "postalCode", "country"});
    data["socialProfiles"] = rowsToArray(m_socialProfiles->rows(), {"type", "value"});
    data["messagingHandles"] = rowsToArray(m_messagingHandles->rows(), {"type", "value"});
    data["customFields"] = rowsToArray(m_customFields->rows(), {"label", "value"});
    data["keyDates"] = rowsToArray(m_keyDates->rows(), {"label", "date"});

    QJsonArray relationships;
    for (const QStringList &row : m_relationships->rows()) {
        if (row.size() < 2) continue;
        const qlonglong contactId = parseContactIdFromChoice(row[1]);
        if (contactId <= 0) continue;
        relationships.append(QJsonObject{{"type", row[0]}, {"contactId", contactId}});
    }
    data["relationships"] = relationships;

    return data;
}

void ContactEditDialog::validateAndAccept() {
    if (m_firstName->text().trimmed().isEmpty() && m_lastName->text().trimmed().isEmpty()) {
        m_error->setText(tr("Enter at least a first or last name."));
        return;
    }
    accept();
}
