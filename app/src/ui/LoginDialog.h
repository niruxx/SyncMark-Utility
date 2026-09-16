#pragma once

#include <QDialog>

class ApiClient;
class QLineEdit;
class QLabel;
class QCheckBox;
class QPushButton;

// Logs in against an already-connected server (POST /api/auth/login).
// Only accept()s once the server confirms the session; stays open on error.
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(ApiClient *api, const QString &serverUrl, QWidget *parent = nullptr);

    QString role() const { return m_role; }

private slots:
    void submit();

private:
    ApiClient *m_api;
    QLineEdit *m_username;
    QLineEdit *m_password;
    QCheckBox *m_remember;
    QLabel *m_statusLabel;
    QPushButton *m_loginButton;
    QPushButton *m_cancelButton;
    QString m_role;
};
