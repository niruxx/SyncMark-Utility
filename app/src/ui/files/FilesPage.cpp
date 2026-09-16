#include "FilesPage.h"
#include "FileLocationDialog.h"
#include "TextFileDialog.h"
#include "PermissionsDialog.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"
#include "../../core/Session.h"

#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrlQuery>
#include <QJsonArray>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QDesktopServices>
#include <QTemporaryDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLocale>
#include <QLineEdit>
#include <functional>
#include <memory>
#include <algorithm>

namespace {
QString formatSize(qint64 bytes) {
    return QLocale::system().formattedDataSize(bytes);
}
QString joinPath(const QString &base, const QString &name) {
    if (base.isEmpty()) return name;
    return base.endsWith('/') ? base + name : base + "/" + name;
}
bool looksLikeText(const QString &name) {
    static const QStringList exts = {"txt", "md", "markdown", "json", "csv", "log",
                                      "xml", "yaml", "yml", "ini", "conf", "cfg", "js", "ts",
                                      "py", "sh", "html", "css"};
    return exts.contains(QFileInfo(name).suffix().toLower());
}
}

FilesPage::FilesPage(ApiClient *api, Session *session, QWidget *parent)
    : PageWidget(parent), m_api(api), m_session(session) {
    m_locationCombo = new QComboBox(this);
    connect(m_locationCombo, &QComboBox::currentIndexChanged, this, [this](int) { navigateTo(""); });

    auto *addLocBtn = new QPushButton(tr("+"), this);
    addLocBtn->setToolTip(tr("Add location"));
    addLocBtn->setFixedWidth(28);
    connect(addLocBtn, &QPushButton::clicked, this, &FilesPage::addLocation);
    auto *editLocBtn = new QPushButton(tr("Edit"), this);
    connect(editLocBtn, &QPushButton::clicked, this, &FilesPage::editLocation);
    auto *removeLocBtn = new QPushButton(tr("Remove"), this);
    connect(removeLocBtn, &QPushButton::clicked, this, &FilesPage::removeLocation);

    auto *locationRow = new QHBoxLayout;
    locationRow->addWidget(new QLabel(tr("Location:"), this));
    locationRow->addWidget(m_locationCombo, 1);
    locationRow->addWidget(addLocBtn);
    locationRow->addWidget(editLocBtn);
    locationRow->addWidget(removeLocBtn);

    auto *upBtn = new QPushButton(tr("Up"), this);
    connect(upBtn, &QPushButton::clicked, this, &FilesPage::navigateUp);
    m_pathLabel = new QLabel(this);
    m_pathLabel->setStyleSheet("font-family: monospace;");

    auto *pathRow = new QHBoxLayout;
    pathRow->addWidget(upBtn);
    pathRow->addWidget(m_pathLabel, 1);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Size"), tr("Modified")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(30);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &FilesPage::onContextMenu);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &FilesPage::onRowActivated);

    auto *newFolderBtn = new QPushButton(tr("New Folder"), this);
    connect(newFolderBtn, &QPushButton::clicked, this, &FilesPage::newFolder);
    auto *uploadBtn = new QPushButton(tr("Upload..."), this);
    connect(uploadBtn, &QPushButton::clicked, this, &FilesPage::uploadFiles);
    auto *downloadBtn = new QPushButton(tr("Download"), this);
    connect(downloadBtn, &QPushButton::clicked, this, &FilesPage::downloadSelected);
    auto *renameBtn = new QPushButton(tr("Rename..."), this);
    connect(renameBtn, &QPushButton::clicked, this, &FilesPage::renameSelected);
    auto *deleteBtn = new QPushButton(tr("Delete"), this);
    connect(deleteBtn, &QPushButton::clicked, this, &FilesPage::deleteSelected);
    auto *trashBtn = new QPushButton(tr("Trash..."), this);
    connect(trashBtn, &QPushButton::clicked, this, &FilesPage::openTrash);

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(newFolderBtn);
    toolbar->addWidget(uploadBtn);
    toolbar->addWidget(downloadBtn);
    toolbar->addWidget(renameBtn);
    toolbar->addWidget(deleteBtn);
    toolbar->addStretch();
    toolbar->addWidget(trashBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(locationRow);
    layout->addLayout(toolbar);
    layout->addLayout(pathRow);
    layout->addWidget(m_table);
}

void FilesPage::reload() { reloadLocations(); }

void FilesPage::reloadLocations() {
    m_api->get("/files/locations", {}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Failed to load locations: %1").arg(result.error)); return; }
        const QString previousId = currentLocationId();
        m_locations.clear();
        m_locationCombo->clear();
        for (const QJsonValue &v : result.body.array()) {
            const QJsonObject loc = v.toObject();
            m_locations << loc;
            m_locationCombo->addItem(loc.value("name").toString(), loc.value("id").toVariant());
        }
        int idx = m_locationCombo->findData(previousId);
        m_locationCombo->setCurrentIndex(idx >= 0 ? idx : 0);
        if (m_locationCombo->count() == 0) {
            m_table->setRowCount(0);
            m_pathLabel->setText(tr("No locations configured. Click + to add one."));
        } else {
            navigateTo("");
        }
    });
}

