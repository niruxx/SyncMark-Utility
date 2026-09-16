#include "ContactsPage.h"
#include "ContactEditDialog.h"
#include "ContactGroupEditDialog.h"
#include "DuplicatesDialog.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"

#include <QTreeWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrlQuery>
#include <QJsonArray>
#include <QTimer>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QInputDialog>
#include <algorithm>

namespace {
constexpr int KindRole = Qt::UserRole + 1;
constexpr int GroupIdRole = Qt::UserRole + 2;
constexpr int GroupTypeRole = Qt::UserRole + 3;
enum NodeKind { KindAll, KindFavorites, KindGroup };

QString displayName(const QJsonObject &c) {
    QString name = c.value("full_name").toString();
    if (name.trimmed().isEmpty())
        name = (c.value("first_name").toString() + " " + c.value("last_name").toString()).trimmed();
    return name;
}
}

ContactsPage::ContactsPage(ApiClient *api, QWidget *parent) : PageWidget(parent), m_api(api) {
    m_sidebar = new QTreeWidget(this);
    m_sidebar->setHeaderHidden(true);
    m_sidebar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_sidebar, &QTreeWidget::itemSelectionChanged, this, &ContactsPage::onSidebarSelectionChanged);
    connect(m_sidebar, &QTreeWidget::customContextMenuRequested, this, &ContactsPage::onSidebarContextMenu);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search contacts..."));
    auto *searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(250);
    connect(m_search, &QLineEdit::textChanged, searchTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(searchTimer, &QTimer::timeout, this, &ContactsPage::applyClientFilter);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("★"), tr("Name"), tr("Organization"), tr("Email"), tr("Phone")});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 28);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(30);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &ContactsPage::onContextMenu);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) { editContactAt(row); });

    auto *addBtn = new QPushButton(tr("New Contact"), this);
    connect(addBtn, &QPushButton::clicked, this, &ContactsPage::addContact);
    auto *addGroupBtn = new QPushButton(tr("New Group"), this);
    connect(addGroupBtn, &QPushButton::clicked, this, &ContactsPage::addGroup);
    auto *duplicatesBtn = new QPushButton(tr("Find Duplicates..."), this);
    connect(duplicatesBtn, &QPushButton::clicked, this, &ContactsPage::openDuplicates);

    auto *bulkBtn = new QToolButton(this);
    bulkBtn->setText(tr("Bulk Actions"));
    bulkBtn->setPopupMode(QToolButton::InstantPopup);
    auto *bulkMenu = new QMenu(bulkBtn);
    connect(bulkMenu->addAction(tr("Favorite Selected")), &QAction::triggered, this, [this] { bulkAction("favorite"); });
    connect(bulkMenu->addAction(tr("Unfavorite Selected")), &QAction::triggered, this, [this] { bulkAction("unfavorite"); });
    bulkMenu->addSeparator();
    connect(bulkMenu->addAction(tr("Delete Selected")), &QAction::triggered, this, [this] { bulkAction("delete"); });
    bulkBtn->setMenu(bulkMenu);

    auto *importBtn = new QPushButton(tr("Import..."), this);
    connect(importBtn, &QPushButton::clicked, this, &ContactsPage::importContacts);

    auto *exportBtn = new QToolButton(this);
    exportBtn->setText(tr("Export"));
    exportBtn->setPopupMode(QToolButton::InstantPopup);
    auto *exportMenu = new QMenu(exportBtn);
    connect(exportMenu->addAction(tr("Export as CSV...")), &QAction::triggered, this, [this] { exportContacts("csv"); });
    connect(exportMenu->addAction(tr("Export as vCard...")), &QAction::triggered, this, [this] { exportContacts("vcf"); });
    exportBtn->setMenu(exportMenu);

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(addBtn);
    toolbar->addWidget(addGroupBtn);
    toolbar->addWidget(duplicatesBtn);
    toolbar->addWidget(bulkBtn);
    toolbar->addStretch();
    toolbar->addWidget(importBtn);
    toolbar->addWidget(exportBtn);

    auto *rightLayout = new QVBoxLayout;
    rightLayout->addWidget(m_search);
    rightLayout->addWidget(m_table);
    auto *rightWidget = new QWidget(this);
    rightWidget->setLayout(rightLayout);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(m_sidebar);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({190, 600});

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(splitter);
}

