#include "PermissionsDialog.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"

#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QUrlQuery>
#include <QJsonObject>

namespace {
QCheckBox *makeBox(QWidget *parent, QGridLayout *grid, int row, int col, const QString &label) {
    auto *cb = new QCheckBox(label, parent);
    grid->addWidget(cb, row, col);
    return cb;
}
}

PermissionsDialog::PermissionsDialog(ApiClient *api, const QString &location, const QString &path, QWidget *parent)
    : QDialog(parent), m_api(api), m_location(location), m_path(path) {
    setWindowTitle(tr("Permissions - %1").arg(path));
    setMinimumWidth(380);

    auto *grid = new QGridLayout;
    grid->addWidget(new QLabel(tr("Read")), 0, 1);
    grid->addWidget(new QLabel(tr("Write")), 0, 2);
    grid->addWidget(new QLabel(tr("Execute")), 0, 3);
    grid->addWidget(new QLabel(tr("Owner")), 1, 0);
    grid->addWidget(new QLabel(tr("Group")), 2, 0);
    grid->addWidget(new QLabel(tr("Other")), 3, 0);

    m_ownerRead = makeBox(this, grid, 1, 1, {});
    m_ownerWrite = makeBox(this, grid, 1, 2, {});
    m_ownerExec = makeBox(this, grid, 1, 3, {});
    m_groupRead = makeBox(this, grid, 2, 1, {});
    m_groupWrite = makeBox(this, grid, 2, 2, {});
    m_groupExec = makeBox(this, grid, 2, 3, {});
    m_otherRead = makeBox(this, grid, 3, 1, {});
    m_otherWrite = makeBox(this, grid, 3, 2, {});
    m_otherExec = makeBox(this, grid, 3, 3, {});

    for (QCheckBox *cb : {m_ownerRead, m_ownerWrite, m_ownerExec, m_groupRead, m_groupWrite, m_groupExec,
                           m_otherRead, m_otherWrite, m_otherExec}) {
        connect(cb, &QCheckBox::toggled, this, [this] {
            m_octalLabel->setText(tr("Octal: %1").arg(currentMode(), 3, 8, QChar('0')));
        });
    }

    auto *box = new QGroupBox(tr("Permissions"), this);
    box->setLayout(grid);

    m_octalLabel = new QLabel(this);
    m_status = new QLabel(this);
    m_status->setWordWrap(true);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &PermissionsDialog::apply);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(box);
    layout->addWidget(m_octalLabel);
    layout->addWidget(m_status);
    layout->addWidget(m_buttons);

    load();
}

void PermissionsDialog::setMode(int mode) {
    m_ownerRead->setChecked(mode & 0400);
    m_ownerWrite->setChecked(mode & 0200);
    m_ownerExec->setChecked(mode & 0100);
    m_groupRead->setChecked(mode & 0040);
    m_groupWrite->setChecked(mode & 0020);
    m_groupExec->setChecked(mode & 0010);
    m_otherRead->setChecked(mode & 0004);
    m_otherWrite->setChecked(mode & 0002);
    m_otherExec->setChecked(mode & 0001);
    m_octalLabel->setText(tr("Octal: %1").arg(mode, 3, 8, QChar('0')));
}

int PermissionsDialog::currentMode() const {
    int mode = 0;
    if (m_ownerRead->isChecked()) mode |= 0400;
    if (m_ownerWrite->isChecked()) mode |= 0200;
    if (m_ownerExec->isChecked()) mode |= 0100;
    if (m_groupRead->isChecked()) mode |= 0040;
    if (m_groupWrite->isChecked()) mode |= 0020;
    if (m_groupExec->isChecked()) mode |= 0010;
    if (m_otherRead->isChecked()) mode |= 0004;
    if (m_otherWrite->isChecked()) mode |= 0002;
    if (m_otherExec->isChecked()) mode |= 0001;
    return mode;
}

void PermissionsDialog::load() {
    QUrlQuery query;
    query.addQueryItem("location", m_location);
    query.addQueryItem("path", m_path);
    m_buttons->setEnabled(false);
    m_status->setText(tr("Loading..."));
    m_api->get("/files/permissions", query, [this](const ApiResult &result) {
        m_buttons->setEnabled(true);
        if (!result.ok) { m_status->setText(result.error); return; }
        const QJsonObject o = result.body.object();
        setMode(o.value("mode").toInt());
        if (o.value("platform").toString() == "win32")
            m_status->setText(tr("Note: the server is running on Windows, where these Unix-style bits have limited effect."));
        else
            m_status->clear();
    });
}

void PermissionsDialog::apply() {
    m_buttons->setEnabled(false);
    QUrlQuery query;
    query.addQueryItem("location", m_location);
    query.addQueryItem("path", m_path);
    QJsonObject body{{"location", m_location}, {"path", m_path}, {"mode", currentMode()}};
    m_api->putJson(QStringLiteral("/files/permissions?%1").arg(query.toString(QUrl::FullyEncoded)), body,
                    [this](const ApiResult &result) {
        m_buttons->setEnabled(true);
        if (!result.ok) { Notify::error(this, tr("Could not update permissions: %1").arg(result.error)); return; }
        accept();
    });
}
