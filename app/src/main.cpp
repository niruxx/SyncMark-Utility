#include <QApplication>
#include <QEventLoop>
#include <QJsonObject>
#include <QMessageBox>

#include "core/ApiClient.h"
#include "core/AppSettings.h"
#include "core/Session.h"
#include "ui/ConnectDialog.h"
#include "ui/SetupDialog.h"
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"

namespace {

// Blocks (via a nested event loop) until the given async call finishes.
// Used only for the startup handshake below, where a simple top-to-bottom
// flow is much easier to read than a chain of callbacks; every other part
// of the app uses ApiClient's callbacks directly and never blocks.
ApiResult waitFor(const std::function<void(ApiJsonCallback)> &starter) {
    QEventLoop loop;
    ApiResult out;
    starter([&](const ApiResult &r) {
        out = r;
        loop.quit();
    });
    loop.exec();
    return out;
}

QString normalizeUrl(QString url) {
    url = url.trimmed();
    if (!url.startsWith("http://") && !url.startsWith("https://"))
        url = "http://" + url;
    while (url.endsWith('/'))
        url.chop(1);
    return url;
}

// Runs the connect -> setup/login handshake. Returns false if the user gave up.
bool authenticate(ApiClient &api, Session &session) {
    ConnectDialog connectDialog;

    for (;;) {
        if (connectDialog.exec() != QDialog::Accepted)
            return false;

        const QString url = normalizeUrl(connectDialog.serverUrl());
        if (url.isEmpty()) {
            connectDialog.showError(QObject::tr("Enter a server URL."));
            continue;
        }

        api.setBaseUrl(url);
        connectDialog.setBusy(true, QObject::tr("Connecting..."));
        const ApiResult status = waitFor([&](ApiJsonCallback cb) { api.get("/auth/status", {}, cb); });
        connectDialog.setBusy(false);

        if (!status.ok) {
            connectDialog.showError(QObject::tr("Could not reach server: %1").arg(status.error));
            continue;
        }

        AppSettings::setLastServerUrl(url);
        AppSettings::addRecentServerUrl(url);

        const QJsonObject statusBody = status.body.object();
        session.setupRequired = statusBody.value("setupRequired").toBool();
        session.authenticated = statusBody.value("authenticated").toBool();
        session.role = statusBody.value("role").toString();

        if (session.setupRequired) {
            SetupDialog setupDialog(&api);
            if (setupDialog.exec() != QDialog::Accepted)
                continue;
            break;
        }
        if (!session.authenticated) {
            LoginDialog loginDialog(&api, url);
            if (loginDialog.exec() != QDialog::Accepted)
                continue;
            session.role = loginDialog.role();
            break;
        }
        break; // a persisted session cookie was already valid
    }

    session.authenticated = true;

    const ApiResult me = waitFor([&](ApiJsonCallback cb) { api.get("/auth/me", {}, cb); });
    if (me.ok) {
        session.username = me.body.object().value("username").toString();
    }
    if (session.role.isEmpty()) {
        const ApiResult status = waitFor([&](ApiJsonCallback cb) { api.get("/auth/status", {}, cb); });
        if (status.ok) session.role = status.body.object().value("role").toString();
    }

    const ApiResult features = waitFor([&](ApiJsonCallback cb) { api.get("/features", {}, cb); });
    if (features.ok)
        session.features = features.body.object();

    return true;
}

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("SyncMark");
    QApplication::setApplicationName("SyncMark Desktop Client");

    ApiClient api;
    Session session;

    MainWindow *window = nullptr;

    std::function<void()> startSession = [&]() {
        session.reset();
        if (!authenticate(api, session)) {
            QApplication::quit();
            return;
        }

        window = new MainWindow(&api, &session);
        QObject::connect(window, &MainWindow::loggedOut, &app, [&]() {
            window->deleteLater();
            window = nullptr;
            QMetaObject::invokeMethod(&app, [&]() { startSession(); }, Qt::QueuedConnection);
        });
        QObject::connect(&api, &ApiClient::unauthorized, &app, [&]() {
            if (!window) return;
            QMessageBox::information(nullptr, QObject::tr("Session Expired"),
                                      QObject::tr("Your session has expired. Please sign in again."));
            window->deleteLater();
            window = nullptr;
            QMetaObject::invokeMethod(&app, [&]() { startSession(); }, Qt::QueuedConnection);
        });
        window->show();
    };

    startSession();
    if (!window)
        return 0;

    return app.exec();
}
