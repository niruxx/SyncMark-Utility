#include "ConnectDialog.h"
#include "common/AuthWidgets.h"
#include "../core/AppSettings.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

ConnectDialog::ConnectDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Connect to SyncMark"));
    setMinimumWidth(440);

    m_urlCombo = new QComboBox(this);
    m_urlCombo->setEditable(true);
    m_urlCombo->setMinimumHeight(36);
    m_urlCombo->addItems(AppSettings::recentServerUrls());
    QString last = AppSettings::lastServerUrl();
    if (!last.isEmpty()) {
        m_urlCombo->setCurrentText(last);
    } else if (m_urlCombo->count() == 0) {
        m_urlCombo->setCurrentText("http://localhost:3000");
    }
    m_urlCombo->lineEdit()->setPlaceholderText("http://localhost:3000");

    m_statusLabel = AuthWidgets::messageLabel(this);

    m_connectButton = AuthWidgets::primaryButton(tr("Connect"), this);
    connect(m_connectButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_urlCombo->lineEdit(), &QLineEdit::returnPressed, this, &QDialog::accept);

    m_exitButton = AuthWidgets::linkButton(tr("Exit"), this);
    connect(m_exitButton, &QPushButton::clicked, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(44, 40, 44, 32);
    layout->setSpacing(6);
    layout->addWidget(AuthWidgets::icon(this));
    layout->addSpacing(12);
    layout->addWidget(AuthWidgets::title(tr("Connect to SyncMark"), this));
    layout->addWidget(AuthWidgets::subtitle(
        tr("Enter the address of your self-hosted server to get started."), this));
    layout->addSpacing(18);
    layout->addWidget(AuthWidgets::fieldLabel(tr("Server URL"), this));
    layout->addWidget(m_urlCombo);
    layout->addSpacing(4);
    layout->addWidget(m_statusLabel);
    layout->addSpacing(14);
    layout->addWidget(m_connectButton);

    auto *exitRow = new QHBoxLayout;
    exitRow->addStretch();
    exitRow->addWidget(m_exitButton);
    exitRow->addStretch();
    layout->addLayout(exitRow);
}

QString ConnectDialog::serverUrl() const {
    return m_urlCombo->currentText().trimmed();
}

void ConnectDialog::setBusy(bool busy, const QString &message) {
    m_urlCombo->setEnabled(!busy);
    m_connectButton->setEnabled(!busy);
    if (message.isEmpty())
        AuthWidgets::clearMessage(m_statusLabel);
    else
        AuthWidgets::showInfo(m_statusLabel, message);
}

void ConnectDialog::showError(const QString &message) {
    AuthWidgets::showError(m_statusLabel, message);
}
