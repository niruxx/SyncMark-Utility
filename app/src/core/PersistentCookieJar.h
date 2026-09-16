#pragma once

#include <QNetworkCookieJar>
#include <QString>

// A cookie jar that persists SyncMark session cookies to QSettings, keyed by
// server URL, so the desktop client can stay logged in across restarts
// (mirrors the server's own "remember me" session duration setting).
class PersistentCookieJar : public QNetworkCookieJar {
    Q_OBJECT
public:
    explicit PersistentCookieJar(QObject *parent = nullptr);

    // Called whenever ApiClient's base URL changes; loads any saved cookies for it.
    void setActiveServer(const QString &baseUrl);
    void clearForActiveServer();

protected:
    bool insertCookie(const QNetworkCookie &cookie) override;
    bool deleteCookie(const QNetworkCookie &cookie) override;

private:
    void save();
    QString m_activeServer;
    QString settingsKey() const;
};
