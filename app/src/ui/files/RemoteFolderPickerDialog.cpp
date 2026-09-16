#include "RemoteFolderPickerDialog.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"

#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QUrlQuery>
#include <QJsonArray>
#include <QJsonObject>
#include <QStyle>

RemoteFolderPickerDialog::RemoteFolderPickerDialog(ApiClient *api, QWidget *parent)
    : QDialog(parent), m_api(api) {
    setWindowTitle(tr("Choose Server Folder"));
    setMinimumSize(480, 420);

    m_pathLabel = new QLabel(this);
    m_pathLabel->setWordWrap(true);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    connect(m_tree, &QTreeWidget::itemActivated, this, &RemoteFolderPickerDialog::onItemActivated);

    auto *upBtn = new QPushButton(tr("Up"), this);
    connect(upBtn, &QPushButton::clicked, this, [this] {
        int idx = m_currentPath.lastIndexOf('/');
        browseTo(idx > 0 ? m_currentPath.left(idx) : "/");
    });

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Select This Folder"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_pathLabel);
    layout->addWidget(upBtn);
    layout->addWidget(m_tree);
    layout->addWidget(m_buttons);

    browseTo({});
}

void RemoteFolderPickerDialog::browseTo(const QString &path) {
    QUrlQuery query;
    if (!path.isEmpty()) query.addQueryItem("path", path);
    m_api->get("/files/browse-server", query, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not browse: %1").arg(result.error)); return; }
        const QJsonObject obj = result.body.object();
        m_currentPath = obj.value("path").toString();
        m_pathLabel->setText(m_currentPath);

        m_tree->clear();
        for (const QJsonValue &v : obj.value("entries").toArray()) {
            const QJsonObject entry = v.toObject();
            if (entry.value("type").toString() != "dir" && !entry.value("isDirectory").toBool())
                continue;
            auto *item = new QTreeWidgetItem(m_tree, {entry.value("name").toString()});
            item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        }
    });
}

void RemoteFolderPickerDialog::onItemActivated(QTreeWidgetItem *item, int) {
    if (!item) return;
    const QString sep = m_currentPath.endsWith('/') ? "" : "/";
    browseTo(m_currentPath + sep + item->text(0));
}
