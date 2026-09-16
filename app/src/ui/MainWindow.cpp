#include "MainWindow.h"
#include "AccountSettingsDialog.h"
#include "FeatureTogglesDialog.h"
#include "DashboardPage.h"
#include "bookmarks/BookmarksPage.h"
#include "contacts/ContactsPage.h"
#include "calendar/CalendarPage.h"
#include "passwords/PasswordsPage.h"
#include "files/FilesPage.h"
#include "admin/AdminUsersPage.h"
#include "admin/BackupsPage.h"
#include "common/Notify.h"
#include "../core/ApiClient.h"
#include "../core/Session.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QAction>

MainWindow::MainWindow(ApiClient *api, Session *session, QWidget *parent)
    : QMainWindow(parent), m_api(api), m_session(session) {
    setWindowTitle(tr("SyncMark"));
    resize(1100, 720);

    m_nav = new QListWidget(this);
    m_nav->setMaximumWidth(180);
    m_stack = new QStackedWidget(this);
    connect(m_nav, &QListWidget::currentRowChanged, m_stack, &QStackedWidget::setCurrentIndex);
    connect(m_stack, &QStackedWidget::currentChanged, this, [this](int index) {
        if (auto *page = qobject_cast<PageWidget *>(m_stack->widget(index)))
            page->reload();
    });

    m_dashboard = new DashboardPage(m_api, m_session, this);
    m_bookmarks = new BookmarksPage(m_api, this);
    m_contacts = new ContactsPage(m_api, this);
    m_calendar = new CalendarPage(m_api, this);
    m_passwords = new PasswordsPage(m_api, this);
    m_files = new FilesPage(m_api, m_session, this);
    m_adminUsers = new AdminUsersPage(m_api, this);
    m_backups = new BackupsPage(m_api, this);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(m_nav);
    splitter->addWidget(m_stack);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    m_statusLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_statusLabel);

    auto *accountMenu = menuBar()->addMenu(tr("&Account"));
    connect(accountMenu->addAction(tr("Account Settings...")), &QAction::triggered, this, &MainWindow::openAccountSettings);
    QAction *featuresAction = accountMenu->addAction(tr("Enabled Modules..."));
    connect(featuresAction, &QAction::triggered, this, &MainWindow::openFeatureToggles);
    accountMenu->addSeparator();
    connect(accountMenu->addAction(tr("Log Out")), &QAction::triggered, this, &MainWindow::logout);
    featuresAction->setVisible(m_session->isAdmin());

    rebuildNavigation();
    m_statusLabel->setText(tr("Signed in as %1").arg(m_session->username));
}

void MainWindow::addNavPage(const QString &label, QWidget *page) {
    m_nav->addItem(label);
    m_stack->addWidget(page);
}

void MainWindow::rebuildNavigation() {
    m_nav->clear();
    while (m_stack->count() > 0)
        m_stack->removeWidget(m_stack->widget(0));

    addNavPage(tr("Dashboard"), m_dashboard);
    if (m_session->featureEnabled("bookmarks")) addNavPage(tr("Bookmarks"), m_bookmarks);
    if (m_session->featureEnabled("contacts")) addNavPage(tr("Contacts"), m_contacts);
    if (m_session->featureEnabled("calendar")) addNavPage(tr("Calendar"), m_calendar);
    if (m_session->featureEnabled("passwords")) addNavPage(tr("Passwords"), m_passwords);
    if (m_session->featureEnabled("files")) addNavPage(tr("Files"), m_files);
    if (m_session->isAdmin()) {
        addNavPage(tr("Users (Admin)"), m_adminUsers);
        addNavPage(tr("Backups (Admin)"), m_backups);
    }

    if (m_nav->count() > 0)
        m_nav->setCurrentRow(0);
}

void MainWindow::openAccountSettings() {
    AccountSettingsDialog dialog(m_api, this);
    connect(&dialog, &AccountSettingsDialog::accountDeleted, this, &MainWindow::logout);
    dialog.exec();
}

void MainWindow::openFeatureToggles() {
    auto *dialog = new FeatureTogglesDialog(m_api, this);
    connect(dialog, &FeatureTogglesDialog::featuresChanged, this, [this]() {
        m_api->get("/features", {}, [this](const ApiResult &result) {
            if (result.ok) m_session->features = result.body.object();
            rebuildNavigation();
        });
    });
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}

void MainWindow::logout() {
    m_api->postJson("/auth/logout", {}, [this](const ApiResult &) {
        m_api->clearSession();
        emit loggedOut();
    });
}
