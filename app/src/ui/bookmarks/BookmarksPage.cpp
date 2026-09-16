#include "BookmarksPage.h"
#include "BookmarkEditDialog.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"

#include <QTreeWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QTimer>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>

namespace {
constexpr int KindRole = Qt::UserRole + 1;
constexpr int PathRole = Qt::UserRole + 2;
enum NodeKind { KindAll, KindFavorites, KindFolder };
}

BookmarksPage::BookmarksPage(ApiClient *api, QWidget *parent) : PageWidget(parent), m_api(api) {
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, &BookmarksPage::onFolderSelectionChanged);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &BookmarksPage::onFolderContextMenu);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search bookmarks..."));
    auto *searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(300);
    connect(m_search, &QLineEdit::textChanged, searchTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(searchTimer, &QTimer::timeout, this, &BookmarksPage::reloadBookmarks);

    m_sort = new QComboBox(this);
    m_sort->addItem(tr("Title A-Z"), "title-asc");
    m_sort->addItem(tr("Title Z-A"), "title-desc");
    m_sort->addItem(tr("Newest first"), "date-desc");
    m_sort->addItem(tr("Oldest first"), "date-asc");
    connect(m_sort, &QComboBox::currentIndexChanged, this, &BookmarksPage::reloadBookmarks);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("★"), tr("Title"), tr("URL"), tr("Folder")});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 28);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &BookmarksPage::onTableContextMenu);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) { editBookmarkAt(row); });

    auto *addFolderBtn = new QPushButton(tr("New Folder"), this);
    connect(addFolderBtn, &QPushButton::clicked, this, &BookmarksPage::addFolder);

    auto *addBookmarkBtn = new QPushButton(tr("New Bookmark"), this);
    connect(addBookmarkBtn, &QPushButton::clicked, this, &BookmarksPage::addBookmark);

    auto *importBtn = new QPushButton(tr("Import..."), this);
    connect(importBtn, &QPushButton::clicked, this, &BookmarksPage::importBookmarks);

    auto *exportBtn = new QToolButton(this);
    exportBtn->setText(tr("Export"));
    exportBtn->setPopupMode(QToolButton::InstantPopup);
    auto *exportMenu = new QMenu(exportBtn);
    connect(exportMenu->addAction(tr("Export as HTML...")), &QAction::triggered, this,
            [this] { exportBookmarks("html"); });
    connect(exportMenu->addAction(tr("Export as JSON...")), &QAction::triggered, this,
            [this] { exportBookmarks("json"); });
    exportBtn->setMenu(exportMenu);

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(addBookmarkBtn);
    toolbar->addWidget(addFolderBtn);
    toolbar->addStretch();
    toolbar->addWidget(importBtn);
    toolbar->addWidget(exportBtn);

    auto *filterBar = new QHBoxLayout;
    filterBar->addWidget(m_search, 1);
    filterBar->addWidget(m_sort);

    auto *rightLayout = new QVBoxLayout;
    rightLayout->addLayout(filterBar);
    rightLayout->addWidget(m_table);

    auto *rightWidget = new QWidget(this);
    rightWidget->setLayout(rightLayout);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(m_tree);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({200, 600});

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(splitter);
}

void BookmarksPage::reload() {
    reloadFolders();
    reloadBookmarks();
}

QStringList BookmarksPage::allFolderPaths() const {
    QStringList paths;
    for (const QString &p : m_folderPaths) paths << p;
    return paths;
}

void BookmarksPage::reloadFolders() {
    m_api->get("/folders", {}, [this](const ApiResult &result) {
        if (!result.ok || !result.body.isArray()) return;

        m_folderPaths.clear();
        QMap<QString, QTreeWidgetItem *> nodes;

        QTreeWidgetItem *currentSelection = m_tree->currentItem();
        QString previousPath = currentSelection ? currentSelection->data(0, PathRole).toString() : QString();
        int previousKind = currentSelection ? currentSelection->data(0, KindRole).toInt() : KindAll;

        m_tree->clear();

        auto *allItem = new QTreeWidgetItem(m_tree, {tr("All Bookmarks")});
        allItem->setData(0, KindRole, KindAll);
        auto *favItem = new QTreeWidgetItem(m_tree, {tr("★ Favorites")});
        favItem->setData(0, KindRole, KindFavorites);

        const QJsonArray arr = result.body.array();
        QStringList rawPaths;
        for (const QJsonValue &v : arr) {
            const QJsonObject o = v.toObject();
            QString path = o.value("name").toString();
            if (path.isEmpty()) path = o.value("folder").toString();
            if (path.isEmpty()) path = o.value("path").toString();
            if (!path.isEmpty()) rawPaths << path;
        }
        rawPaths.sort(Qt::CaseInsensitive);

        for (const QString &path : rawPaths) {
            if (nodes.contains(path)) continue;
            const QStringList parts = path.split('/', Qt::SkipEmptyParts);
            QString built;
            QTreeWidgetItem *parent = nullptr;
            for (const QString &part : parts) {
                built = built.isEmpty() ? part : built + "/" + part;
                if (nodes.contains(built)) {
                    parent = nodes.value(built);
                    continue;
                }
                auto *item = parent ? new QTreeWidgetItem(parent, {part}) : new QTreeWidgetItem(m_tree, {part});
                item->setData(0, KindRole, KindFolder);
                item->setData(0, PathRole, built);
                nodes.insert(built, item);
                parent = item;
                m_folderPaths << built;
            }
        }

        m_tree->expandAll();

        if (previousKind == KindFolder && nodes.contains(previousPath)) {
            m_tree->setCurrentItem(nodes.value(previousPath));
        } else if (previousKind == KindFavorites) {
            m_tree->setCurrentItem(favItem);
        } else {
            m_tree->setCurrentItem(allItem);
        }
    });
}

