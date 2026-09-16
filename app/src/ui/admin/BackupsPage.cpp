#include "BackupsPage.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QFile>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QInputDialog>
#include <QUrl>

QStringList BackupsPage::pickModules(const QString &title, const QStringList &preset) {
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    QVector<QCheckBox *> boxes;
    auto *layout = new QVBoxLayout(&dialog);
    for (const QString &m : preset) {
        auto *cb = new QCheckBox(m, &dialog);
        cb->setChecked(true);
        boxes << cb;
        layout->addWidget(cb);
    }
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) return {};
    QStringList selected;
    for (auto *cb : boxes)
        if (cb->isChecked()) selected << cb->text();
    return selected;
}

BackupsPage::BackupsPage(ApiClient *api, QWidget *parent) : PageWidget(parent), m_api(api) {
    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({tr("File"), tr("Modules")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(30);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &BackupsPage::onContextMenu);

    auto *runBtn = new QPushButton(tr("Run Backup Now..."), this);
    connect(runBtn, &QPushButton::clicked, this, &BackupsPage::runBackupNow);
    auto *scheduleBtn = new QPushButton(tr("Schedule..."), this);
    connect(scheduleBtn, &QPushButton::clicked, this, &BackupsPage::openSchedule);

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(runBtn);
    toolbar->addWidget(scheduleBtn);
    toolbar->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);
}

void BackupsPage::reload() { reloadBackups(); }

void BackupsPage::reloadBackups() {
    m_api->get("/backups", {}, [this](const ApiResult &result) {
        if (!result.ok || !result.body.isArray()) {
            if (!result.ok) Notify::error(this, tr("Failed to load backups: %1").arg(result.error));
            return;
        }
        m_backups.clear();
        for (const QJsonValue &v : result.body.array()) m_backups << v.toObject();

        m_table->setRowCount(m_backups.size());
        for (int row = 0; row < m_backups.size(); ++row) {
            const QJsonObject &b = m_backups[row];
            m_table->setItem(row, 0, new QTableWidgetItem(b.value("name").toString()));
            QStringList mods;
            for (const QJsonValue &m : b.value("modules").toArray()) mods << m.toString();
            m_table->setItem(row, 1, new QTableWidgetItem(mods.join(", ")));
        }
    });
}

int BackupsPage::selectedRow() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

void BackupsPage::onContextMenu(const QPoint &pos) {
    if (m_table->itemAt(pos)) m_table->selectRow(m_table->itemAt(pos)->row());
    if (selectedRow() < 0) return;
    QMenu menu(this);
    QAction *downloadAction = menu.addAction(tr("Download..."));
    QAction *restoreAction = menu.addAction(tr("Restore..."));
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(tr("Delete"));
    QAction *chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (chosen == downloadAction) downloadSelected();
    else if (chosen == restoreAction) restoreSelected();
    else if (chosen == deleteAction) deleteSelected();
}

void BackupsPage::runBackupNow() {
    const QStringList modules = pickModules(tr("Run Backup"));
    if (modules.isEmpty()) return;
    QJsonArray arr;
    for (const QString &m : modules) arr.append(m);
    m_api->postJson("/backups/run", {{"modules", arr}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Backup failed: %1").arg(result.error)); return; }
        reloadBackups();
    });
}

void BackupsPage::downloadSelected() {
    int row = selectedRow();
    if (row < 0) return;
    const QString name = m_backups[row].value("name").toString();
    m_api->getRaw(QStringLiteral("/backups/%1/download").arg(QUrl::toPercentEncoding(name)), {},
                  [this, name](const ApiRawResult &result) {
        if (!result.ok) { Notify::error(this, tr("Download failed: %1").arg(result.error)); return; }
        const QString path = QFileDialog::getSaveFileName(this, tr("Save Backup"), name);
        if (path.isEmpty()) return;
        QFile out(path);
        if (!out.open(QIODevice::WriteOnly)) { Notify::error(this, tr("Could not write file.")); return; }
        out.write(result.data);
    });
}

void BackupsPage::deleteSelected() {
    int row = selectedRow();
    if (row < 0) return;
    const QString name = m_backups[row].value("name").toString();
    if (!Notify::confirm(this, tr("Delete Backup"), tr("Delete backup \"%1\"?").arg(name))) return;
    m_api->del(QStringLiteral("/backups/%1").arg(QUrl::toPercentEncoding(name)), [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not delete backup: %1").arg(result.error)); return; }
        reloadBackups();
    });
}

void BackupsPage::restoreSelected() {
    int row = selectedRow();
    if (row < 0) return;
    const QString name = m_backups[row].value("name").toString();

    if (!Notify::confirm(this, tr("Restore Backup"),
                          tr("Restoring \"%1\" will overwrite current data for the selected modules. Continue?").arg(name)))
        return;

    const QStringList modules = pickModules(tr("Restore Modules"));
    if (modules.isEmpty()) return;

    bool ok = false;
    const QString password = QInputDialog::getText(this, tr("Confirm Password"),
                                                     tr("Enter your account password to confirm restore:"),
                                                     QLineEdit::Password, {}, &ok);
    if (!ok || password.isEmpty()) return;

    QJsonArray arr;
    for (const QString &m : modules) arr.append(m);
    m_api->postJson(QStringLiteral("/backups/%1/restore").arg(QUrl::toPercentEncoding(name)),
                     {{"password", password}, {"modules", arr}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Restore failed: %1").arg(result.error)); return; }
        Notify::info(this, tr("Restore complete."));
    });
}

void BackupsPage::openSchedule() {
    m_api->get("/backups/schedule", {}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not load schedule: %1").arg(result.error)); return; }
        const QJsonObject sched = result.body.object();

        auto *dialog = new QDialog(this);
        dialog->setWindowTitle(tr("Backup Schedule"));
        dialog->setAttribute(Qt::WA_DeleteOnClose);

        auto *enabled = new QCheckBox(tr("Enable scheduled backups"), dialog);
        enabled->setChecked(sched.value("enabled").toBool());

        auto *frequency = new QComboBox(dialog);
        frequency->addItem(tr("Daily"), "daily");
        frequency->addItem(tr("Weekly"), "weekly");
        frequency->setCurrentIndex(sched.value("frequency").toString() == "weekly" ? 1 : 0);

        auto *retention = new QSpinBox(dialog);
        retention->setRange(1, 100);
        retention->setValue(sched.value("retentionCount").toInt(7));

        auto *dir = new QLineEdit(sched.value("dir").toString(), dialog);
        dir->setPlaceholderText(sched.value("effectiveDir").toString());

        auto *form = new QFormLayout;
        form->addRow(tr("Keep last:"), retention);
        form->addRow(tr("Frequency:"), frequency);
        form->addRow(tr("Directory (blank = default):"), dir);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, dialog);
        connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

        auto *layout = new QVBoxLayout(dialog);
        layout->addWidget(enabled);
        layout->addLayout(form);
        layout->addWidget(buttons);

        connect(dialog, &QDialog::accepted, this, [this, dialog, enabled, frequency, retention, dir]() {
            QJsonObject body{
                {"enabled", enabled->isChecked()},
                {"frequency", frequency->currentData().toString()},
                {"retentionCount", retention->value()},
            };
            if (!dir->text().trimmed().isEmpty()) body["dir"] = dir->text().trimmed();
            m_api->putJson("/backups/schedule", body, [this](const ApiResult &r) {
                if (!r.ok) Notify::error(this, tr("Could not save schedule: %1").arg(r.error));
            });
        });

        dialog->show();
    });
}