void ContactsPage::reload() {
    reloadGroups();
}

void ContactsPage::reloadGroups() {
    m_api->get("/contact-groups", {}, [this](const ApiResult &result) {
        m_groups.clear();
        if (result.ok && result.body.isArray())
            for (const QJsonValue &v : result.body.array()) m_groups << v.toObject();

        QTreeWidgetItem *previous = m_sidebar->currentItem();
        const QString previousGroupId = previous ? previous->data(0, GroupIdRole).toString() : QString();
        const int previousKind = previous ? previous->data(0, KindRole).toInt() : KindAll;

        m_sidebar->clear();
        auto *allItem = new QTreeWidgetItem(m_sidebar, {tr("All Contacts")});
        allItem->setData(0, KindRole, KindAll);
        auto *favItem = new QTreeWidgetItem(m_sidebar, {tr("★ Favorites")});
        favItem->setData(0, KindRole, KindFavorites);

        QTreeWidgetItem *toSelect = allItem;
        if (!m_groups.isEmpty()) {
            auto *groupsHeader = new QTreeWidgetItem(m_sidebar, {tr("Groups")});
            groupsHeader->setFlags(Qt::ItemIsEnabled);
            QFont f = groupsHeader->font(0);
            f.setBold(true);
            groupsHeader->setFont(0, f);
            for (const QJsonObject &g : m_groups) {
                const QString label = (g.value("type").toString() == "smart" ? QStringLiteral("⚙ ") : QString())
                                       + g.value("name").toString();
                auto *item = new QTreeWidgetItem(groupsHeader, {label});
                item->setData(0, KindRole, KindGroup);
                item->setData(0, GroupIdRole, g.value("id").toVariant().toString());
                item->setData(0, GroupTypeRole, g.value("type").toString());
                if (previousKind == KindGroup && previousGroupId == g.value("id").toVariant().toString())
                    toSelect = item;
            }
            m_sidebar->expandAll();
        }
        if (previousKind == KindFavorites) toSelect = favItem;
        m_sidebar->setCurrentItem(toSelect);
    });
}

QString ContactsPage::currentGroupId() const {
    QTreeWidgetItem *item = m_sidebar->currentItem();
    if (!item || item->data(0, KindRole).toInt() != KindGroup) return {};
    return item->data(0, GroupIdRole).toString();
}

bool ContactsPage::currentGroupIsManual() const {
    QTreeWidgetItem *item = m_sidebar->currentItem();
    if (!item || item->data(0, KindRole).toInt() != KindGroup) return false;
    return item->data(0, GroupTypeRole).toString() != "smart";
}

void ContactsPage::onSidebarSelectionChanged() {
    reloadContacts();
}

void ContactsPage::reloadContacts() {
    QTreeWidgetItem *item = m_sidebar->currentItem();
    const int kind = item ? item->data(0, KindRole).toInt() : KindAll;

    if (kind == KindGroup) {
        const QString groupId = currentGroupId();
        m_api->get(QStringLiteral("/contact-groups/%1/contacts").arg(groupId), {}, [this](const ApiResult &result) {
            if (!result.ok || !result.body.isArray()) return;
            m_contacts.clear();
            for (const QJsonValue &v : result.body.array()) m_contacts << v.toObject();
            applyClientFilter();
        });
        return;
    }

    QUrlQuery query;
    if (kind == KindFavorites) query.addQueryItem("favorite", "1");
    m_api->get("/contacts", query, [this](const ApiResult &result) {
        if (!result.ok || !result.body.isArray()) {
            if (!result.ok) Notify::error(this, tr("Failed to load contacts: %1").arg(result.error));
            return;
        }
        m_contacts.clear();
        for (const QJsonValue &v : result.body.array()) m_contacts << v.toObject();
        applyClientFilter();
    });
}

void ContactsPage::applyClientFilter() {
    const QString needle = m_search->text().trimmed();
    if (needle.isEmpty()) {
        populateTable(m_contacts);
        return;
    }
    QVector<QJsonObject> filtered;
    for (const QJsonObject &c : m_contacts) {
        QStringList haystack{displayName(c), c.value("organization").toString(), c.value("notes").toString()};
        for (const QJsonValue &v : c.value("emails").toArray()) haystack << v.toObject().value("value").toString();
        for (const QJsonValue &v : c.value("tags").toArray()) haystack << v.toString();
        bool match = false;
        for (const QString &h : haystack)
            if (h.contains(needle, Qt::CaseInsensitive)) { match = true; break; }
        if (match) filtered << c;
    }
    populateTable(filtered);
}

