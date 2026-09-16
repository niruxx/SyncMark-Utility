#include "LoginDialog.h"
#include "common/AuthWidgets.h"
#include "../core/ApiClient.h"
#include "../core/AppSettings.h"

#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QPushButton>

LoginDialog::LoginDialog(ApiClient *api, const QString &serverUrl, QWidget *parent)
    : QDialog(parent), m_api(api) {
    setWindowTitle(tr("Log in"));
    setMinimumWidth(400);

    m_username = new QLineEdit(this);
    m_username->setMinimumHeight(36);
    m_username->setPlaceholderText(tr("Username"));
    m_password = new QLineEdit(this);
    m_password->setMinimumHeight(36);
    m_password->setPlaceholderText(tr("Password"));
    m_password->setEchoMode(QLineEdit::Password);
    m_remember = new QCheckBox(tr("Stay signed in on this computer"), this);
    m_remember->setChecked(AppSettings::rememberSession());

    m_statusLabel = AuthWidgets::messageLabel(this);

    m_loginButton = AuthWidgets::primaryButton(tr("Log In"), this);
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::submit);
    connect(m_password, &QLineEdit::returnPressed, this, &LoginDialog::submit);
    connect(m_username, &QLineEdit::returnPressed, this, &LoginDialog::submit);

    m_cancelButton = AuthWidgets::linkButton(tr("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(44, 36, 44, 28);
    layout->setSpacing(6);
    layout->addWidget(AuthWidgets::icon(this, 56));
    layout->addSpacing(10);
    layout->addWidget(AuthWidgets::title(tr("Welcome back"), this));
    layout->addWidget(AuthWidgets::subtitle(tr("Signing in to %1").arg(serverUrl), this));
    layout->addSpacing(16);
    layout->addWidget(AuthWidgets::fieldLabel(tr("Username"), this));
    layout->addWidget(m_username);
    layout->addSpacing(10);
    layout->addWidget(AuthWidgets::fieldLabel(tr("Password"), this));
    layout->addWidget(m_password);
    layout->addSpacing(6);
    layout->addWidget(m_remember);
    layout->addSpacing(4);
    layout->addWidget(m_statusLabel);
    layout->addSpacing(12);
    layout->addWidget(m_loginButton);

    auto *cancelRow = new QHBoxLayout;
    cancelRow->addStretch();
    cancelRow->addWidget(m_cancelButton);
    cancelRow->addStretch();
    layout->addLayout(cancelRow);
}

void LoginDialog::submit() {
    const QString user = m_username->text().trimmed();
    const QString pass = m_password->text();
    if (user.isEmpty() || pass.isEmpty()) {
        AuthWidgets::showError(m_statusLabel, tr("Username and password are required."));
        return;
    }

    m_loginButton->setEnabled(false);
    AuthWidgets::showInfo(m_statusLabel, tr("Signing in..."));

    QJsonObject body{{"username", user}, {"password", pass}};
    m_api->postJson("/auth/login", body, [this](const ApiResult &result) {
        m_loginButton->setEnabled(true);
        if (!result.ok) {
            AuthWidgets::showError(m_statusLabel, result.error.isEmpty() ? tr("Login failed.") : result.error);
            return;
        }
        m_role = result.body.object().value("role").toString();
        AppSettings::setRememberSession(m_remember->isChecked());
        accept();
    });
}
