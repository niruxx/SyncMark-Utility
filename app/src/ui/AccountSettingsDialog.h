#pragma once

#include <QDialog>

class ApiClient;
class QLineEdit;
class QLabel;
class QComboBox;
class QPushButton;

// Account settings: profile (username/avatar), password change, session
// duration, and account deletion. Backed by /api/auth/{me,avatar,account,settings}.
class AccountSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit AccountSettingsDialog(ApiClient *api, QWidget *parent = nullptr);

signals:
    void accountDeleted();

private slots:
    void loadMe();
    void loadSettings();
    void changeAvatar();
    void removeAvatar();
    void saveProfile();
    void saveSessionDuration();
    void deleteAccount();

private:
    ApiClient *m_api;
    QLabel *m_avatarLabel;
    QLineEdit *m_username;
    QLineEdit *m_currentPassword;
    QLineEdit *m_newPassword;
    QComboBox *m_sessionDuration;
    QLabel *m_status;
};
