#pragma once

#include <QDialog>

class QLineEdit;
class QLabel;

// Create/edit dialog for an admin-managed user account. For edits, the
// password field is optional (left blank = keep current password).
class UserEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit UserEditDialog(bool isNew, QWidget *parent = nullptr);

    void setUsername(const QString &username);
    QString username() const;
    QString password() const; // may be empty when editing (= unchanged)

private slots:
    void validateAndAccept();

private:
    bool m_isNew;
    QLineEdit *m_username;
    QLineEdit *m_password;
    QLabel *m_error;
};