QString FilesPage::currentLocationId() const {
    return m_locationCombo->currentData().toString();
}

QString FilesPage::currentLocationName() const {
    return m_locationCombo->currentText();
}

void FilesPage::navigateUp() {
    int idx = m_currentPath.lastIndexOf('/');
    navigateTo(idx >= 0 ? m_currentPath.left(idx) : "");
}

void FilesPage::navigateTo(const QString &path) {
    m_currentPath = path;
    reloadListing();
}

void FilesPage::reloadListing() {
    if (currentLocationId().isEmpty()) { m_table->setRowCount(0); return; }

    QUrlQuery query;
    query.addQueryItem("location", currentLocationId());
    if (!m_currentPath.isEmpty()) query.addQueryItem("path", m_currentPath);

    m_pathLabel->setText(QStringLiteral("%1:/%2").arg(currentLocationName(), m_currentPath));

    m_api->get("/files/browse", query, [this](const ApiResult &result) {
        if (!result.ok || !result.body.isArray()) {
            if (!result.ok) Notify::error(this, tr("Failed to list files: %1").arg(result.error));
            return;
        }
        m_entries.clear();
        for (const QJsonValue &v : result.body.array()) m_entries << v.toObject();

        m_table->setRowCount(m_entries.size());
        for (int row = 0; row < m_entries.size(); ++row) {
            const QJsonObject &e = m_entries[row];
            const bool isDir = e.value("type").toString() == "dir";
            auto *nameItem = new QTableWidgetItem((isDir ? QStringLiteral("\U0001F4C1 ") : QStringLiteral("\U0001F4C4 ")) + e.value("name").toString());
            m_table->setItem(row, 0, nameItem);
            m_table->setItem(row, 1, new QTableWidgetItem(isDir ? QString() : formatSize(e.value("size").toVariant().toLongLong())));
            m_table->setItem(row, 2, new QTableWidgetItem(e.value("modifiedAt").toString()));
        }
    });
}

void FilesPage::onRowActivated(int row, int) {
    QJsonObject entry = entryAt(row);
    if (entry.isEmpty()) return;
    const QString name = entry.value("name").toString();
    const QString fullPath = joinPath(m_currentPath, name);
    if (entry.value("type").toString() == "dir") {
        navigateTo(fullPath);
        return;
    }
    if (looksLikeText(name)) {
        auto *dialog = new TextFileDialog(m_api, currentLocationId(), fullPath, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
        return;
    }
    downloadSelected();
}

int FilesPage::selectedRow() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

QJsonObject FilesPage::entryAt(int row) const {
    if (row < 0 || row >= m_entries.size()) return {};
    return m_entries[row];
}

void FilesPage::onContextMenu(const QPoint &pos) {
    if (m_table->itemAt(pos)) m_table->selectRow(m_table->itemAt(pos)->row());
    QMenu menu(this);
    QAction *openAction = selectedRow() >= 0 ? menu.addAction(tr("Open")) : nullptr;
    QAction *downloadAction = selectedRow() >= 0 ? menu.addAction(tr("Download")) : nullptr;
    QAction *renameAction = selectedRow() >= 0 ? menu.addAction(tr("Rename...")) : nullptr;
    QAction *permissionsAction = selectedRow() >= 0 ? menu.addAction(tr("Permissions...")) : nullptr;
    QAction *deleteAction = selectedRow() >= 0 ? menu.addAction(tr("Delete")) : nullptr;
    menu.addSeparator();
    QAction *newFolderAction = menu.addAction(tr("New Folder..."));
    QAction *uploadAction = menu.addAction(tr("Upload..."));
    QAction *chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (chosen == openAction) onRowActivated(selectedRow(), 0);
    else if (chosen == downloadAction) downloadSelected();
    else if (chosen == renameAction) renameSelected();
    else if (chosen == permissionsAction) openPermissions();
    else if (chosen == deleteAction) deleteSelected();
    else if (chosen == newFolderAction) newFolder();
    else if (chosen == uploadAction) uploadFiles();
}

void FilesPage::addLocation() {
    FileLocationDialog dialog(m_api, m_session->isAdmin(), this);
    if (dialog.exec() != QDialog::Accepted) return;
    m_api->postJson("/files/locations", {{"name", dialog.name()}, {"path", dialog.path()}},
                     [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not add location: %1").arg(result.error)); return; }
        reloadLocations();
    });
}

void FilesPage::editLocation() {
    if (m_locationCombo->count() == 0) return;
    const QString id = currentLocationId();
    const auto it = std::find_if(m_locations.begin(), m_locations.end(), [&id](const QJsonObject &l) {
        return l.value("id").toVariant().toString() == id;
    });
    if (it == m_locations.end()) return;

    FileLocationDialog dialog(m_api, m_session->isAdmin(), this);
    dialog.setValues(it->value("name").toString(), it->value("path").toString());
    if (dialog.exec() != QDialog::Accepted) return;
    m_api->putJson(QStringLiteral("/files/locations/%1").arg(id), {{"name", dialog.name()}, {"path", dialog.path()}},
                    [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not update location: %1").arg(result.error)); return; }
        reloadLocations();
    });
}

void FilesPage::removeLocation() {
    if (m_locationCombo->count() == 0) return;
    if (!Notify::confirm(this, tr("Remove Location"),
                          tr("Remove location \"%1\"? The files themselves are not deleted.").arg(currentLocationName())))
        return;
    m_api->del(QStringLiteral("/files/locations/%1").arg(currentLocationId()), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not remove location: %1").arg(result.error)); return; }
        reloadLocations();
    });
}

