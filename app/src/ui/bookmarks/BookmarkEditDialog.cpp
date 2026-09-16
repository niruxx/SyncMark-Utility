#include "BookmarkEditDialog.h"

#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>

BookmarkEditDialog::BookmarkEditDialog(const QStringList &existingFolders, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("Bookmark"));
    setMinimumWidth(420);

    m_title = new QLineEdit(this);
    m_url = new QLineEdit(this);
    m_url->setPlaceholderText("https://example.com");
    m_folder = new QComboBox(this);
    m_folder->setEditable(true);
    m_folder->addItem("");
    m_folder->addItems(existingFolders);
    m_favorite = new QCheckBox(tr("Favorite"), this);

    m_error = new QLabel(this);
    m_error->setStyleSheet("color: #b33;");
    m_error->setWordWrap(true);

    auto *form = new QFormLayout;
    form->addRow(tr("Title:"), m_title);
    form->addRow(tr("URL:"), m_url);
    form->addRow(tr("Folder:"), m_folder);
    form->addRow(QString(), m_favorite);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &BookmarkEditDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_error);
    layout->addWidget(buttons);
}

void BookmarkEditDialog::setBookmark(const QJsonObject &bookmark) {
    m_title->setText(bookmark.value("title").toString());
    m_url->setText(bookmark.value("url").toString());
    m_folder->setCurrentText(bookmark.value("folder").toString());
    m_favorite->setChecked(bookmark.value("favorite").toBool());
}

QJsonObject BookmarkEditDialog::formData() const {
    return QJsonObject{
        {"title", m_title->text().trimmed()},
        {"url", m_url->text().trimmed()},
        {"folder", m_folder->currentText().trimmed()},
        {"favorite", m_favorite->isChecked()},
    };
}

void BookmarkEditDialog::validateAndAccept() {
    if (m_title->text().trimmed().isEmpty() || m_url->text().trimmed().isEmpty()) {
        m_error->setText(tr("Title and URL are required."));
        return;
    }
    const QString url = m_url->text().trimmed();
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        m_error->setText(tr("URL must start with http:// or https://"));
        return;
    }
    accept();
}
