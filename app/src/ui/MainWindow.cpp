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
#include "common/Animations.h"
#include "Theme.h"
#include "../core/ApiClient.h"
#include "../core/Session.h"
#include "../core/AppSettings.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QToolButton>
#include <QAction>
#include <QFrame>
#include <QCloseEvent>
#include <QApplication>
#include <QTimer>
#include <QFont>
#include <QEvent>

MainWindow::MainWindow(ApiClient *api, Session *session, QWidget *parent)
    : QMainWindow(parent), m_api(api), m_session(session) {
    setWindowTitle(tr("SyncMark"));
    resize(1180, 760);

    m_nav = new QListWidget(this);
    m_nav->setObjectName("navList");
    m_nav->setFrameShape(QFrame::NoFrame);
    m_nav->setFocusPolicy(Qt::NoFocus);
    m_stack = new QStackedWidget(this);
    connect(m_nav, &QListWidget::currentRowChanged, m_stack, &QStackedWidget::setCurrentIndex);
    connect(m_stack, &QStackedWidget::currentChanged, this, [this](int index) {
        QWidget *page = m_stack->widget(index);
        if (!page) return;
        if (auto *pageWidget = qobject_cast<PageWidget *>(page))
            pageWidget->reload();
        Animations::fadeIn(page, 150);
    });

    m_dashboard = new DashboardPage(m_api, m_session, this);
    m_bookmarks = new BookmarksPage(m_api, this);
    m_contacts = new ContactsPage(m_api, this);
    m_calendar = new CalendarPage(m_api, this);
    m_passwords = new PasswordsPage(m_api, this);
    m_files = new FilesPage(m_api, m_session, this);
    m_adminUsers = new AdminUsersPage(m_api, this);
    m_backups = new BackupsPage(m_api, this);

    auto *sidebarLayout = new QVBoxLayout;
    sidebarLayout->setContentsMargins(0, 8, 0, 0);
    sidebarLayout->setSpacing(0);
    sidebarLayout->addWidget(m_nav, 1);

    auto *sidebarPanel = new QWidget(this);
    sidebarPanel->setObjectName("sidebarPanel");
    sidebarPanel->setLayout(sidebarLayout);
    sidebarPanel->setMinimumWidth(200);
    sidebarPanel->setMaximumWidth(220);

    auto *splitter = new QSplitter(this);
    splitter->setObjectName("mainSplitter");
    splitter->addWidget(sidebarPanel);
    splitter->addWidget(m_stack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setChildrenCollapsible(false);

    auto *central = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(buildTopBar());
    centralLayout->addWidget(splitter, 1);
    setCentralWidget(central);

    setupTray();
    rebuildNavigation();
}

QWidget *MainWindow::buildTopBar() {
    auto *topBar = new QWidget(this);
    topBar->setObjectName("topBar");
    topBar->setFixedHeight(56);

    auto *brandIcon = new QLabel(topBar);
    brandIcon->setPixmap(Theme::appIcon().pixmap(26, 26));
    auto *brandTitle = new QLabel(tr("SyncMark"), topBar);
    brandTitle->setObjectName("brandTitle");

    m_accountLabel = new QLabel(topBar);
    m_accountLabel->setObjectName("topBarSubtle");
    m_accountLabel->setText(tr("Signed in as %1").arg(m_session->username));

    auto *accountButton = new QToolButton(topBar);
    accountButton->setObjectName("accountButton");
    accountButton->setText(QStringLiteral("⚙  ") + tr("Account"));
    accountButton->setPopupMode(QToolButton::InstantPopup);
    accountButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto *accountMenu = new QMenu(accountButton);
    connect(accountMenu->addAction(tr("Account Settings...")), &QAction::triggered, this, &MainWindow::openAccountSettings);
    m_featuresAction = accountMenu->addAction(tr("Enabled Modules..."));
    connect(m_featuresAction, &QAction::triggered, this, &MainWindow::openFeatureToggles);
    m_featuresAction->setVisible(m_session->isAdmin());

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        accountMenu->addSeparator();
        m_minimizeToTrayAction = accountMenu->addAction(tr("Minimize to Tray on Close"));
        m_minimizeToTrayAction->setCheckable(true);
        m_minimizeToTrayAction->setChecked(AppSettings::minimizeToTrayEnabled());
        connect(m_minimizeToTrayAction, &QAction::toggled, this, &MainWindow::toggleMinimizeToTray);
    }

    accountMenu->addSeparator();
    connect(accountMenu->addAction(tr("Log Out")), &QAction::triggered, this, &MainWindow::logout);
    connect(accountMenu->addAction(tr("Quit SyncMark")), &QAction::triggered, this, &MainWindow::quitApplication);
    accountButton->setMenu(accountMenu);

    auto *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(16, 0, 12, 0);
    layout->setSpacing(10);
    layout->addWidget(brandIcon);
    layout->addWidget(brandTitle);
    layout->addStretch();
    layout->addWidget(m_accountLabel);
    layout->addWidget(accountButton);

    return topBar;
}

