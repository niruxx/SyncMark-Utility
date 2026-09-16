#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

// Holds the client's current view of "who am I / what's enabled", refreshed
// from GET /api/auth/status, /api/auth/me and /api/features.
class Session : public QObject {
    Q_OBJECT
public:
    explicit Session(QObject *parent = nullptr) : QObject(parent) {}

    bool authenticated = false;
    bool setupRequired = false;
    QString role;       // "user" or "admin"
    QString username;
    QJsonObject features; // { bookmarks, contacts, calendar, files, passwords }

    bool isAdmin() const { return role == QStringLiteral("admin"); }
    bool featureEnabled(const QString &name) const {
        return features.value(name).toBool(true);
    }

    void reset() {
        authenticated = false;
        role.clear();
        username.clear();
        features = {};
    }

signals:
    void changed();
};
