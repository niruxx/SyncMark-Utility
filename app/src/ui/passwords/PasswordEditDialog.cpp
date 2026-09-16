#include "PasswordEditDialog.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PasswordEditDialog::PasswordEditDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Password Entry"));
    setMinimumWidth(420);

    m_siteName = new QLineEdit(this);
    m_url = new QLineEdit(this);
    m_url->setPlaceholderText("https://example.com");
    m_username = new QLineEdit(this);

    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_revealBtn = new QToolButton(this);
    m_revealBtn->setText(tr("Show"));
    m_revealBtn->setCheckable(true);
    connect(m_revealBtn, &QToolButton::toggled, this, &PasswordEditDialog::togglePasswordVisibility);
    auto *genBtn = new QPushButton(tr("Generate"), this);
    connect(genBtn, &QPushButton::clicked, this, &PasswordEditDialog::generatePassword);

    auto *passwordRow = new QHBoxLayout;
    passwordRow->addWidget(m_password);
    passwordRow->addWidget(m_revealBtn);
    passwordRow->addWidget(genBtn);

    m_notes = new QPlainTextEdit(this);
    m_notes->setMaximumHeight(80);
    m_favorite = new QCheckBox(tr("Favorite"), this);

    m_error = new QLabel(this);
    m_error->setStyleSheet("color: #b33;");
    m_error->setWordWrap(true);

    auto *form = new QFormLayout;
    form->addRow(tr("Site name:"), m_siteName);
    form->addRow(tr("URL:"), m_url);
    form->addRow(tr("Username:"), m_username);
    form->addRow(tr("Password:"), passwordRow);
    form->addRow(tr("Notes:"), m_notes);
    form->addRow(QString(), m_favorite);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &PasswordEditDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_error);
    layout->addWidget(buttons);
}

void PasswordEditDialog::togglePasswordVisibility() {
    const bool shown = m_revealBtn->isChecked();
    m_password->setEchoMode(shown ? QLineEdit::Normal : QLineEdit::Password);
    m_revealBtn->setText(shown ? tr("Hide") : tr("Show"));
}

void PasswordEditDialog::generatePassword() {
    static const QString chars =
        "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789!@#$%^&*()-_=+";
    QString pass;
    for (int i = 0; i < 20; ++i)
        pass += chars.at(QRandomGenerator::global()->bounded(chars.size()));
    m_password->setText(pass);
    m_revealBtn->setChecked(true);
}

void PasswordEditDialog::setEntry(const QJsonObject &entry) {
    m_siteName->setText(entry.value("site_name").toString());
    m_url->setText(entry.value("url").toString());
    m_username->setText(entry.value("username").toString());
    m_password->setText(entry.value("password").toString());
    m_notes->setPlainText(entry.value("notes").toString());
    m_favorite->setChecked(entry.value("favorite").toInt() != 0 || entry.value("favorite").toBool());
}

QJsonObject PasswordEditDialog::formData() const {
    return QJsonObject{
        {"siteName", m_siteName->text().trimmed()},
        {"url", m_url->text().trimmed()},
        {"username", m_username->text().trimmed()},
        {"password", m_password->text()},
        {"notes", m_notes->toPlainText()},
        {"favorite", m_favorite->isChecked()},
    };
}

void PasswordEditDialog::validateAndAccept() {
    if (m_siteName->text().trimmed().isEmpty()) {
        m_error->setText(tr("Site name is required."));
        return;
    }
    if (m_password->text().isEmpty()) {
        m_error->setText(tr("Password is required."));
        return;
    }
    accept();
}
