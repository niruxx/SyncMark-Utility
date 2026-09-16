#pragma once

#include <QSettings>
#include <QString>
#include <QStringList>

// Small wrapper around QSettings for client-local preferences (not server data).
class AppSettings {
public:
    static QString lastServerUrl() {
        return QSettings().value("server/lastUrl").toString();
    }
    static void setLastServerUrl(const QString &url) {
        QSettings().setValue("server/lastUrl", url);
    }

    static QStringList recentServerUrls() {
        return QSettings().value("server/recent").toStringList();
    }
    static void addRecentServerUrl(const QString &url) {
        QSettings settings;
        QStringList recent = settings.value("server/recent").toStringList();
        recent.removeAll(url);
        recent.prepend(url);
        while (recent.size() > 8)
            recent.removeLast();
        settings.setValue("server/recent", recent);
    }

    static bool rememberSession() {
        return QSettings().value("server/remember", true).toBool();
    }
    static void setRememberSession(bool v) {
        QSettings().setValue("server/remember", v);
    }

    static bool minimizeToTrayEnabled() {
        return QSettings().value("ui/minimizeToTray", true).toBool();
    }
    static void setMinimizeToTrayEnabled(bool v) {
        QSettings().setValue("ui/minimizeToTray", v);
    }
};