void ContactsPage::populateTable(const QVector<QJsonObject> &contacts) {
    m_filtered = contacts;
    m_table->setRowCount(m_filtered.size());
    for (int row = 0; row < m_filtered.size(); ++row) {
        const QJsonObject &c = m_filtered[row];
        const bool favorite = c.value("favorite").toInt() != 0 || c.value("favorite").toBool();
        auto *favItem = new QTableWidgetItem(favorite ? QStringLiteral("★") : QString());
        favItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 0, favItem);
        m_table->setItem(row, 1, new QTableWidgetItem(displayName(c)));
        m_table->setItem(row, 2, new QTableWidgetItem(c.value("organization").toString()));

        QString email;
        const QJsonArray emails = c.value("emails").toArray();
        if (!emails.isEmpty()) email = emails.first().toObject().value("value").toString();
        m_table->setItem(row, 3, new QTableWidgetItem(email));

        QString phone;
        const QJsonArray phones = c.value("phones").toArray();
        if (!phones.isEmpty()) phone = phones.first().toObject().value("value").toString();
        m_table->setItem(row, 4, new QTableWidgetItem(phone));
    }
}

QVector<int> ContactsPage::selectedRows() const {
    QVector<int> rows;
    for (const auto &idx : m_table->selectionModel()->selectedRows())
        rows << idx.row();
    std::sort(rows.begin(), rows.end());
    return rows;
}

QVector<qlonglong> ContactsPage::selectedContactIds() const {
    QVector<qlonglong> ids;
    for (int row : selectedRows())
        if (row >= 0 && row < m_filtered.size())
            ids << m_filtered[row].value("id").toVariant().toLongLong();
    return ids;
}

void ContactsPage::onSidebarContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = m_sidebar->itemAt(pos);
    QMenu menu(this);
    QAction *newGroupAction = menu.addAction(tr("New Group..."));
    QAction *editAction = nullptr;
    QAction *deleteAction = nullptr;
    if (item && item->data(0, KindRole).toInt() == KindGroup) {
        editAction = menu.addAction(tr("Edit Group..."));
        deleteAction = menu.addAction(tr("Delete Group"));
    }
    QAction *chosen = menu.exec(m_sidebar->viewport()->mapToGlobal(pos));
    if (chosen == newGroupAction) addGroup();
    else if (chosen == editAction) editGroup(item);
    else if (chosen == deleteAction) deleteGroup(item);
}

void ContactsPage::onContextMenu(const QPoint &pos) {
    if (m_table->itemAt(pos)) {
        if (!m_table->selectionModel()->isRowSelected(m_table->itemAt(pos)->row(), {}))
            m_table->selectRow(m_table->itemAt(pos)->row());
    }
    if (selectedRows().isEmpty()) return;
    const bool singleSelection = selectedRows().size() == 1;

    QMenu menu(this);
    QAction *editAction = singleSelection ? menu.addAction(tr("Edit...")) : nullptr;
    QAction *favAction = menu.addAction(tr("Toggle Favorite"));
    QAction *photoAction = singleSelection ? menu.addAction(tr("Set Photo...")) : nullptr;
    QAction *removePhotoAction = singleSelection ? menu.addAction(tr("Remove Photo")) : nullptr;
    menu.addSeparator();

    QMenu *addToGroupMenu = menu.addMenu(tr("Add to Group"));
    QVector<QJsonObject> manualGroups;
    for (const QJsonObject &g : m_groups)
        if (g.value("type").toString() != "smart") manualGroups << g;
    for (const QJsonObject &g : manualGroups)
        connect(addToGroupMenu->addAction(g.value("name").toString()), &QAction::triggered, this,
                [this, g] { addSelectedToGroup(g); });
    addToGroupMenu->setEnabled(!manualGroups.isEmpty());

    QAction *removeFromGroupAction = nullptr;
    if (currentGroupIsManual())
        removeFromGroupAction = menu.addAction(tr("Remove from This Group"));

    menu.addSeparator();
    QAction *deleteAction = menu.addAction(tr("Delete"));

    QAction *chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (chosen == editAction) editSelectedContact();
    else if (chosen == favAction) toggleSelectedFavorite();
    else if (chosen == photoAction) setPhotoForSelected();
    else if (chosen == removePhotoAction) removePhotoForSelected();
    else if (chosen == removeFromGroupAction) removeSelectedFromCurrentGroup();
    else if (chosen == deleteAction) deleteSelectedContacts();
}

