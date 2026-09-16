#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>

class ApiClient;
class Session;
class QListWidget;
class QStackedWidget;
class QLabel;
class QMenu;
class QAction;
class QCloseEvent;

class DashboardPage;
class BookmarksPage;
class ContactsPage;
class CalendarPage;
class PasswordsPage;
class FilesPage;
class AdminUsersPage;
class BackupsPage;

// Top-level shell: a fixed top bar (brand + account menu) above a
// left-hand navigation list + stacked content pages, plus optional
// minimize-to-tray behavior. Pages are added/hidden based on the
// session's enabled features and role.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(ApiClient *api, Session *session, QWidget *parent = nullptr);

signals:
    void loggedOut();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void logout();
    void quitApplication();
    void openAccountSettings();
    void openFeatureToggles();
    void rebuildNavigation();
    void toggleMinimizeToTray(bool enabled);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void restoreFromTray();

private:
    void addNavPage(const QString &label, QWidget *page);
    QWidget *buildTopBar();
    void setupTray();

    ApiClient *m_api;
    Session *m_session;

    QListWidget *m_nav;
    QStackedWidget *m_stack;
    QLabel *m_accountLabel;

    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_featuresAction = nullptr;
    QAction *m_minimizeToTrayAction = nullptr;
    bool m_forceQuit = false;
    bool m_trayHintShown = false;

    DashboardPage *m_dashboard = nullptr;
    BookmarksPage *m_bookmarks = nullptr;
    ContactsPage *m_contacts = nullptr;
    CalendarPage *m_calendar = nullptr;
    PasswordsPage *m_passwords = nullptr;
    FilesPage *m_files = nullptr;
    AdminUsersPage *m_adminUsers = nullptr;
    BackupsPage *m_backups = nullptr;
};
