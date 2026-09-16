#include "DashboardPage.h"
#include "../core/ApiClient.h"
#include "../core/Session.h"

#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QJsonObject>

namespace {
QGroupBox *makeStatTile(QWidget *parent, const QString &title, QLabel *&valueLabelOut) {
    auto *box = new QGroupBox(title, parent);
    valueLabelOut = new QLabel("-", box);
    valueLabelOut->setObjectName("statTileValue");
    auto *layout = new QVBoxLayout(box);
    layout->addWidget(valueLabelOut);
    return box;
}
}

DashboardPage::DashboardPage(ApiClient *api, Session *session, QWidget *parent)
    : PageWidget(parent), m_api(api), m_session(session) {
    m_welcomeLabel = new QLabel(this);
    QFont f = m_welcomeLabel->font();
    f.setPointSize(f.pointSize() + 5);
    f.setBold(true);
    m_welcomeLabel->setFont(f);
    m_welcomeLabel->setContentsMargins(0, 0, 0, 8);

    auto *grid = new QGridLayout;
    grid->setSpacing(14);
    grid->addWidget(makeStatTile(this, tr("Bookmarks"), m_bookmarksStat), 0, 0);
    grid->addWidget(makeStatTile(this, tr("Folders"), m_foldersStat), 0, 1);
    grid->addWidget(makeStatTile(this, tr("Contacts"), m_contactsStat), 0, 2);
    grid->addWidget(makeStatTile(this, tr("Events"), m_eventsStat), 1, 0);
    grid->addWidget(makeStatTile(this, tr("Password Entries"), m_passwordsStat), 1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->addWidget(m_welcomeLabel);
    layout->addLayout(grid);
    layout->addStretch();
}

void DashboardPage::reload() {
    m_welcomeLabel->setText(tr("Welcome, %1 (%2)").arg(m_session->username, m_session->role));
    m_api->get("/stats", {}, [this](const ApiResult &result) {
        if (!result.ok) return;
        const QJsonObject o = result.body.object();
        m_bookmarksStat->setText(QString::number(o.value("total").toInt()));
        m_foldersStat->setText(QString::number(o.value("folderCount").toInt()));
        m_contactsStat->setText(QString::number(o.value("contactTotal").toInt()));
        m_eventsStat->setText(QString::number(o.value("eventTotal").toInt()));
        m_passwordsStat->setText(QString::number(o.value("passwordTotal").toInt()));
    });
}