void FilesPage::newFolder() {
    if (currentLocationId().isEmpty()) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New Folder"), tr("Folder name:"), QLineEdit::Normal, {}, &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    QJsonObject body{{"location", currentLocationId()}, {"name", name.trimmed()}};
    if (!m_currentPath.isEmpty()) body["path"] = m_currentPath;
    m_api->postJson("/files/mkdir", body, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not create folder: %1").arg(result.error)); return; }
        reloadListing();
    });
}

void FilesPage::uploadFiles() {
    if (currentLocationId().isEmpty()) return;
    const QStringList paths = QFileDialog::getOpenFileNames(this, tr("Upload Files"));
    if (paths.isEmpty()) return;

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    for (const QString &localPath : paths) {
        auto *file = new QFile(localPath);
        if (!file->open(QIODevice::ReadOnly)) { delete file; continue; }
        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QStringLiteral("form-data; name=\"files\"; filename=\"%1\"").arg(QFileInfo(localPath).fileName())));
        file->setParent(multiPart);
        part.setBodyDevice(file);
        multiPart->append(part);
    }

    QUrlQuery query;
    query.addQueryItem("location", currentLocationId());
    if (!m_currentPath.isEmpty()) query.addQueryItem("path", m_currentPath);

    m_api->postMultipart(QStringLiteral("/files/upload?%1").arg(query.toString(QUrl::FullyEncoded)), multiPart,
                          [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Upload failed: %1").arg(result.error)); return; }
        reloadListing();
    });
}

void FilesPage::downloadSelected() {
    int row = selectedRow();
    if (row < 0) return;
    const QJsonObject entry = entryAt(row);
    if (entry.value("type").toString() == "dir") return;
    const QString fullPath = joinPath(m_currentPath, entry.value("name").toString());

    QUrlQuery query;
    query.addQueryItem("location", currentLocationId());
    query.addQueryItem("path", fullPath);

    m_api->getRaw("/files/download", query, [this, entry](const ApiRawResult &result) {
        if (!result.ok) { Notify::error(this, tr("Download failed: %1").arg(result.error)); return; }
        const QString savePath = QFileDialog::getSaveFileName(this, tr("Save File"), entry.value("name").toString());
        if (savePath.isEmpty()) return;
        QFile out(savePath);
        if (!out.open(QIODevice::WriteOnly)) { Notify::error(this, tr("Could not write file.")); return; }
        out.write(result.data);
    });
}

void FilesPage::renameSelected() {
    int row = selectedRow();
    if (row < 0) return;
    const QJsonObject entry = entryAt(row);
    const QString fullPath = joinPath(m_currentPath, entry.value("name").toString());
    bool ok = false;
    const QString newName = QInputDialog::getText(this, tr("Rename"), tr("New name:"),
                                                    QLineEdit::Normal, entry.value("name").toString(), &ok);
    if (!ok || newName.trimmed().isEmpty()) return;
    m_api->putJson("/files/rename", {{"location", currentLocationId()}, {"path", fullPath}, {"newName", newName.trimmed()}},
                    [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not rename: %1").arg(result.error)); return; }
        reloadListing();
    });
}

void FilesPage::deleteSelected() {
    int row = selectedRow();
    if (row < 0) return;
    const QJsonObject entry = entryAt(row);
    if (!Notify::confirm(this, tr("Delete"), tr("Move \"%1\" to trash?").arg(entry.value("name").toString())))
        return;
    const QString fullPath = joinPath(m_currentPath, entry.value("name").toString());
    QUrlQuery query;
    query.addQueryItem("location", currentLocationId());
    query.addQueryItem("path", fullPath);
    m_api->del(QStringLiteral("/files/item?%1").arg(query.toString(QUrl::FullyEncoded)), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not delete: %1").arg(result.error)); return; }
        reloadListing();
    });
}

