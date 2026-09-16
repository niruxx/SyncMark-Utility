#include "ApiClient.h"
#include "PersistentCookieJar.h"

#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QJsonParseError>
#include <QRegularExpression>

ApiClient::ApiClient(QObject *parent) : QObject(parent) {
    m_nam.setCookieJar(new PersistentCookieJar(&m_nam));
}

void ApiClient::setBaseUrl(const QString &url) {
    m_baseUrl = url;
    while (m_baseUrl.endsWith('/'))
        m_baseUrl.chop(1);
    if (auto *jar = qobject_cast<PersistentCookieJar *>(m_nam.cookieJar()))
        jar->setActiveServer(m_baseUrl);
}

void ApiClient::clearSession() {
    if (auto *jar = qobject_cast<PersistentCookieJar *>(m_nam.cookieJar()))
        jar->clearForActiveServer();
}

QUrl ApiClient::buildUrl(const QString &path, const QUrlQuery &query) const {
    QUrl url(m_baseUrl + "/api" + path);
    if (!query.isEmpty())
        url.setQuery(query);
    return url;
}

ApiResult ApiClient::toResult(QNetworkReply *reply) {
    ApiResult result;
    result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray raw = reply->readAll();

    QJsonParseError parseError{};
    QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (parseError.error == QJsonParseError::NoError)
        result.body = doc;

    result.ok = reply->error() == QNetworkReply::NoError && result.status >= 200 && result.status < 300;

    if (!result.ok) {
        if (result.body.isObject() && result.body.object().contains("error"))
            result.error = result.body.object().value("error").toString();
        else if (!raw.isEmpty() && parseError.error != QJsonParseError::NoError)
            result.error = QString::fromUtf8(raw.left(200));
        else
            result.error = reply->errorString();
        if (result.error.isEmpty())
            result.error = QStringLiteral("Request failed (HTTP %1)").arg(result.status);
    }
    return result;
}

void ApiClient::send(const QString &method, const QUrl &url, const QByteArray &body,
                      const QString &contentType, ApiJsonCallback cb) {
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    req.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = nullptr;
    if (method == "GET")
        reply = m_nam.get(req);
    else if (method == "POST")
        reply = m_nam.post(req, body);
    else if (method == "PUT")
        reply = m_nam.put(req, body);
    else if (method == "DELETE")
        reply = body.isEmpty() ? m_nam.deleteResource(req) : m_nam.sendCustomRequest(req, "DELETE", body);

    if (!reply) {
        if (cb) cb(ApiResult{false, 0, {}, QStringLiteral("Unsupported method")});
        return;
    }

    connect(reply, &QNetworkReply::finished, this, [this, reply, cb]() {
        ApiResult result = toResult(reply);
        if (result.status == 401)
            emit unauthorized();
        if (cb) cb(result);
        reply->deleteLater();
    });
}

void ApiClient::get(const QString &path, const QUrlQuery &query, ApiJsonCallback cb) {
    send("GET", buildUrl(path, query), {}, "application/json", std::move(cb));
}

void ApiClient::postJson(const QString &path, const QJsonObject &body, ApiJsonCallback cb) {
    send("POST", buildUrl(path), QJsonDocument(body).toJson(QJsonDocument::Compact),
         "application/json", std::move(cb));
}

void ApiClient::putJson(const QString &path, const QJsonObject &body, ApiJsonCallback cb) {
    send("PUT", buildUrl(path), QJsonDocument(body).toJson(QJsonDocument::Compact),
         "application/json", std::move(cb));
}

void ApiClient::putRaw(const QString &path, const QUrlQuery &query, const QByteArray &body,
                        const QString &contentType, ApiJsonCallback cb) {
    send("PUT", buildUrl(path, query), body, contentType, std::move(cb));
}

void ApiClient::del(const QString &path, ApiJsonCallback cb) {
    send("DELETE", buildUrl(path), {}, "application/json", std::move(cb));
}

void ApiClient::delJson(const QString &path, const QJsonObject &body, ApiJsonCallback cb) {
    send("DELETE", buildUrl(path), QJsonDocument(body).toJson(QJsonDocument::Compact),
         "application/json", std::move(cb));
}

void ApiClient::getRaw(const QString &path, const QUrlQuery &query, ApiRawCallback cb) {
    QNetworkRequest req(buildUrl(path, query));
    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb]() {
        ApiRawResult result;
        result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result.ok = reply->error() == QNetworkReply::NoError && result.status >= 200 && result.status < 300;
        result.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        const QByteArray disposition = reply->rawHeader("Content-Disposition");
        if (!disposition.isEmpty()) {
            QRegularExpression re("filename=\"?([^\";]+)\"?");
            auto m = re.match(QString::fromUtf8(disposition));
            if (m.hasMatch())
                result.fileName = m.captured(1);
        }
        if (result.ok) {
            result.data = reply->readAll();
        } else {
            const QByteArray raw = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(raw);
            if (doc.isObject() && doc.object().contains("error"))
                result.error = doc.object().value("error").toString();
            else
                result.error = reply->errorString();
        }
        if (result.status == 401)
            emit unauthorized();
        if (cb) cb(result);
        reply->deleteLater();
    });
}

void ApiClient::postMultipart(const QString &path, QHttpMultiPart *multiPart, ApiJsonCallback cb) {
    QNetworkRequest req(buildUrl(path));
    QNetworkReply *reply = m_nam.post(req, multiPart);
    multiPart->setParent(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb]() {
        ApiResult result = toResult(reply);
        if (result.status == 401)
            emit unauthorized();
        if (cb) cb(result);
        reply->deleteLater();
    });
}