void ContactsPage::addContact() {
    ContactEditDialog dialog(m_contacts, this);
    if (dialog.exec() != QDialog::Accepted) return;
    m_api->postJson("/contacts", dialog.formData(), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not create contact: %1").arg(result.error)); return; }
        reloadContacts();
    });
}

void ContactsPage::editContactAt(int row) {
    if (row < 0 || row >= m_filtered.size()) return;
    ContactEditDialog dialog(m_contacts, this);
    dialog.setContact(m_filtered[row]);
    if (dialog.exec() != QDialog::Accepted) return;
    const QString id = QString::number(m_filtered[row].value("id").toVariant().toLongLong());
    m_api->putJson("/contacts/" + id, dialog.formData(), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not update contact: %1").arg(result.error)); return; }
        reloadContacts();
    });
}

void ContactsPage::editSelectedContact() {
    const auto rows = selectedRows();
    if (!rows.isEmpty()) editContactAt(rows.first());
}

void ContactsPage::deleteSelectedContacts() {
    const auto ids = selectedContactIds();
    if (ids.isEmpty()) return;
    if (!Notify::confirm(this, tr("Delete Contact(s)"), tr("Delete %1 contact(s)?").arg(ids.size()))) return;

    if (ids.size() == 1) {
        m_api->del("/contacts/" + QString::number(ids.first()), [this](const ApiResult &result) {
            if (!result.ok) { Notify::error(this, tr("Could not delete contact: %1").arg(result.error)); return; }
            reloadContacts();
        });
        return;
    }
    bulkAction("delete");
}

void ContactsPage::toggleSelectedFavorite() {
    const auto rows = selectedRows();
    if (rows.size() == 1) {
        const QJsonObject &c = m_filtered[rows.first()];
        const bool favorite = c.value("favorite").toInt() != 0 || c.value("favorite").toBool();
        const QString id = QString::number(c.value("id").toVariant().toLongLong());
        m_api->putJson("/contacts/" + id + "/favorite", {{"favorite", !favorite}}, [this](const ApiResult &result) {
            if (!result.ok) { Notify::error(this, tr("Could not update favorite: %1").arg(result.error)); return; }
            reloadContacts();
        });
        return;
    }
    bulkAction("favorite");
}

void ContactsPage::bulkAction(const QString &action) {
    const auto ids = selectedContactIds();
    if (ids.isEmpty()) return;
    if (action == "delete" && !Notify::confirm(this, tr("Delete Contacts"), tr("Delete %1 contact(s)?").arg(ids.size())))
        return;

    QJsonArray idArr;
    for (qlonglong id : ids) idArr.append(id);
    m_api->postJson("/contacts/bulk", {{"ids", idArr}, {"action", action}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Bulk action failed: %1").arg(result.error)); return; }
        reloadContacts();
    });
}

void ContactsPage::setPhotoForSelected() {
    const auto rows = selectedRows();
    if (rows.size() != 1) return;
    const QString path = QFileDialog::getOpenFileName(this, tr("Select Photo"), {},
        tr("Images (*.png *.jpg *.jpeg *.gif *.webp)"));
    if (path.isEmpty()) return;

    auto *file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) { Notify::error(this, tr("Could not open file.")); delete file; return; }

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QVariant(QStringLiteral("form-data; name=\"photo\"; filename=\"%1\"").arg(QFileInfo(path).fileName())));
    file->setParent(multiPart);
    part.setBodyDevice(file);
    multiPart->append(part);

    const QString id = QString::number(m_filtered[rows.first()].value("id").toVariant().toLongLong());
    m_api->postMultipart("/contacts/" + id + "/photo", multiPart, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not upload photo: %1").arg(result.error)); return; }
        reloadContacts();
    });
}

void ContactsPage::removePhotoForSelected() {
    const auto rows = selectedRows();
    if (rows.size() != 1) return;
    const QString id = QString::number(m_filtered[rows.first()].value("id").toVariant().toLongLong());
    m_api->del("/contacts/" + id + "/photo", [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not remove photo: %1").arg(result.error)); return; }
        reloadContacts();
    });
}

