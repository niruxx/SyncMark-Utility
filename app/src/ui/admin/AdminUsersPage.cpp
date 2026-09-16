#include "AdminUsersPage.h"
#include "UserEditDialog.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>

AdminUsersPage::AdminUsersPage(ApiClient *api, QWidget *parent) : PageWidget(parent), m_api(api) {
    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("Username"), tr("Status"), tr("Items")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(30);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &AdminUsersPage::onContextMenu);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int, int) { editSelectedUser(); });

    auto *addBtn = new QPushButton(tr("New User"), this);
    connect(addBtn, &QPushButton::clicked, this, &AdminUsersPage::addUser);

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(addBtn);
    toolbar->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);
}

void AdminUsersPage::reload() { reloadUsers(); }

void AdminUsersPage::reloadUsers() {
    m_api->get("/admin/users", {}, [this](const ApiResult &result) {
        if (!result.ok || !result.body.isArray()) {
            if (!result.ok) Notify::error(this, tr("Failed to load users: %1").arg(result.error));
            return;
        }
        m_users.clear();
        for (const QJsonValue &v : result.body.array()) m_users << v.toObject();

        m_table->setRowCount(m_users.size());
        for (int row = 0; row < m_users.size(); ++row) {
            const QJsonObject &u = m_users[row];
            m_table->setItem(row, 0, new QTableWidgetItem(u.value("username").toString()));
            m_table->setItem(row, 1, new QTableWidgetItem(u.value("enabled").toBool() ? tr("Enabled") : tr("Disabled")));

            const QJsonObject counts = u.value("counts").toObject();
            QStringList parts;
            for (const char *key : {"bookmarks", "contacts", "events", "passwords", "fileLocations"}) {
                int n = counts.value(key).toInt();
                if (n > 0) parts << QStringLiteral("%1 %2").arg(n).arg(key);
            }
            m_table->setItem(row, 2, new QTableWidgetItem(parts.join(", ")));
        }
    });
}

int AdminUsersPage::selectedRow() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

void AdminUsersPage::onContextMenu(const QPoint &pos) {
    if (m_table->itemAt(pos)) m_table->selectRow(m_table->itemAt(pos)->row());
    if (selectedRow() < 0) return;
    QMenu menu(this);
    QAction *editAction = menu.addAction(tr("Edit..."));
    QAction *toggleAction = menu.addAction(tr("Toggle Enabled"));
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(tr("Delete User"));
    QAction *chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (chosen == editAction) editSelectedUser();
    else if (chosen == toggleAction) toggleSelectedEnabled();
    else if (chosen == deleteAction) deleteSelectedUser();
}

void AdminUsersPage::addUser() {
    UserEditDialog dialog(true, this);
    if (dialog.exec() != QDialog::Accepted) return;
    m_api->postJson("/admin/users", {{"username", dialog.username()}, {"password", dialog.password()}},
                     [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not create user: %1").arg(result.error)); return; }
        reloadUsers();
    });
}

void AdminUsersPage::editSelectedUser() {
    int row = selectedRow();
    if (row < 0) return;
    const QString id = QString::number(m_users[row].value("id").toVariant().toLongLong());

    UserEditDialog dialog(false, this);
    dialog.setUsername(m_users[row].value("username").toString());
    if (dialog.exec() != QDialog::Accepted) return;

    QJsonObject body{{"username", dialog.username()}};
    if (!dialog.password().isEmpty()) body["password"] = dialog.password();
    m_api->putJson("/admin/users/" + id, body, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not update user: %1").arg(result.error)); return; }
        reloadUsers();
    });
}

void AdminUsersPage::toggleSelectedEnabled() {
    int row = selectedRow();
    if (row < 0) return;
    const QString id = QString::number(m_users[row].value("id").toVariant().toLongLong());
    const bool enabled = m_users[row].value("enabled").toBool();
    m_api->putJson("/admin/users/" + id + "/enabled", {{"enabled", !enabled}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not update user: %1").arg(result.error)); return; }
        reloadUsers();
    });
}

void AdminUsersPage::deleteSelectedUser() {
    int row = selectedRow();
    if (row < 0) return;
    if (!Notify::confirm(this, tr("Delete User"),
                          tr("Permanently delete \"%1\" and all their data?").arg(m_users[row].value("username").toString())))
        return;
    const QString id = QString::number(m_users[row].value("id").toVariant().toLongLong());
    m_api->del("/admin/users/" + id, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not delete user: %1").arg(result.error)); return; }
        reloadUsers();
    });
}
