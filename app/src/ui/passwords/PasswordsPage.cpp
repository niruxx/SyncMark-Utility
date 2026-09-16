#include "PasswordsPage.h"
#include "PasswordEditDialog.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrlQuery>
#include <QJsonArray>
#include <QTimer>
#include <QClipboard>
#include <QGuiApplication>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>

PasswordsPage::PasswordsPage(ApiClient *api, QWidget *parent) : PageWidget(parent), m_api(api) {
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search vault..."));
    auto *searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(300);
    connect(m_search, &QLineEdit::textChanged, searchTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(searchTimer, &QTimer::timeout, this, &PasswordsPage::reloadEntries);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("★"), tr("Site"), tr("Username"), tr("URL")});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 28);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &PasswordsPage::onContextMenu);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int, int) { editSelectedEntry(); });

    auto *addBtn = new QPushButton(tr("New Entry"), this);
    connect(addBtn, &QPushButton::clicked, this, &PasswordsPage::addEntry);
    auto *importBtn = new QPushButton(tr("Import CSV..."), this);
    connect(importBtn, &QPushButton::clicked, this, &PasswordsPage::importEntries);
    auto *exportBtn = new QPushButton(tr("Export CSV..."), this);
    connect(exportBtn, &QPushButton::clicked, this, &PasswordsPage::exportEntries);

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(addBtn);
    toolbar->addStretch();
    toolbar->addWidget(importBtn);
    toolbar->addWidget(exportBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_search);
    layout->addWidget(m_table);
    layout->addWidget(new QLabel(tr("Tip: right-click an entry to copy its password or username."), this));
}

void PasswordsPage::reload() { reloadEntries(); }

void PasswordsPage::reloadEntries() {
    QUrlQuery query;
    if (!m_search->text().trimmed().isEmpty())
        query.addQueryItem("q", m_search->text().trimmed());

    m_api->get("/passwords", query, [this](const ApiResult &result) {
        if (!result.ok || !result.body.isArray()) {
            if (!result.ok) Notify::error(this, tr("Failed to load vault: %1").arg(result.error));
            return;
        }
        m_entries.clear();
        for (const QJsonValue &v : result.body.array()) m_entries << v.toObject();

        m_table->setRowCount(m_entries.size());
        for (int row = 0; row < m_entries.size(); ++row) {
            const QJsonObject &e = m_entries[row];
            const bool favorite = e.value("favorite").toInt() != 0 || e.value("favorite").toBool();
            auto *favItem = new QTableWidgetItem(favorite ? QStringLiteral("★") : QString());
            favItem->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, 0, favItem);
            m_table->setItem(row, 1, new QTableWidgetItem(e.value("site_name").toString()));
            m_table->setItem(row, 2, new QTableWidgetItem(e.value("username").toString()));
            m_table->setItem(row, 3, new QTableWidgetItem(e.value("url").toString()));
        }
    });
}

int PasswordsPage::selectedRow() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

qlonglong PasswordsPage::idAt(int row) const {
    if (row < 0 || row >= m_entries.size()) return -1;
    return m_entries[row].value("id").toVariant().toLongLong();
}

void PasswordsPage::onContextMenu(const QPoint &pos) {
    if (m_table->itemAt(pos)) m_table->selectRow(m_table->itemAt(pos)->row());
    if (selectedRow() < 0) return;
    QMenu menu(this);
    QAction *copyPass = menu.addAction(tr("Copy Password"));
    QAction *copyUser = menu.addAction(tr("Copy Username"));
    menu.addSeparator();
    QAction *editAction = menu.addAction(tr("Edit..."));
    QAction *favAction = menu.addAction(tr("Toggle Favorite"));
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(tr("Delete"));
    QAction *chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (chosen == copyPass) copySelectedPassword();
    else if (chosen == copyUser) copySelectedUsername();
    else if (chosen == editAction) editSelectedEntry();
    else if (chosen == favAction) toggleSelectedFavorite();
    else if (chosen == deleteAction) deleteSelectedEntry();
}

void PasswordsPage::addEntry() {
    PasswordEditDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    m_api->postJson("/passwords", dialog.formData(), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not create entry: %1").arg(result.error)); return; }
        reloadEntries();
    });
}

void PasswordsPage::editSelectedEntry() {
    int row = selectedRow();
    if (row < 0) return;
    const qlonglong id = idAt(row);

    m_api->get(QStringLiteral("/passwords/%1").arg(id), {}, [this, id](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not open entry: %1").arg(result.error)); return; }
        PasswordEditDialog dialog(this);
        dialog.setEntry(result.body.object());
        if (dialog.exec() != QDialog::Accepted) return;
        m_api->putJson(QStringLiteral("/passwords/%1").arg(id), dialog.formData(), [this](const ApiResult &r2) {
            if (!r2.ok) { Notify::error(this, tr("Could not update entry: %1").arg(r2.error)); return; }
            reloadEntries();
        });
    });
}

void PasswordsPage::deleteSelectedEntry() {
    int row = selectedRow();
    if (row < 0) return;
    if (!Notify::confirm(this, tr("Delete Entry"),
                          tr("Delete \"%1\"?").arg(m_entries[row].value("site_name").toString())))
        return;
    m_api->del(QStringLiteral("/passwords/%1").arg(idAt(row)), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not delete entry: %1").arg(result.error)); return; }
        reloadEntries();
    });
}

void PasswordsPage::toggleSelectedFavorite() {
    int row = selectedRow();
    if (row < 0) return;
    const QJsonObject &e = m_entries[row];
    const bool favorite = e.value("favorite").toInt() != 0 || e.value("favorite").toBool();
    m_api->putJson(QStringLiteral("/passwords/%1/favorite").arg(idAt(row)), {{"favorite", !favorite}},
                   [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not update favorite: %1").arg(result.error)); return; }
        reloadEntries();
    });
}

void PasswordsPage::copySelectedPassword() {
    int row = selectedRow();
    if (row < 0) return;
    m_api->get(QStringLiteral("/passwords/%1/reveal").arg(idAt(row)), {}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not reveal password: %1").arg(result.error)); return; }
        QGuiApplication::clipboard()->setText(result.body.object().value("password").toString());
    });
}

void PasswordsPage::copySelectedUsername() {
    int row = selectedRow();
    if (row < 0) return;
    QGuiApplication::clipboard()->setText(m_entries[row].value("username").toString());
}

void PasswordsPage::importEntries() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Import Passwords"), {}, tr("CSV (*.csv)"));
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

    m_api->postMultipart("/passwords/import", multiPart, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Import failed: %1").arg(result.error)); return; }
        reloadEntries();
    });
}

void PasswordsPage::exportEntries() {
    if (!Notify::confirm(this, tr("Export Vault"),
                          tr("This will save all passwords as plaintext CSV to a file on disk. Continue?")))
        return;
    m_api->getRaw("/passwords/export", {}, [this](const ApiRawResult &result) {
        if (!result.ok) { Notify::error(this, tr("Export failed: %1").arg(result.error)); return; }
        const QString path = QFileDialog::getSaveFileName(this, tr("Save Export"), "syncmark-passwords.csv");
        if (path.isEmpty()) return;
        QFile out(path);
        if (!out.open(QIODevice::WriteOnly)) { Notify::error(this, tr("Could not write file.")); return; }
        out.write(result.data);
    });
}
