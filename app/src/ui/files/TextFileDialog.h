#pragma once

#include <QDialog>

class ApiClient;
class QPlainTextEdit;
class QLabel;
class QDialogButtonBox;

// Views and edits a server-side text file (GET/PUT /api/files/text).
class TextFileDialog : public QDialog {
    Q_OBJECT
public:
    TextFileDialog(ApiClient *api, const QString &location, const QString &path, QWidget *parent = nullptr);

private slots:
    void load();
    void save();

private:
    ApiClient *m_api;
    QString m_location;
    QString m_path;
    QPlainTextEdit *m_editor;
    QLabel *m_status;
    QDialogButtonBox *m_buttons;
};