void FilesPage::openPermissions() {
    int row = selectedRow();
    if (row < 0) return;
    const QJsonObject entry = entryAt(row);
    const QString fullPath = joinPath(m_currentPath, entry.value("name").toString());
    PermissionsDialog dialog(m_api, currentLocationId(), fullPath, this);
    dialog.exec();
}

void FilesPage::openTrash() {
    if (currentLocationId().isEmpty()) return;
    const QString locationId = currentLocationId();

    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Trash - %1").arg(currentLocationName()));
    dialog->setMinimumSize(520, 400);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    auto *table = new QTableWidget(dialog);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({tr("Name"), tr("Original Path"), tr("Deleted")});
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(28);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto refresh = std::make_shared<std::function<void()>>();
    auto trashItems = std::make_shared<QVector<QJsonObject>>();
    *refresh = [this, table, locationId, trashItems]() {
        QUrlQuery q;
        q.addQueryItem("location", locationId);
        m_api->get("/files/trash", q, [table, trashItems](const ApiResult &result) {
            if (!result.ok) return;
            trashItems->clear();
            for (const QJsonValue &v : result.body.array()) *trashItems << v.toObject();
            table->setRowCount(trashItems->size());
            for (int row = 0; row < trashItems->size(); ++row) {
                const QJsonObject &t = (*trashItems)[row];
                table->setItem(row, 0, new QTableWidgetItem(t.value("name").toString()));
                table->setItem(row, 1, new QTableWidgetItem(t.value("originalRelPath").toString()));
                table->setItem(row, 2, new QTableWidgetItem(t.value("deletedAt").toString()));
            }
        });
    };

    auto *restoreBtn = new QPushButton(tr("Restore"), dialog);
    connect(restoreBtn, &QPushButton::clicked, dialog, [this, table, trashItems, locationId, refresh]() {
        const auto rows = table->selectionModel()->selectedRows();
        if (rows.isEmpty()) return;
        const QString id = (*trashItems)[rows.first().row()].value("id").toString();
        QUrlQuery q;
        q.addQueryItem("location", locationId);
        m_api->postJson(QStringLiteral("/files/trash/%1/restore?%2").arg(id, q.toString(QUrl::FullyEncoded)), {},
                        [this, refresh](const ApiResult &result) {
            if (!result.ok) { Notify::error(this, tr("Restore failed: %1").arg(result.error)); return; }
            (*refresh)();
            reloadListing();
        });
    });

    auto *deleteBtn = new QPushButton(tr("Delete Permanently"), dialog);
    connect(deleteBtn, &QPushButton::clicked, dialog, [this, dialog, table, trashItems, locationId, refresh]() {
        const auto rows = table->selectionModel()->selectedRows();
        if (rows.isEmpty()) return;
        if (!Notify::confirm(dialog, tr("Delete Permanently"), tr("This cannot be undone. Continue?")))
            return;
        const QString id = (*trashItems)[rows.first().row()].value("id").toString();
        QUrlQuery q;
        q.addQueryItem("location", locationId);
        m_api->del(QStringLiteral("/files/trash/%1?%2").arg(id, q.toString(QUrl::FullyEncoded)),
                   [this, refresh](const ApiResult &result) {
            if (!result.ok) { Notify::error(this, tr("Delete failed: %1").arg(result.error)); return; }
            (*refresh)();
        });
    });

    auto *emptyBtn = new QPushButton(tr("Empty Trash"), dialog);
    connect(emptyBtn, &QPushButton::clicked, dialog, [this, dialog, locationId, refresh]() {
        if (!Notify::confirm(dialog, tr("Empty Trash"), tr("Permanently delete everything in trash?")))
            return;
        QUrlQuery q;
        q.addQueryItem("location", locationId);
        m_api->del(QStringLiteral("/files/trash?%1").arg(q.toString(QUrl::FullyEncoded)),
                   [this, refresh](const ApiResult &result) {
            if (!result.ok) { Notify::error(this, tr("Failed: %1").arg(result.error)); return; }
            (*refresh)();
        });
    });

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(restoreBtn);
    btnRow->addWidget(deleteBtn);
    btnRow->addStretch();
    btnRow->addWidget(emptyBtn);

    auto *closeButtons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(closeButtons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(closeButtons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);

    auto *layout = new QVBoxLayout(dialog);
    layout->addWidget(table);
    layout->addLayout(btnRow);
    layout->addWidget(closeButtons);

    (*refresh)();
    dialog->show();
}
