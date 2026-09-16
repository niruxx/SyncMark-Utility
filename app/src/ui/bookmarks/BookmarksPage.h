#pragma once

#include "../common/PageWidget.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

class ApiClient;
class Session;
class QTreeWidget;
class QTreeWidgetItem;
class QTableWidget;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QPushButton;

// Bookmarks + folders module: folder tree on the left, filtered/sortable
// bookmark table on the right. Talks to /api/bookmarks, /api/folders,
// /api/export and /api/import.
class BookmarksPage : public PageWidget {
    Q_OBJECT
public:
    explicit BookmarksPage(ApiClient *api, QWidget *parent = nullptr);

    void reload() override;

private slots:
    void reloadFolders();
    void reloadBookmarks();
    void onFolderSelectionChanged();
    void onFolderContextMenu(const QPoint &pos);
    void onTableContextMenu(const QPoint &pos);
    void addBookmark();
    void editSelectedBookmark();
    void deleteSelectedBookmark();
    void toggleSelectedFavorite();
    void openSelectedUrl();
    void addFolder();
    void renameFolder(QTreeWidgetItem *item);
    void deleteFolder(QTreeWidgetItem *item);
    void importBookmarks();
    void exportBookmarks(const QString &format);

private:
    QStringList allFolderPaths() const;
    int selectedRow() const;
    QString currentFolderFilter() const; // empty = all
    bool favoritesOnly() const;
    void editBookmarkAt(int row);

    ApiClient *m_api;
    QTreeWidget *m_tree;
    QTableWidget *m_table;
    QLineEdit *m_search;
    QComboBox *m_sort;
    QVector<QJsonObject> m_bookmarks;
    QVector<QString> m_folderPaths;
};