QString BookmarksPage::currentFolderFilter() const {
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item || item->data(0, KindRole).toInt() != KindFolder) return {};
    return item->data(0, PathRole).toString();
}

bool BookmarksPage::favoritesOnly() const {
    QTreeWidgetItem *item = m_tree->currentItem();
    return item && item->data(0, KindRole).toInt() == KindFavorites;
}

void BookmarksPage::onFolderSelectionChanged() {
    reloadBookmarks();
}

void BookmarksPage::reloadBookmarks() {
    QUrlQuery query;
    if (!m_search->text().trimmed().isEmpty())
        query.addQueryItem("q", m_search->text().trimmed());
    const QString folder = currentFolderFilter();
    if (!folder.isEmpty())
        query.addQueryItem("folder", folder);
    if (favoritesOnly())
        query.addQueryItem("favorite", "1");
    query.addQueryItem("sort", m_sort->currentData().toString());

    m_api->get("/bookmarks", query, [this](const ApiResult &result) {
        if (!result.ok || !result.body.isArray()) {
            if (!result.ok) Notify::error(this, tr("Failed to load bookmarks: %1").arg(result.error));
            return;
        }
        m_bookmarks.clear();
        const QJsonArray arr = result.body.array();
        for (const QJsonValue &v : arr) m_bookmarks << v.toObject();

        m_table->setRowCount(m_bookmarks.size());
        for (int row = 0; row < m_bookmarks.size(); ++row) {
            const QJsonObject &bm = m_bookmarks[row];
            auto *favItem = new QTableWidgetItem(bm.value("favorite").toBool() ? QStringLiteral("★") : QString());
            favItem->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, 0, favItem);
            m_table->setItem(row, 1, new QTableWidgetItem(bm.value("title").toString()));
            m_table->setItem(row, 2, new QTableWidgetItem(bm.value("url").toString()));
            m_table->setItem(row, 3, new QTableWidgetItem(bm.value("folder").toString()));
        }
    });
}

int BookmarksPage::selectedRow() const {
    const auto selected = m_table->selectionModel()->selectedRows();
    return selected.isEmpty() ? -1 : selected.first().row();
}

void BookmarksPage::onFolderContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = m_tree->itemAt(pos);
    QMenu menu(this);
    QAction *newFolderAction = menu.addAction(tr("New Folder..."));
    QAction *renameAction = nullptr;
    QAction *deleteAction = nullptr;
    if (item && item->data(0, KindRole).toInt() == KindFolder) {
        renameAction = menu.addAction(tr("Rename..."));
        deleteAction = menu.addAction(tr("Delete Folder"));
    }
    QAction *chosen = menu.exec(m_tree->viewport()->mapToGlobal(pos));
    if (!chosen) return;
    if (chosen == newFolderAction) addFolder();
    else if (chosen == renameAction) renameFolder(item);
    else if (chosen == deleteAction) deleteFolder(item);
}

void BookmarksPage::onTableContextMenu(const QPoint &pos) {
    if (m_table->itemAt(pos)) m_table->selectRow(m_table->itemAt(pos)->row());
    if (selectedRow() < 0) return;
    QMenu menu(this);
    QAction *openAction = menu.addAction(tr("Open in Browser"));
    QAction *editAction = menu.addAction(tr("Edit..."));
    QAction *favAction = menu.addAction(tr("Toggle Favorite"));
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(tr("Delete"));
    QAction *chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (chosen == openAction) openSelectedUrl();
    else if (chosen == editAction) editSelectedBookmark();
    else if (chosen == favAction) toggleSelectedFavorite();
    else if (chosen == deleteAction) deleteSelectedBookmark();
}

void BookmarksPage::addBookmark() {
    BookmarkEditDialog dialog(allFolderPaths(), this);
    const QString folder = currentFolderFilter();
    if (!folder.isEmpty()) {
        QJsonObject prefill{{"folder", folder}};
        dialog.setBookmark(prefill);
    }
    if (dialog.exec() != QDialog::Accepted) return;

    m_api->postJson("/bookmarks", dialog.formData(), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not create bookmark: %1").arg(result.error)); return; }
        reloadFolders();
        reloadBookmarks();
    });
}

