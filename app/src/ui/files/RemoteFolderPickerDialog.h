#pragma once

#include <QDialog>

class ApiClient;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QDialogButtonBox;

// Admin-only unsandboxed filesystem browser (GET /files/browse-server), used
// to pick an absolute directory path when registering a new file location.
class RemoteFolderPickerDialog : public QDialog {
    Q_OBJECT
public:
    explicit RemoteFolderPickerDialog(ApiClient *api, QWidget *parent = nullptr);

    QString selectedPath() const { return m_currentPath; }

private slots:
    void browseTo(const QString &path);
    void onItemActivated(QTreeWidgetItem *item, int column);

private:
    ApiClient *m_api;
    QTreeWidget *m_tree;
    QLabel *m_pathLabel;
    QDialogButtonBox *m_buttons;
    QString m_currentPath;
};
