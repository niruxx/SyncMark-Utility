#include "DuplicatesDialog.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"

#include <QListWidget>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QDialogButtonBox>
#include <QJsonArray>
#include <QScrollArea>

QString DuplicatesDialog::contactLabel(const QJsonObject &c) {
    QString name = c.value("full_name").toString();
    if (name.trimmed().isEmpty())
        name = (c.value("first_name").toString() + " " + c.value("last_name").toString()).trimmed();
    if (name.isEmpty()) name = QObject::tr("(unnamed)");
    QString extra;
    QJsonArray emails = c.value("emails").toArray();
    if (!emails.isEmpty()) extra = emails.first().toObject().value("value").toString();
    return extra.isEmpty() ? name : QStringLiteral("%1 — %2").arg(name, extra);
}

QVector<QJsonObject> DuplicatesDialog::contactsInGroup(const QJsonValue &groupValue) {
    QVector<QJsonObject> result;
    QJsonArray arr;
    if (groupValue.isArray()) {
        arr = groupValue.toArray();
    } else if (groupValue.isObject()) {
        const QJsonObject o = groupValue.toObject();
        if (o.contains("contacts")) arr = o.value("contacts").toArray();
        else if (o.contains("group")) arr = o.value("group").toArray();
        else if (o.contains("items")) arr = o.value("items").toArray();
    }
    for (const QJsonValue &v : arr)
        if (v.isObject()) result << v.toObject();
    return result;
}

DuplicatesDialog::DuplicatesDialog(ApiClient *api, QWidget *parent) : QDialog(parent), m_api(api) {
    setWindowTitle(tr("Possible Duplicate Contacts"));
    setMinimumSize(640, 440);

    m_groupList = new QListWidget(this);
    connect(m_groupList, &QListWidget::currentRowChanged, this, &DuplicatesDialog::showGroup);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    m_detailPanel = new QWidget(this);
    m_detailLayout = new QVBoxLayout(m_detailPanel);
    m_detailLayout->addStretch();
    scroll->setWidget(m_detailPanel);

    auto *mergeBtn = new QPushButton(tr("Merge Into Selected"), this);
    connect(mergeBtn, &QPushButton::clicked, this, &DuplicatesDialog::mergeCurrentGroup);
    auto *dismissBtn = new QPushButton(tr("Not Duplicates (Dismiss)"), this);
    connect(dismissBtn, &QPushButton::clicked, this, &DuplicatesDialog::dismissCurrentGroup);

    auto *actionRow = new QHBoxLayout;
    actionRow->addWidget(mergeBtn);
    actionRow->addWidget(dismissBtn);
    actionRow->addStretch();

    auto *rightLayout = new QVBoxLayout;
    rightLayout->addWidget(new QLabel(tr("Pick the contact to keep; the others merge into it.")));
    rightLayout->addWidget(scroll, 1);
    rightLayout->addLayout(actionRow);
    auto *rightWidget = new QWidget(this);
    rightWidget->setLayout(rightLayout);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(m_groupList);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(1, 1);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);

    auto *closeButtons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(closeButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(closeButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(splitter, 1);
    layout->addWidget(m_status);
    layout->addWidget(closeButtons);

    reloadGroups();
}

void DuplicatesDialog::reloadGroups() {
    m_api->get("/contacts/duplicates", {}, [this](const ApiResult &result) {
        if (!result.ok) { m_status->setText(result.error); return; }
        m_groups.clear();
        m_groupList->clear();
        const QJsonArray arr = result.body.array();
        for (const QJsonValue &v : arr) {
            const auto contacts = contactsInGroup(v);
            if (contacts.size() < 2) continue;
            m_groups << v;
            QStringList names;
            for (const QJsonObject &c : contacts) names << contactLabel(c);
            m_groupList->addItem(tr("%1 contacts: %2").arg(contacts.size()).arg(names.join(", ")));
        }
        m_status->setText(m_groups.isEmpty() ? tr("No likely duplicates found.") : QString());
        if (!m_groups.isEmpty()) m_groupList->setCurrentRow(0);
        else showGroup(-1);
    });
}

void DuplicatesDialog::showGroup(int index) {
    m_currentGroup = index;
    QLayoutItem *item;
    while ((item = m_detailLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    if (index < 0 || index >= m_groups.size()) {
        m_detailLayout->addStretch();
        return;
    }
    const auto contacts = contactsInGroup(m_groups[index]);
    auto *group = new QButtonGroup(m_detailPanel);
    for (int i = 0; i < contacts.size(); ++i) {
        auto *radio = new QRadioButton(contactLabel(contacts[i]), m_detailPanel);
        radio->setChecked(i == 0);
        radio->setProperty("contactId", contacts[i].value("id").toVariant());
        group->addButton(radio);
        m_detailLayout->addWidget(radio);
    }
    m_detailLayout->addStretch();
}

void DuplicatesDialog::mergeCurrentGroup() {
    if (m_currentGroup < 0) return;
    const auto contacts = contactsInGroup(m_groups[m_currentGroup]);

    qlonglong primaryId = -1;
    for (int i = 0; i < m_detailLayout->count(); ++i) {
        auto *radio = qobject_cast<QRadioButton *>(m_detailLayout->itemAt(i)->widget());
        if (radio && radio->isChecked()) { primaryId = radio->property("contactId").toLongLong(); break; }
    }
    if (primaryId < 0) return;

    QJsonArray mergeIds;
    for (const QJsonObject &c : contacts) {
        const qlonglong id = c.value("id").toVariant().toLongLong();
        if (id != primaryId) mergeIds.append(id);
    }
    if (mergeIds.isEmpty()) return;

    m_api->postJson("/contacts/merge", {{"primaryId", primaryId}, {"mergeIds", mergeIds}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Merge failed: %1").arg(result.error)); return; }
        emit contactsChanged();
        reloadGroups();
    });
}

void DuplicatesDialog::dismissCurrentGroup() {
    if (m_currentGroup < 0) return;
    const auto contacts = contactsInGroup(m_groups[m_currentGroup]);
    QJsonArray ids;
    for (const QJsonObject &c : contacts) ids.append(c.value("id").toVariant().toLongLong());

    m_api->postJson("/contacts/duplicates/dismiss", {{"ids", ids}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not dismiss: %1").arg(result.error)); return; }
        reloadGroups();
    });
}
