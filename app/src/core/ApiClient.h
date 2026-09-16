#pragma once

#include <QObject>
#include <QString>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <functional>

class QHttpMultiPart;

// Result of a JSON API call.
struct ApiResult {
    bool ok = false;
    int status = 0;
    QJsonDocument body;
    QString error;
};

// Result of a raw (binary/text) API call.
struct ApiRawResult {
    bool ok = false;
    int status = 0;
    QByteArray data;
    QString contentType;
    QString fileName; // best-effort, parsed from Content-Disposition
    QString error;
};

using ApiJsonCallback = std::function<void(const ApiResult &)>;
using ApiRawCallback = std::function<void(const ApiRawResult &)>;

// Thin wrapper around QNetworkAccessManager that talks to a SyncMark server's
// REST API (see https://github.com/niruxx/SyncMark). Sessions are cookie
// based, so a QNetworkCookieJar is attached and persisted between runs.
class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject *parent = nullptr);

    void setBaseUrl(const QString &url);
    QString baseUrl() const { return m_baseUrl; }

    // Clears session cookies for the current base URL (logout).
    void clearSession();

    void get(const QString &path, const QUrlQuery &query, ApiJsonCallback cb);
    void postJson(const QString &path, const QJsonObject &body, ApiJsonCallback cb);
    void putJson(const QString &path, const QJsonObject &body, ApiJsonCallback cb);
    void putRaw(const QString &path, const QUrlQuery &query, const QByteArray &body,
                const QString &contentType, ApiJsonCallback cb);
    void del(const QString &path, ApiJsonCallback cb);
    void delJson(const QString &path, const QJsonObject &body, ApiJsonCallback cb);

    // For downloads/exports/binary content (images, files, csv, ics, ...).
    void getRaw(const QString &path, const QUrlQuery &query, ApiRawCallback cb);

    // Takes ownership of multiPart. Used for uploads (avatar, photo, file, imports).
    void postMultipart(const QString &path, QHttpMultiPart *multiPart, ApiJsonCallback cb);

    QUrl buildUrl(const QString &path, const QUrlQuery &query = {}) const;

signals:
    // Emitted whenever a request comes back 401, so the UI can prompt for login again.
    void unauthorized();

private:
    void send(const QString &method, const QUrl &url, const QByteArray &body,
              const QString &contentType, ApiJsonCallback cb);
    ApiResult toResult(class QNetworkReply *reply);

    QNetworkAccessManager m_nam;
    QString m_baseUrl;
};
