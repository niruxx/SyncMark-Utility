#include "SetupDialog.h"
#include "../core/ApiClient.h"

#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QJsonObject>
#include <QPushButton>

SetupDialog::SetupDialog(ApiClient *api, QWidget *parent) : QDialog(parent), m_api(api) {
    setWindowTitle(tr("Set up SyncMark"));
    setMinimumWidth(420);

    m_username = new QLineEdit(this);
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_passwordConfirm = new QLineEdit(this);
    m_passwordConfirm->setEchoMode(QLineEdit::Password);
    m_adminPassword = new QLineEdit(this);
    m_adminPassword->setEchoMode(QLineEdit::Password);

    auto *form = new QFormLayout;
    form->addRow(tr("Username:"), m_username);
    form->addRow(tr("Password:"), m_password);
    form->addRow(tr("Confirm password:"), m_passwordConfirm);
    form->addRow(tr("Admin recovery password:"), m_adminPassword);

    m_bookmarks = new QCheckBox(tr("Bookmarks"), this);
    m_contacts = new QCheckBox(tr("Contacts"), this);
    m_calendar = new QCheckBox(tr("Calendar"), this);
    m_files = new QCheckBox(tr("Files"), this);
    m_passwords = new QCheckBox(tr("Password vault"), this);
    for (auto *cb : {m_bookmarks, m_contacts, m_calendar, m_files, m_passwords})
        cb->setChecked(true);

    auto *featuresBox = new QGroupBox(tr("Enabled modules"), this);
    auto *featuresLayout = new QVBoxLayout(featuresBox);
    for (auto *cb : {m_bookmarks, m_contacts, m_calendar, m_files, m_passwords})
        featuresLayout->addWidget(cb);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #b33;");

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Create account"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, &SetupDialog::submit);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("This server has no account yet. Create the first one.")));
    layout->addLayout(form);
    layout->addWidget(featuresBox);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_buttons);
}

void SetupDialog::submit() {
    const QString user = m_username->text().trimmed();
    const QString pass = m_password->text();
    const QString adminPass = m_adminPassword->text();

    if (user.isEmpty() || pass.isEmpty() || adminPass.isEmpty()) {
        m_statusLabel->setText(tr("All fields are required."));
        return;
    }
    if (pass != m_passwordConfirm->text()) {
        m_statusLabel->setText(tr("Passwords do not match."));
        return;
    }
    if (pass.size() < 8) {
        m_statusLabel->setText(tr("Password must be at least 8 characters."));
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

    m_buttons->setEnabled(false);
    m_statusLabel->setStyleSheet("color: palette(text);");
    m_statusLabel->setText(tr("Creating account..."));

    m_api->postJson("/auth/setup", body, [this](const ApiResult &result) {
        m_buttons->setEnabled(true);
        if (!result.ok) {
            m_statusLabel->setStyleSheet("color: #b33;");
            m_statusLabel->setText(result.error.isEmpty() ? tr("Setup failed.") : result.error);
            return;
        }
        accept();
    });
}