void ContactsPage::importContacts() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Import Contacts"), {},
        tr("Contacts (*.csv *.vcf);;All files (*)"));
    if (path.isEmpty()) return;

    auto *file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) { Notify::error(this, tr("Could not open file.")); delete file; return; }

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QVariant(QStringLiteral("form-data; name=\"file\"; filename=\"%1\"").arg(QFileInfo(path).fileName())));
    file->setParent(multiPart);
    part.setBodyDevice(file);
    multiPart->append(part);

    m_api->postMultipart("/contacts/import", multiPart, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Import failed: %1").arg(result.error)); return; }
        reloadContacts();
    });
}

void ContactsPage::exportContacts(const QString &format) {
    QUrlQuery query;
    query.addQueryItem("format", format);
    m_api->getRaw("/contacts/export", query, [this, format](const ApiRawResult &result) {
        if (!result.ok) { Notify::error(this, tr("Export failed: %1").arg(result.error)); return; }
        const QString defaultName = QStringLiteral("syncmark-contacts.%1").arg(format);
        const QString path = QFileDialog::getSaveFileName(this, tr("Save Export"), defaultName);
        if (path.isEmpty()) return;
        QFile out(path);
        if (!out.open(QIODevice::WriteOnly)) { Notify::error(this, tr("Could not write file.")); return; }
        out.write(result.data);
    });
}

void ContactsPage::addGroup() {
    ContactGroupEditDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    QJsonObject body{{"name", dialog.name()}, {"type", dialog.type()}};
    if (dialog.type() == "smart") body["smartRules"] = dialog.smartRules();
    m_api->postJson("/contact-groups", body, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not create group: %1").arg(result.error)); return; }
        reloadGroups();
    });
}

void ContactsPage::editGroup(QTreeWidgetItem *item) {
    if (!item) return;
    const QString id = item->data(0, GroupIdRole).toString();
    const auto it = std::find_if(m_groups.begin(), m_groups.end(), [&id](const QJsonObject &g) {
        return g.value("id").toVariant().toString() == id;
    });
    if (it == m_groups.end()) return;

    ContactGroupEditDialog dialog(this);
    dialog.setGroup(*it);
    if (dialog.exec() != QDialog::Accepted) return;
    QJsonObject body{{"name", dialog.name()}};
    if (dialog.type() == "smart") body["smartRules"] = dialog.smartRules();
    m_api->putJson("/contact-groups/" + id, body, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not update group: %1").arg(result.error)); return; }
        reloadGroups();
    });
}

void ContactsPage::deleteGroup(QTreeWidgetItem *item) {
    if (!item) return;
    if (!Notify::confirm(this, tr("Delete Group"), tr("Delete this group? Contacts themselves are not deleted."))) return;
    const QString id = item->data(0, GroupIdRole).toString();
    m_api->del("/contact-groups/" + id, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not delete group: %1").arg(result.error)); return; }
        reloadGroups();
    });
}

void ContactsPage::addSelectedToGroup(const QJsonObject &group) {
    const auto ids = selectedContactIds();
    const QString groupId = group.value("id").toVariant().toString();
    for (qlonglong id : ids) {
        m_api->postJson(QStringLiteral("/contact-groups/%1/members/%2").arg(groupId).arg(id), {},
                         [this](const ApiResult &result) {
            if (!result.ok) Notify::error(this, tr("Could not add to group: %1").arg(result.error));
        });
    }
}

void ContactsPage::removeSelectedFromCurrentGroup() {
    const QString groupId = currentGroupId();
    if (groupId.isEmpty()) return;
    const auto ids = selectedContactIds();
    for (qlonglong id : ids) {
        m_api->del(QStringLiteral("/contact-groups/%1/members/%2").arg(groupId).arg(id), [this](const ApiResult &result) {
            if (!result.ok) { Notify::error(this, tr("Could not remove from group: %1").arg(result.error)); return; }
            reloadContacts();
        });
    }
}

void ContactsPage::openDuplicates() {
    auto *dialog = new DuplicatesDialog(m_api, this);
    connect(dialog, &DuplicatesDialog::contactsChanged, this, &ContactsPage::reloadContacts);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}
