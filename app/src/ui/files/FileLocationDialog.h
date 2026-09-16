#pragma once

#include <QDialog>

class ApiClient;
class QLineEdit;
class QLabel;

// Add/edit a named file location (an absolute directory path the file
// browser is sandboxed to). "Browse Server..." is only offered for admins,
// matching the server's unsandboxed /files/browse-server route.
class FileLocationDialog : public QDialog {
    Q_OBJECT
public:
    explicit FileLocationDialog(ApiClient *api, bool isAdmin, QWidget *parent = nullptr);

    void setValues(const QString &name, const QString &path);
    QString name() const;
    QString path() const;

private slots:
    void browseServer();
    void validateAndAccept();

private:
    ApiClient *m_api;
    QLineEdit *m_name;
    QLineEdit *m_path;
    QLabel *m_error;
};
