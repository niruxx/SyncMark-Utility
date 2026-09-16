#pragma once

#include "../common/PageWidget.h"
#include <QJsonObject>
#include <QVector>

class ApiClient;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;

// Address book module: a sidebar of All/Favorites/Groups (manual + smart),
// a searchable/multi-selectable contact table, full field editing, duplicate
// detection & merging, and CSV/vCard import/export.
class ContactsPage : public PageWidget {
    Q_OBJECT
public:
    explicit ContactsPage(ApiClient *api, QWidget *parent = nullptr);
    void reload() override;

private slots:
    void reloadGroups();
    void reloadContacts();
    void onSidebarSelectionChanged();
    void onSidebarContextMenu(const QPoint &pos);
    void addContact();
    void editSelectedContact();
    void deleteSelectedContacts();
    void toggleSelectedFavorite();
    void setPhotoForSelected();
    void removePhotoForSelected();
    void importContacts();
    void exportContacts(const QString &format);
    void onContextMenu(const QPoint &pos);
    void addGroup();
    void editGroup(QTreeWidgetItem *item);
    void deleteGroup(QTreeWidgetItem *item);
    void addSelectedToGroup(const QJsonObject &group);
    void removeSelectedFromCurrentGroup();
    void openDuplicates();
    void bulkAction(const QString &action);

private:
    QVector<int> selectedRows() const;
    QVector<qlonglong> selectedContactIds() const;
    void editContactAt(int row);
    QString currentGroupId() const; // empty if not viewing a group
    bool currentGroupIsManual() const;
    void populateTable(const QVector<QJsonObject> &contacts);
    void applyClientFilter();

    ApiClient *m_api;
    QTreeWidget *m_sidebar;
    QTableWidget *m_table;
    QLineEdit *m_search;
    QVector<QJsonObject> m_contacts;      // currently displayed (post server fetch, pre client filter)
    QVector<QJsonObject> m_filtered;      // what's actually in the table
    QVector<QJsonObject> m_groups;
};
