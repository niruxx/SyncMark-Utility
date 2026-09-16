#include "PersistentCookieJar.h"

#include <QSettings>
#include <QUrl>
#include <QNetworkCookie>
#include <QCryptographicHash>

PersistentCookieJar::PersistentCookieJar(QObject *parent) : QNetworkCookieJar(parent) {}

QString PersistentCookieJar::settingsKey() const {
    const QByteArray hash = QCryptographicHash::hash(m_activeServer.toUtf8(), QCryptographicHash::Sha1);
    return QStringLiteral("cookies/%1").arg(QString::fromLatin1(hash.toHex()));
}

void PersistentCookieJar::setActiveServer(const QString &baseUrl) {
    m_activeServer = baseUrl;
    if (m_activeServer.isEmpty())
        return;

    QSettings settings;
    const QByteArray raw = settings.value(settingsKey()).toByteArray();
    const QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(raw);
    setCookiesFromUrl(cookies, QUrl(m_activeServer));
}

void PersistentCookieJar::clearForActiveServer() {
    setCookiesFromUrl({}, QUrl(m_activeServer));
    QSettings settings;
    settings.remove(settingsKey());
}

bool PersistentCookieJar::insertCookie(const QNetworkCookie &cookie) {
    bool changed = QNetworkCookieJar::insertCookie(cookie);
    if (changed) save();
    return changed;
}

bool PersistentCookieJar::deleteCookie(const QNetworkCookie &cookie) {
    bool changed = QNetworkCookieJar::deleteCookie(cookie);
    if (changed) save();
    return changed;
}

void PersistentCookieJar::save() {
    if (m_activeServer.isEmpty())
        return;
    const QList<QNetworkCookie> cookies = cookiesForUrl(QUrl(m_activeServer));
    QByteArray raw;
    for (const QNetworkCookie &c : cookies) {
        if (!raw.isEmpty()) raw += "\n";
        raw += c.toRawForm();
    }
    QSettings settings;
    settings.setValue(settingsKey(), raw);
}
