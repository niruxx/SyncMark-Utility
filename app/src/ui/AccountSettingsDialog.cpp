#include "AccountSettingsDialog.h"
#include "common/Notify.h"
#include "../core/ApiClient.h"

#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonObject>
#include <QPixmap>
#include <QInputDialog>

namespace {
constexpr qint64 Minute = 60'000;
constexpr qint64 Hour = 60 * Minute;
constexpr qint64 Day = 24 * Hour;
}

AccountSettingsDialog::AccountSettingsDialog(ApiClient *api, QWidget *parent) : QDialog(parent), m_api(api) {
    setWindowTitle(tr("Account Settings"));
    setMinimumWidth(440);

    m_avatarLabel = new QLabel(tr("(no avatar)"), this);
    m_avatarLabel->setFixedSize(64, 64);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setStyleSheet("border: 1px solid palette(mid);");
    auto *avatarBtn = new QPushButton(tr("Change..."), this);
    connect(avatarBtn, &QPushButton::clicked, this, &AccountSettingsDialog::changeAvatar);
    auto *avatarRemoveBtn = new QPushButton(tr("Remove"), this);
    connect(avatarRemoveBtn, &QPushButton::clicked, this, &AccountSettingsDialog::removeAvatar);

    auto *avatarRow = new QHBoxLayout;
    avatarRow->addWidget(m_avatarLabel);
    avatarRow->addWidget(avatarBtn);
    avatarRow->addWidget(avatarRemoveBtn);
    avatarRow->addStretch();

    m_username = new QLineEdit(this);
    m_currentPassword = new QLineEdit(this);
    m_currentPassword->setEchoMode(QLineEdit::Password);
    m_newPassword = new QLineEdit(this);
    m_newPassword->setEchoMode(QLineEdit::Password);
    m_newPassword->setPlaceholderText(tr("Leave blank to keep current password"));

    auto *profileForm = new QFormLayout;
    profileForm->addRow(tr("Username:"), m_username);
    profileForm->addRow(tr("Current password:"), m_currentPassword);
    profileForm->addRow(tr("New password:"), m_newPassword);

    auto *saveProfileBtn = new QPushButton(tr("Save Profile"), this);
    connect(saveProfileBtn, &QPushButton::clicked, this, &AccountSettingsDialog::saveProfile);

    auto *profileBox = new QGroupBox(tr("Profile"), this);
    auto *profileLayout = new QVBoxLayout(profileBox);
    profileLayout->addLayout(avatarRow);
    profileLayout->addLayout(profileForm);
    profileLayout->addWidget(saveProfileBtn);

    m_sessionDuration = new QComboBox(this);
    m_sessionDuration->addItem(tr("5 minutes"), Minute * 5);
    m_sessionDuration->addItem(tr("1 hour"), Hour);
    m_sessionDuration->addItem(tr("1 day"), Day);
    m_sessionDuration->addItem(tr("30 days"), Day * 30);
    m_sessionDuration->addItem(tr("Permanent"), Day * 365 * 10);
    auto *saveSessionBtn = new QPushButton(tr("Save"), this);
    connect(saveSessionBtn, &QPushButton::clicked, this, &AccountSettingsDialog::saveSessionDuration);

    auto *sessionRow = new QHBoxLayout;
    sessionRow->addWidget(m_sessionDuration, 1);
    sessionRow->addWidget(saveSessionBtn);

    auto *sessionBox = new QGroupBox(tr("Stay signed in for"), this);
    auto *sessionLayout = new QVBoxLayout(sessionBox);
    sessionLayout->addLayout(sessionRow);

    auto *deleteBtn = new QPushButton(tr("Delete Account..."), this);
    deleteBtn->setStyleSheet("color: #b33;");
    connect(deleteBtn, &QPushButton::clicked, this, &AccountSettingsDialog::deleteAccount);

    auto *dangerBox = new QGroupBox(tr("Danger Zone"), this);
    auto *dangerLayout = new QVBoxLayout(dangerBox);
    dangerLayout->addWidget(deleteBtn);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);

    auto *closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(profileBox);
    layout->addWidget(sessionBox);
    layout->addWidget(dangerBox);
    layout->addWidget(m_status);
    layout->addWidget(closeBtn);

    loadMe();
    loadSettings();
}

