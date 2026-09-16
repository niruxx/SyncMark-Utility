#pragma once

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QPlainTextEdit;
class QCheckBox;
class QLabel;
class QToolButton;

// Add/edit dialog for a password vault entry. When editing, the caller must
// supply the decrypted password (from GET /passwords/:id) via setEntry().
class PasswordEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit PasswordEditDialog(QWidget *parent = nullptr);

    void setEntry(const QJsonObject &entry);
    QJsonObject formData() const;

private slots:
    void validateAndAccept();
    void togglePasswordVisibility();
    void generatePassword();

private:
    QLineEdit *m_siteName;
    QLineEdit *m_url;
    QLineEdit *m_username;
    QLineEdit *m_password;
    QToolButton *m_revealBtn;
    QPlainTextEdit *m_notes;
    QCheckBox *m_favorite;
    QLabel *m_error;
};
