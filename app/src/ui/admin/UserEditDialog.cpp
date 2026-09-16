#include "UserEditDialog.h"

#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>

UserEditDialog::UserEditDialog(bool isNew, QWidget *parent) : QDialog(parent), m_isNew(isNew) {
    setWindowTitle(isNew ? tr("New User") : tr("Edit User"));
    setMinimumWidth(360);

    m_username = new QLineEdit(this);
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText(isNew ? QString() : tr("Leave blank to keep current password"));

    m_error = new QLabel(this);
    m_error->setStyleSheet("color: #b33;");
    m_error->setWordWrap(true);

    auto *form = new QFormLayout;
    form->addRow(tr("Username:"), m_username);
    form->addRow(tr("Password:"), m_password);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &UserEditDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_error);
    layout->addWidget(buttons);
}

void UserEditDialog::setUsername(const QString &username) { m_username->setText(username); }
QString UserEditDialog::username() const { return m_username->text().trimmed(); }
QString UserEditDialog::password() const { return m_password->text(); }

void UserEditDialog::validateAndAccept() {
    if (m_username->text().trimmed().isEmpty()) {
        m_error->setText(tr("Username is required."));
        return;
    }
    if (m_isNew && m_password->text().size() < 8) {
        m_error->setText(tr("Password must be at least 8 characters."));
        return;
    }
    if (!m_isNew && !m_password->text().isEmpty() && m_password->text().size() < 8) {
        m_error->setText(tr("Password must be at least 8 characters."));
        return;
    }
    accept();
}
