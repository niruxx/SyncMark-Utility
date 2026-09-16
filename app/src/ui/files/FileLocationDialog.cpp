#include "FileLocationDialog.h"
#include "RemoteFolderPickerDialog.h"

#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>

FileLocationDialog::FileLocationDialog(ApiClient *api, bool isAdmin, QWidget *parent)
    : QDialog(parent), m_api(api) {
    setWindowTitle(tr("File Location"));
    setMinimumWidth(420);

    m_name = new QLineEdit(this);
    m_path = new QLineEdit(this);
    m_path->setPlaceholderText("/home/me/Documents");

    auto *pathRow = new QHBoxLayout;
    pathRow->addWidget(m_path);
    if (isAdmin) {
        auto *browseBtn = new QPushButton(tr("Browse Server..."), this);
        connect(browseBtn, &QPushButton::clicked, this, &FileLocationDialog::browseServer);
        pathRow->addWidget(browseBtn);
    }

    m_error = new QLabel(this);
    m_error->setStyleSheet("color: #b33;");
    m_error->setWordWrap(true);

    auto *form = new QFormLayout;
    form->addRow(tr("Display name:"), m_name);
    form->addRow(tr("Directory path:"), pathRow);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &FileLocationDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_error);
    layout->addWidget(buttons);
}

void FileLocationDialog::browseServer() {
    RemoteFolderPickerDialog picker(m_api, this);
    if (picker.exec() == QDialog::Accepted)
        m_path->setText(picker.selectedPath());
}

void FileLocationDialog::setValues(const QString &name, const QString &path) {
    m_name->setText(name);
    m_path->setText(path);
}

QString FileLocationDialog::name() const { return m_name->text().trimmed(); }
QString FileLocationDialog::path() const { return m_path->text().trimmed(); }

void FileLocationDialog::validateAndAccept() {
    if (m_name->text().trimmed().isEmpty() || m_path->text().trimmed().isEmpty()) {
        m_error->setText(tr("Both a name and a path are required."));
        return;
    }
    accept();
}
