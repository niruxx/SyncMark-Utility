#include "SetupDialog.h"
#include "common/AuthWidgets.h"
#include "../core/ApiClient.h"

#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QJsonObject>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>

SetupDialog::SetupDialog(ApiClient *api, QWidget *parent) : QDialog(parent), m_api(api) {
    setWindowTitle(tr("Set up SyncMark"));
    setMinimumWidth(460);

    m_username = new QLineEdit(this);
    m_username->setMinimumHeight(34);
    m_username->setPlaceholderText(tr("Username"));
    m_password = new QLineEdit(this);
    m_password->setMinimumHeight(34);
    m_password->setPlaceholderText(tr("Password"));
    m_password->setEchoMode(QLineEdit::Password);
    m_passwordConfirm = new QLineEdit(this);
    m_passwordConfirm->setMinimumHeight(34);
    m_passwordConfirm->setPlaceholderText(tr("Confirm password"));
    m_passwordConfirm->setEchoMode(QLineEdit::Password);
    m_adminPassword = new QLineEdit(this);
    m_adminPassword->setMinimumHeight(34);
    m_adminPassword->setPlaceholderText(tr("Admin recovery password"));
    m_adminPassword->setEchoMode(QLineEdit::Password);

    auto *fieldsLayout = new QVBoxLayout;
    fieldsLayout->setSpacing(8);
    fieldsLayout->addWidget(AuthWidgets::fieldLabel(tr("Username"), this));
    fieldsLayout->addWidget(m_username);
    fieldsLayout->addSpacing(4);
    fieldsLayout->addWidget(AuthWidgets::fieldLabel(tr("Password"), this));
    fieldsLayout->addWidget(m_password);
    fieldsLayout->addSpacing(4);
    fieldsLayout->addWidget(AuthWidgets::fieldLabel(tr("Confirm Password"), this));
    fieldsLayout->addWidget(m_passwordConfirm);
    fieldsLayout->addSpacing(4);
    fieldsLayout->addWidget(AuthWidgets::fieldLabel(tr("Admin Recovery Password"), this));
    fieldsLayout->addWidget(m_adminPassword);
    fieldsLayout->addWidget(AuthWidgets::subtitle(
        tr("Used only for account recovery — keep it somewhere safe."), this));

    m_bookmarks = new QCheckBox(tr("Bookmarks"), this);
    m_contacts = new QCheckBox(tr("Contacts"), this);
    m_calendar = new QCheckBox(tr("Calendar"), this);
    m_files = new QCheckBox(tr("Files"), this);
    m_passwords = new QCheckBox(tr("Password vault"), this);
    for (auto *cb : {m_bookmarks, m_contacts, m_calendar, m_files, m_passwords})
        cb->setChecked(true);

    auto *featuresBox = new QGroupBox(tr("Enabled modules"), this);
    auto *featuresLayout = new QGridLayout(featuresBox);
    featuresLayout->addWidget(m_bookmarks, 0, 0);
    featuresLayout->addWidget(m_contacts, 0, 1);
    featuresLayout->addWidget(m_calendar, 1, 0);
    featuresLayout->addWidget(m_files, 1, 1);
    featuresLayout->addWidget(m_passwords, 2, 0);

    m_statusLabel = AuthWidgets::messageLabel(this);

    m_createButton = AuthWidgets::primaryButton(tr("Create Account"), this);
    connect(m_createButton, &QPushButton::clicked, this, &SetupDialog::submit);

    m_cancelButton = AuthWidgets::linkButton(tr("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    auto *content = new QWidget(this);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(44, 36, 44, 28);
    layout->setSpacing(6);
    layout->addWidget(AuthWidgets::icon(this, 52));
    layout->addSpacing(8);
    layout->addWidget(AuthWidgets::title(tr("Set up SyncMark"), this));
    layout->addWidget(AuthWidgets::subtitle(
        tr("This server has no account yet. Create the first one."), this));
    layout->addSpacing(16);
    layout->addLayout(fieldsLayout);
    layout->addSpacing(14);
    layout->addWidget(featuresBox);
    layout->addSpacing(10);
    layout->addWidget(m_statusLabel);
    layout->addSpacing(10);
    layout->addWidget(m_createButton);

    auto *cancelRow = new QHBoxLayout;
    cancelRow->addStretch();
    cancelRow->addWidget(m_cancelButton);
    cancelRow->addStretch();
    layout->addLayout(cancelRow);

    auto *scroll = new QScrollArea(this);
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);
    resize(520, 640);
}

void SetupDialog::submit() {
    const QString user = m_username->text().trimmed();
    const QString pass = m_password->text();
    const QString adminPass = m_adminPassword->text();

    if (user.isEmpty() || pass.isEmpty() || adminPass.isEmpty()) {
        AuthWidgets::showError(m_statusLabel, tr("All fields are required."));
        return;
    }
    if (pass != m_passwordConfirm->text()) {
        AuthWidgets::showError(m_statusLabel, tr("Passwords do not match."));
        return;
    }
    if (pass.size() < 8) {
        AuthWidgets::showError(m_statusLabel, tr("Password must be at least 8 characters."));
        return;
    }

    QJsonObject features{
        {"bookmarks", m_bookmarks->isChecked()},
        {"contacts", m_contacts->isChecked()},
        {"calendar", m_calendar->isChecked()},
        {"files", m_files->isChecked()},
        {"passwords", m_passwords->isChecked()},
    };
    QJsonObject body{
        {"username", user},
        {"password", pass},
        {"adminPassword", adminPass},
        {"features", features},
    };

    m_createButton->setEnabled(false);
    AuthWidgets::showInfo(m_statusLabel, tr("Creating account..."));

    m_api->postJson("/auth/setup", body, [this](const ApiResult &result) {
        m_createButton->setEnabled(true);
        if (!result.ok) {
            AuthWidgets::showError(m_statusLabel, result.error.isEmpty() ? tr("Setup failed.") : result.error);
            return;
        }
        accept();
    });
}
