#pragma once

#include "../common/PageWidget.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

class ApiClient;
class Session;
class QComboBox;
class QTableWidget;
class QLabel;
class QPushButton;

// File browser module: manage sandboxed "locations" (absolute directory
// roots) and browse/upload/download/rename/delete/trash files within them.
class FilesPage : public PageWidget {
    Q_OBJECT
public:
    FilesPage(ApiClient *api, Session *session, QWidget *parent = nullptr);
    void reload() override;

private slots:
    void reloadLocations();
    void reloadListing();
    void navigateUp();
    void onRowActivated(int row, int column);
    void addLocation();
    void editLocation();
    void removeLocation();
    void newFolder();
    void uploadFiles();
    void downloadSelected();
    void renameSelected();
    void deleteSelected();
    void openTrash();
    void openPermissions();
    void onContextMenu(const QPoint &pos);

private:
    QString currentLocationId() const;
    QString currentLocationName() const;
    void navigateTo(const QString &path);
    int selectedRow() const;
    QJsonObject entryAt(int row) const;

    ApiClient *m_api;
    Session *m_session;
    QComboBox *m_locationCombo;
    QLabel *m_pathLabel;
    QTableWidget *m_table;
    QString m_currentPath;
    QVector<QJsonObject> m_entries;
    QVector<QJsonObject> m_locations;
};
