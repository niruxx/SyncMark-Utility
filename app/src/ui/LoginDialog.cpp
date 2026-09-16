#include "LoginDialog.h"
#include "../core/ApiClient.h"
#include "../core/AppSettings.h"

#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QJsonObject>
#include <QPushButton>

LoginDialog::LoginDialog(ApiClient *api, const QString &serverUrl, QWidget *parent)
    : QDialog(parent), m_api(api) {
    setWindowTitle(tr("Log in"));
    setMinimumWidth(360);

    m_username = new QLineEdit(this);
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_remember = new QCheckBox(tr("Stay signed in on this computer"), this);
    m_remember->setChecked(AppSettings::rememberSession());

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #b33;");

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Log in"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, &LoginDialog::submit);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_password, &QLineEdit::returnPressed, this, &LoginDialog::submit);

    auto *form = new QFormLayout;
    form->addRow(tr("Username:"), m_username);
    form->addRow(tr("Password:"), m_password);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Server: %1").arg(serverUrl)));
    layout->addLayout(form);
    layout->addWidget(m_remember);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_buttons);
}

void LoginDialog::submit() {
    const QString user = m_username->text().trimmed();
    const QString pass = m_password->text();
    if (user.isEmpty() || pass.isEmpty()) {
        m_statusLabel->setText(tr("Username and password are required."));
        return;
    }

    m_buttons->setEnabled(false);
    m_statusLabel->setStyleSheet("color: palette(text);");
    m_statusLabel->setText(tr("Signing in..."));

    QJsonObject body{{"username", user}, {"password", pass}};
    m_api->postJson("/auth/login", body, [this](const ApiResult &result) {
        m_buttons->setEnabled(true);
        if (!result.ok) {
            m_statusLabel->setStyleSheet("color: #b33;");
            m_statusLabel->setText(result.error.isEmpty() ? tr("Login failed.") : result.error);
            return;
        }
        m_role = result.body.object().value("role").toString();
        AppSettings::setRememberSession(m_remember->isChecked());
        accept();
    });
}
