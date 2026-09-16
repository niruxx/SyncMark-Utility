#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>

class QLineEdit;
class QPlainTextEdit;
class QCheckBox;
class QLabel;
class RepeatingTableEditor;

// Full add/edit dialog for a contact: identity, emails/phones, addresses,
// social profiles, messaging handles, custom fields, key dates and
// relationships to other contacts, plus tags/notes/favorite.
class ContactEditDialog : public QDialog {
    Q_OBJECT
public:
    // allContacts is used to populate the "related contact" picker; the
    // contact being edited (its "id") is excluded from that list.
    explicit ContactEditDialog(const QVector<QJsonObject> &allContacts, QWidget *parent = nullptr);

    void setContact(const QJsonObject &contact);
    QJsonObject formData() const;

private slots:
    void validateAndAccept();

private:
    static QVector<QStringList> arrayToRows(const QJsonArray &arr, const QStringList &keys);
    static QJsonArray rowsToArray(const QVector<QStringList> &rows, const QStringList &keys);

    qlonglong m_editingId = -1;
    QVector<QJsonObject> m_allContacts;

    QLineEdit *m_firstName;
    QLineEdit *m_lastName;
    QLineEdit *m_organization;
    QLineEdit *m_title;
    QLineEdit *m_tags;
    QPlainTextEdit *m_notes;
    QCheckBox *m_favorite;

    RepeatingTableEditor *m_emails;
    RepeatingTableEditor *m_phones;
    RepeatingTableEditor *m_addresses;
    RepeatingTableEditor *m_socialProfiles;
    RepeatingTableEditor *m_messagingHandles;
    RepeatingTableEditor *m_customFields;
    RepeatingTableEditor *m_keyDates;
    RepeatingTableEditor *m_relationships;

    QLabel *m_error;
};
