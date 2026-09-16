#pragma once

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QPlainTextEdit;
class QCheckBox;
class QLabel;

// Add/edit dialog for a contact. Covers the core identity fields plus
// emails/phones (as "type: value" lines) and tags/notes/favorite.
// Array fields the UI doesn't expose directly (addresses, social profiles,
// messaging handles, custom fields, key dates, relationships) are preserved
// unchanged from the original contact when editing.
class ContactEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit ContactEditDialog(QWidget *parent = nullptr);

    void setContact(const QJsonObject &contact);
    QJsonObject formData() const;

private slots:
    void validateAndAccept();

private:
    QJsonObject m_original;
    QLineEdit *m_firstName;
    QLineEdit *m_lastName;
    QLineEdit *m_organization;
    QLineEdit *m_title;
    QPlainTextEdit *m_emails;
    QPlainTextEdit *m_phones;
    QLineEdit *m_tags;
    QPlainTextEdit *m_notes;
    QCheckBox *m_favorite;
    QLabel *m_error;
};
