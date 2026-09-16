#include "FeatureTogglesDialog.h"
#include "common/Notify.h"
#include "../core/ApiClient.h"

#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QJsonObject>

FeatureTogglesDialog::FeatureTogglesDialog(ApiClient *api, QWidget *parent) : QDialog(parent), m_api(api) {
    setWindowTitle(tr("Enabled Modules"));
    setMinimumWidth(320);

    m_bookmarks = new QCheckBox(tr("Bookmarks"), this);
    m_contacts = new QCheckBox(tr("Contacts"), this);
    m_calendar = new QCheckBox(tr("Calendar"), this);
    m_files = new QCheckBox(tr("Files"), this);
    m_passwords = new QCheckBox(tr("Password vault"), this);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &FeatureTogglesDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("At least one module must stay enabled.")));
    for (auto *cb : {m_bookmarks, m_contacts, m_calendar, m_files, m_passwords})
        layout->addWidget(cb);
    layout->addWidget(m_status);
    layout->addWidget(buttons);

    load();
}

void FeatureTogglesDialog::load() {
    m_api->get("/features", {}, [this](const ApiResult &result) {
        if (!result.ok) { m_status->setText(result.error); return; }
        const QJsonObject o = result.body.object();
        m_bookmarks->setChecked(o.value("bookmarks").toBool(true));
        m_contacts->setChecked(o.value("contacts").toBool(true));
        m_calendar->setChecked(o.value("calendar").toBool(true));
        m_files->setChecked(o.value("files").toBool(true));
        m_passwords->setChecked(o.value("passwords").toBool(true));
    });
}

void FeatureTogglesDialog::save() {
    QJsonObject body{
        {"bookmarks", m_bookmarks->isChecked()},
        {"contacts", m_contacts->isChecked()},
        {"calendar", m_calendar->isChecked()},
        {"files", m_files->isChecked()},
        {"passwords", m_passwords->isChecked()},
    };
    m_api->putJson("/features", body, [this](const ApiResult &result) {
        if (!result.ok) { m_status->setText(result.error); return; }
        emit featuresChanged();
        accept();
    });
}