void AccountSettingsDialog::loadMe() {
    m_api->get("/auth/me", {}, [this](const ApiResult &result) {
        if (!result.ok) return;
        const QJsonObject o = result.body.object();
        m_username->setText(o.value("username").toString());
        if (o.value("hasAvatar").toBool()) {
            m_api->getRaw("/auth/avatar", {}, [this](const ApiRawResult &avatarResult) {
                if (!avatarResult.ok) return;
                QPixmap pix;
                if (pix.loadFromData(avatarResult.data))
                    m_avatarLabel->setPixmap(pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            });
        }
    });
}

void AccountSettingsDialog::loadSettings() {
    m_api->get("/auth/settings", {}, [this](const ApiResult &result) {
        if (!result.ok) return;
        const qint64 duration = result.body.object().value("sessionDuration").toVariant().toLongLong();
        int idx = m_sessionDuration->findData(duration);
        if (idx >= 0) m_sessionDuration->setCurrentIndex(idx);
    });
}

void AccountSettingsDialog::changeAvatar() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Select Avatar"), {},
        tr("Images (*.png *.jpg *.jpeg *.gif *.webp)"));
    if (path.isEmpty()) return;

    auto *file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) { Notify::error(this, tr("Could not open file.")); delete file; return; }

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QVariant(QStringLiteral("form-data; name=\"avatar\"; filename=\"%1\"").arg(QFileInfo(path).fileName())));
    file->setParent(multiPart);
    part.setBodyDevice(file);
    multiPart->append(part);

    m_api->postMultipart("/auth/avatar", multiPart, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not upload avatar: %1").arg(result.error)); return; }
        loadMe();
    });
}

void AccountSettingsDialog::removeAvatar() {
    m_api->del("/auth/avatar", [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not remove avatar: %1").arg(result.error)); return; }
        m_avatarLabel->setPixmap({});
        m_avatarLabel->setText(tr("(no avatar)"));
    });
}

void AccountSettingsDialog::saveProfile() {
    if (m_currentPassword->text().isEmpty()) {
        m_status->setText(tr("Enter your current password to save profile changes."));
        return;
    }
    QJsonObject body{{"currentPassword", m_currentPassword->text()}, {"username", m_username->text().trimmed()}};
    if (!m_newPassword->text().isEmpty()) body["newPassword"] = m_newPassword->text();

    m_api->putJson("/auth/account", body, [this](const ApiResult &result) {
        if (!result.ok) { m_status->setText(result.error); return; }
        m_status->setText(tr("Profile updated."));
        m_currentPassword->clear();
        m_newPassword->clear();
    });
}

void AccountSettingsDialog::saveSessionDuration() {
    m_api->putJson("/auth/settings", {{"sessionDuration", m_sessionDuration->currentData().toDouble()}},
                    [this](const ApiResult &result) {
        m_status->setText(result.ok ? tr("Session setting saved.") : result.error);
    });
}

void AccountSettingsDialog::deleteAccount() {
    if (!Notify::confirm(this, tr("Delete Account"),
                          tr("This permanently deletes your account and all its data. Continue?")))
        return;
    bool ok = false;
    const QString password = QInputDialog::getText(this, tr("Confirm Password"),
                                                     tr("Enter your password to confirm:"),
                                                     QLineEdit::Password, {}, &ok);
    if (!ok || password.isEmpty()) return;

    m_api->delJson("/auth/account", {{"password", password}}, [this](const ApiResult &result) {
        if (!result.ok) { Notify::error(this, tr("Could not delete account: %1").arg(result.error)); return; }
        emit accountDeleted();
        accept();
    });
}