void MainWindow::setupTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    m_trayIcon = new QSystemTrayIcon(Theme::appIcon(), this);
    m_trayIcon->setToolTip(tr("SyncMark — %1").arg(m_session->username));

    m_trayMenu = new QMenu(this);
    QAction *openAction = m_trayMenu->addAction(tr("Open SyncMark"));
    QFont boldFont = openAction->font();
    boldFont.setBold(true);
    openAction->setFont(boldFont);
    connect(openAction, &QAction::triggered, this, &MainWindow::restoreFromTray);
    m_trayMenu->addSeparator();
    connect(m_trayMenu->addAction(tr("Log Out")), &QAction::triggered, this, &MainWindow::logout);
    connect(m_trayMenu->addAction(tr("Quit SyncMark")), &QAction::triggered, this, &MainWindow::quitApplication);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
    m_trayIcon->show();
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible() && !isMinimized())
            hide();
        else
            restoreFromTray();
    }
}

void MainWindow::restoreFromTray() {
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::toggleMinimizeToTray(bool enabled) {
    AppSettings::setMinimizeToTrayEnabled(enabled);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!m_forceQuit && m_trayIcon && m_trayIcon->isVisible() && AppSettings::minimizeToTrayEnabled()) {
        hide();
        if (!m_trayHintShown) {
            m_trayHintShown = true;
            m_trayIcon->showMessage(tr("SyncMark"),
                                     tr("Still running in the background. Click the tray icon to reopen, "
                                        "or use its menu to quit."),
                                     QSystemTrayIcon::Information, 4000);
        }
        event->ignore();
        return;
    }
    event->accept();
}

void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange && isMinimized() &&
        m_trayIcon && m_trayIcon->isVisible() && AppSettings::minimizeToTrayEnabled()) {
        QTimer::singleShot(0, this, &QWidget::hide);
    }
}

void MainWindow::addNavPage(const QString &label, QWidget *page) {
    m_nav->addItem(label);
    m_stack->addWidget(page);
}

void MainWindow::rebuildNavigation() {
    m_nav->clear();
    while (m_stack->count() > 0)
        m_stack->removeWidget(m_stack->widget(0));

    addNavPage(QStringLiteral("\U0001F3E0  ") + tr("Dashboard"), m_dashboard);
    if (m_session->featureEnabled("bookmarks"))
        addNavPage(QStringLiteral("\U0001F516  ") + tr("Bookmarks"), m_bookmarks);
    if (m_session->featureEnabled("contacts"))
        addNavPage(QStringLiteral("\U0001F464  ") + tr("Contacts"), m_contacts);
    if (m_session->featureEnabled("calendar"))
        addNavPage(QStringLiteral("\U0001F4C5  ") + tr("Calendar"), m_calendar);
    if (m_session->featureEnabled("passwords"))
        addNavPage(QStringLiteral("\U0001F512  ") + tr("Passwords"), m_passwords);
    if (m_session->featureEnabled("files"))
        addNavPage(QStringLiteral("\U0001F4C1  ") + tr("Files"), m_files);
    if (m_session->isAdmin()) {
        addNavPage(QStringLiteral("\U0001F465  ") + tr("Users"), m_adminUsers);
        addNavPage(QStringLiteral("\U0001F4BE  ") + tr("Backups"), m_backups);
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
        m_forceQuit = true;
        emit loggedOut();
    });
}

void MainWindow::quitApplication() {
    m_forceQuit = true;
    close();
    qApp->quit();
}
