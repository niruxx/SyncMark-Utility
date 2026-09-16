#pragma once

#include <QMainWindow>

class ApiClient;
class Session;
class QListWidget;
class QStackedWidget;
class QLabel;

class DashboardPage;
class BookmarksPage;
class ContactsPage;
class CalendarPage;
class PasswordsPage;
class FilesPage;
class AdminUsersPage;
class BackupsPage;

// Top-level shell: left-hand navigation list + stacked content pages.
// Pages are added/hidden based on the session's enabled features and role.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(ApiClient *api, Session *session, QWidget *parent = nullptr);

signals:
    void loggedOut();

private slots:
    void logout();
    void openAccountSettings();
    void openFeatureToggles();
    void rebuildNavigation();

private:
    void addNavPage(const QString &label, QWidget *page);

    ApiClient *m_api;
    Session *m_session;

    QListWidget *m_nav;
    QStackedWidget *m_stack;
    QLabel *m_statusLabel;

    DashboardPage *m_dashboard = nullptr;
    BookmarksPage *m_bookmarks = nullptr;
    ContactsPage *m_contacts = nullptr;
    CalendarPage *m_calendar = nullptr;
    PasswordsPage *m_passwords = nullptr;
    FilesPage *m_files = nullptr;
    AdminUsersPage *m_adminUsers = nullptr;
    BackupsPage *m_backups = nullptr;
};
