#pragma once

#include <QDialog>

class ApiClient;
class QCheckBox;
class QLabel;
class QDialogButtonBox;

// chmod-style permissions editor (GET/PUT /api/files/permissions). The
// server reports mode as `stat.mode & 0o777`, i.e. a plain 0-511 integer.
class PermissionsDialog : public QDialog {
    Q_OBJECT
public:
    PermissionsDialog(ApiClient *api, const QString &location, const QString &path, QWidget *parent = nullptr);

private slots:
    void load();
    void apply();

private:
    int currentMode() const;
    void setMode(int mode);

    ApiClient *m_api;
    QString m_location;
    QString m_path;

    QCheckBox *m_ownerRead, *m_ownerWrite, *m_ownerExec;
    QCheckBox *m_groupRead, *m_groupWrite, *m_groupExec;
    QCheckBox *m_otherRead, *m_otherWrite, *m_otherExec;
    QLabel *m_octalLabel;
    QLabel *m_status;
    QDialogButtonBox *m_buttons;
};
