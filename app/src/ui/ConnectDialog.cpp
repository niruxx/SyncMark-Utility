#include "ConnectDialog.h"
#include "../core/AppSettings.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>

ConnectDialog::ConnectDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Connect to SyncMark"));
    setMinimumWidth(420);

    m_urlCombo = new QComboBox(this);
    m_urlCombo->setEditable(true);
    m_urlCombo->addItems(AppSettings::recentServerUrls());
    QString last = AppSettings::lastServerUrl();
    if (!last.isEmpty()) {
        m_urlCombo->setCurrentText(last);
    } else if (m_urlCombo->count() == 0) {
        m_urlCombo->setCurrentText("http://localhost:3000");
    }
    m_urlCombo->lineEdit()->setPlaceholderText("http://localhost:3000");

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #b33;");

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Connect"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *form = new QFormLayout;
    form->addRow(tr("Server URL:"), m_urlCombo);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Enter the address of your SyncMark server.")));
    layout->addLayout(form);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_buttons);
}

QString ConnectDialog::serverUrl() const {
    return m_urlCombo->currentText().trimmed();
}

void ConnectDialog::setBusy(bool busy, const QString &message) {
    m_urlCombo->setEnabled(!busy);
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(!busy);
    m_statusLabel->setStyleSheet("color: palette(text);");
    m_statusLabel->setText(message);
}

void ConnectDialog::showError(const QString &message) {
    m_statusLabel->setStyleSheet("color: #b33;");
    m_statusLabel->setText(message);
}
