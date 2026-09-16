#include "ContactsPage.h"
#include "ContactEditDialog.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
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

ContactsPage::ContactsPage(ApiClient *api, QWidget *parent) : PageWidget(parent), m_api(api) {
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search contacts..."));
    auto *searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(300);
    connect(m_search, &QLineEdit::textChanged, searchTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(searchTimer, &QTimer::timeout, this, &ContactsPage::reloadContacts);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("★"), tr("Name"), tr("Organization"), tr("Email"), tr("Phone")});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 28);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &ContactsPage::onContextMenu);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) { editContactAt(row); });

    auto *addBtn = new QPushButton(tr("New Contact"), this);
    connect(addBtn, &QPushButton::clicked, this, &ContactsPage::addContact);

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
    toolbar->addStretch();
    toolbar->addWidget(importBtn);
    toolbar->addWidget(exportBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_search);
    layout->addWidget(m_table);
}

void ContactsPage::reload() { reloadContacts(); }

void ContactsPage::reloadContacts() {
    QUrlQuery query;
    if (!m_search->text().trimmed().isEmpty())
        query.addQueryItem("q", m_search->text().trimmed());

    m_api->get("/contacts", query, [this](const ApiResult &result) {
        if (!result.ok || !result.body.isArray()) {
            if (!result.ok) Notify::error(this, tr("Failed to load contacts: %1").arg(result.error));
            return;
        }
        m_contacts.clear();
        for (const QJsonValue &v : result.body.array()) m_contacts << v.toObject();

        m_table->setRowCount(m_contacts.size());
        for (int row = 0; row < m_contacts.size(); ++row) {
            const QJsonObject &c = m_contacts[row];
            const bool favorite = c.value("favorite").toInt() != 0 || c.value("favorite").toBool();
            auto *favItem = new QTableWidgetItem(favorite ? QStringLiteral("★") : QString());
            favItem->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, 0, favItem);

            QString name = c.value("full_name").toString();
            if (name.isEmpty())
                name = (c.value("first_name").toString() + " " + c.value("last_name").toString()).trimmed();
            m_table->setItem(row, 1, new QTableWidgetItem(name));
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
    });
}

int ContactsPage::selectedRow() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

void ContactsPage::onContextMenu(const QPoint &pos) {
    if (m_table->itemAt(pos)) m_table->selectRow(m_table->itemAt(pos)->row());
    if (selectedRow() < 0) return;
    QMenu menu(this);
    QAction *editAction = menu.addAction(tr("Edit..."));
    QAction *favAction = menu.addAction(tr("Toggle Favorite"));
    QAction *photoAction = menu.addAction(tr("Set Photo..."));
    QAction *removePhotoAction = menu.addAction(tr("Remove Photo"));
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(tr("Delete"));
    QAction *chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (chosen == editAction) editSelectedContact();
    else if (chosen == favAction) toggleSelectedFavorite();
    else if (chosen == photoAction) setPhotoForSelected();
    else if (chosen == removePhotoAction) removePhotoForSelected();
    else if (chosen == deleteAction) deleteSelectedContact();
}

void ContactsPage::addContact() {
    ContactEditDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    m_api->postJson("/contacts", dialog.formData(), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not create contact: %1").arg(result.error)); return; }
        reloadContacts();
    });
}

void ContactsPage::editContactAt(int row) {
    if (row < 0 || row >= m_contacts.size()) return;
    ContactEditDialog dialog(this);
    dialog.setContact(m_contacts[row]);
    if (dialog.exec() != QDialog::Accepted) return;
    const QString id = QString::number(m_contacts[row].value("id").toVariant().toLongLong());
    m_api->putJson("/contacts/" + id, dialog.formData(), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not update contact: %1").arg(result.error)); return; }
        reloadContacts();
    });
}

void ContactsPage::editSelectedContact() { editContactAt(selectedRow()); }

void ContactsPage::deleteSelectedContact() {
    int row = selectedRow();
    if (row < 0) return;
    if (!Notify::confirm(this, tr("Delete Contact"), tr("Delete this contact?"))) return;
    const QString id = QString::number(m_contacts[row].value("id").toVariant().toLongLong());
    m_api->del("/contacts/" + id, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not delete contact: %1").arg(result.error)); return; }
        reloadContacts();
    });
}

void ContactsPage::toggleSelectedFavorite() {
    int row = selectedRow();
    if (row < 0) return;
    const QJsonObject &c = m_contacts[row];
    const bool favorite = c.value("favorite").toInt() != 0 || c.value("favorite").toBool();
    const QString id = QString::number(c.value("id").toVariant().toLongLong());
    m_api->putJson("/contacts/" + id + "/favorite", {{"favorite", !favorite}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not update favorite: %1").arg(result.error)); return; }
        reloadContacts();
    });
}

void ContactsPage::setPhotoForSelected() {
    int row = selectedRow();
    if (row < 0) return;
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

    const QString id = QString::number(m_contacts[row].value("id").toVariant().toLongLong());
    m_api->postMultipart("/contacts/" + id + "/photo", multiPart, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not upload photo: %1").arg(result.error)); return; }
        reloadContacts();
    });
}

void ContactsPage::removePhotoForSelected() {
    int row = selectedRow();
    if (row < 0) return;
    const QString id = QString::number(m_contacts[row].value("id").toVariant().toLongLong());
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
