#pragma once

#include <QDialog>

class ApiClient;
class QLineEdit;
class QLabel;
class QCheckBox;
class QPushButton;

// First-run setup wizard (POST /api/auth/setup): creates the single regular
// account plus a separate admin recovery password, and picks which modules
// (bookmarks/contacts/calendar/files/passwords) are enabled.
class SetupDialog : public QDialog {
    Q_OBJECT
public:
    explicit SetupDialog(ApiClient *api, QWidget *parent = nullptr);

private slots:
    void submit();

private:
    ApiClient *m_api;
    QLineEdit *m_username;
    QLineEdit *m_password;
    QLineEdit *m_passwordConfirm;
    QLineEdit *m_adminPassword;
    QCheckBox *m_bookmarks;
    QCheckBox *m_contacts;
    QCheckBox *m_calendar;
    QCheckBox *m_files;
    QCheckBox *m_passwords;
    QLabel *m_statusLabel;
    QPushButton *m_createButton;
    QPushButton *m_cancelButton;
};
