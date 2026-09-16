#pragma once

#include "../common/PageWidget.h"
#include <QJsonObject>
#include <QVector>

class ApiClient;
class QTableWidget;

// Admin: on-demand and scheduled backups (/api/backups).
class BackupsPage : public PageWidget {
    Q_OBJECT
public:
    explicit BackupsPage(ApiClient *api, QWidget *parent = nullptr);
    void reload() override;

private slots:
    void reloadBackups();
    void runBackupNow();
    void downloadSelected();
    void deleteSelected();
    void restoreSelected();
    void openSchedule();
    void onContextMenu(const QPoint &pos);

private:
    int selectedRow() const;
    QStringList pickModules(const QString &title, const QStringList &preset = {"bookmarks", "contacts", "calendar", "files", "passwords"});

    ApiClient *m_api;
    QTableWidget *m_table;
    QVector<QJsonObject> m_backups;
};