void BookmarksPage::editBookmarkAt(int row) {
    if (row < 0 || row >= m_bookmarks.size()) return;
    const QJsonObject bm = m_bookmarks[row];
    BookmarkEditDialog dialog(allFolderPaths(), this);
    dialog.setBookmark(bm);
    if (dialog.exec() != QDialog::Accepted) return;

    const QString id = QString::number(bm.value("id").toVariant().toLongLong());
    m_api->putJson("/bookmarks/" + id, dialog.formData(), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not update bookmark: %1").arg(result.error)); return; }
        reloadFolders();
        reloadBookmarks();
    });
}

void BookmarksPage::editSelectedBookmark() {
    editBookmarkAt(selectedRow());
}

void BookmarksPage::deleteSelectedBookmark() {
    int row = selectedRow();
    if (row < 0) return;
    if (!Notify::confirm(this, tr("Delete Bookmark"), tr("Delete \"%1\"?").arg(m_bookmarks[row].value("title").toString())))
        return;
    const QString id = QString::number(m_bookmarks[row].value("id").toVariant().toLongLong());
    m_api->del("/bookmarks/" + id, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not delete bookmark: %1").arg(result.error)); return; }
        reloadBookmarks();
    });
}

void BookmarksPage::toggleSelectedFavorite() {
    int row = selectedRow();
    if (row < 0) return;
    const bool newFavorite = !m_bookmarks[row].value("favorite").toBool();
    const QString id = QString::number(m_bookmarks[row].value("id").toVariant().toLongLong());
    m_api->putJson("/bookmarks/" + id + "/favorite", {{"favorite", newFavorite}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not update favorite: %1").arg(result.error)); return; }
        reloadBookmarks();
    });
}

void BookmarksPage::openSelectedUrl() {
    int row = selectedRow();
    if (row < 0) return;
    QDesktopServices::openUrl(QUrl(m_bookmarks[row].value("url").toString()));
}

void BookmarksPage::addFolder() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New Folder"),
                                                tr("Folder name (use / for nested, e.g. Work/Projects):"),
                                                QLineEdit::Normal, {}, &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    m_api->postJson("/folders", {{"name", name.trimmed()}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not create folder: %1").arg(result.error)); return; }
        reloadFolders();
    });
}

void BookmarksPage::renameFolder(QTreeWidgetItem *item) {
    if (!item) return;
    const QString oldName = item->data(0, PathRole).toString();
    bool ok = false;
    const QString newName = QInputDialog::getText(this, tr("Rename Folder"), tr("New name:"),
                                                    QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.trimmed().isEmpty() || newName == oldName) return;
    m_api->putJson("/folders", {{"oldName", oldName}, {"newName", newName.trimmed()}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not rename folder: %1").arg(result.error)); return; }
        reloadFolders();
        reloadBookmarks();
    });
}

void BookmarksPage::deleteFolder(QTreeWidgetItem *item) {
    if (!item) return;
    const QString name = item->data(0, PathRole).toString();
    if (!Notify::confirm(this, tr("Delete Folder"),
                          tr("Delete folder \"%1\" and unfile its bookmarks?").arg(name)))
        return;
    m_api->delJson("/folders", {{"name", name}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not delete folder: %1").arg(result.error)); return; }
        reloadFolders();
        reloadBookmarks();
    });
}

void BookmarksPage::importBookmarks() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Import Bookmarks"), {},
        tr("Bookmark files (*.html *.htm *.json *.csv);;All files (*)"));
    if (path.isEmpty()) return;

    auto *file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) {
        Notify::error(this, tr("Could not open file: %1").arg(path));
        delete file;
        return;
    }

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QVariant(QStringLiteral("form-data; name=\"file\"; filename=\"%1\"").arg(QFileInfo(path).fileName())));
    file->setParent(multiPart);
    part.setBodyDevice(file);
    multiPart->append(part);

    m_api->postMultipart("/import", multiPart, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Import failed: %1").arg(result.error)); return; }
        const int imported = result.body.object().value("imported").toInt();
        reloadFolders();
        reloadBookmarks();
        Notify::info(this, tr("Imported %1 bookmark(s).").arg(imported));
    });
}

void BookmarksPage::exportBookmarks(const QString &format) {
    QUrlQuery query;
    query.addQueryItem("format", format);
    m_api->getRaw("/export", query, [this, format](const ApiRawResult &result) {
        if (!result.ok) { Notify::error(this, tr("Export failed: %1").arg(result.error)); return; }
        const QString defaultName = QStringLiteral("syncmark-export.%1").arg(format == "json" ? "json" : "html");
        const QString path = QFileDialog::getSaveFileName(this, tr("Save Export"), defaultName);
        if (path.isEmpty()) return;
        QFile out(path);
        if (!out.open(QIODevice::WriteOnly)) { Notify::error(this, tr("Could not write file.")); return; }
        out.write(result.data);
    });
}
