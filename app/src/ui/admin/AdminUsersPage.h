#pragma once

#include "../common/PageWidget.h"
#include <QJsonObject>
#include <QVector>

class ApiClient;
class QTableWidget;

// Admin: user lifecycle management (/api/admin/users).
class AdminUsersPage : public PageWidget {
    Q_OBJECT
public:
    explicit AdminUsersPage(ApiClient *api, QWidget *parent = nullptr);
    void reload() override;

private slots:
    void reloadUsers();
    void addUser();
    void editSelectedUser();
    void toggleSelectedEnabled();
    void deleteSelectedUser();
    void onContextMenu(const QPoint &pos);

private:
    int selectedRow() const;

    ApiClient *m_api;
    QTableWidget *m_table;
    QVector<QJsonObject> m_users;
};
