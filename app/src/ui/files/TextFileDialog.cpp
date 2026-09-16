#include "TextFileDialog.h"
#include "../common/Notify.h"
#include "../../core/ApiClient.h"

#include <QPlainTextEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QUrlQuery>
#include <QJsonObject>
#include <QPushButton>

TextFileDialog::TextFileDialog(ApiClient *api, const QString &location, const QString &path, QWidget *parent)
    : QDialog(parent), m_api(api), m_location(location), m_path(path) {
    setWindowTitle(path);
    setMinimumSize(600, 500);

    m_editor = new QPlainTextEdit(this);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_status = new QLabel(this);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, this);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &TextFileDialog::save);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_editor);
    layout->addWidget(m_status);
    layout->addWidget(m_buttons);

    load();
}

void TextFileDialog::load() {
    m_editor->setReadOnly(true);
    m_status->setText(tr("Loading..."));
    QUrlQuery query;
    query.addQueryItem("location", m_location);
    query.addQueryItem("path", m_path);
    m_api->get("/files/text", query, [this](const ApiResult &result) {
        m_editor->setReadOnly(false);
        if (!result.ok) {
            m_status->setText(tr("Failed to load: %1").arg(result.error));
            return;
        }
        m_editor->setPlainText(result.body.object().value("content").toString());
        m_status->clear();
    });
}

void TextFileDialog::save() {
    m_buttons->setEnabled(false);
    m_status->setText(tr("Saving..."));
    QUrlQuery query;
    query.addQueryItem("location", m_location);
    query.addQueryItem("path", m_path);
    m_api->putRaw("/files/text", query, m_editor->toPlainText().toUtf8(), "text/plain; charset=utf-8",
                  [this](const ApiResult &result) {
        m_buttons->setEnabled(true);
        if (!result.ok) {
            m_status->setText(tr("Save failed: %1").arg(result.error));
            return;
        }
        m_status->setText(tr("Saved."));
    });
}
