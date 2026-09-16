#pragma once

#include <QDialog>

class QComboBox;
class QLabel;
class QPushButton;

// First screen: asks for the SyncMark server's base URL, e.g. http://localhost:3000
class ConnectDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConnectDialog(QWidget *parent = nullptr);

    QString serverUrl() const;
    void setBusy(bool busy, const QString &message = {});
    void showError(const QString &message);

private:
    QComboBox *m_urlCombo;
    QLabel *m_statusLabel;
    QPushButton *m_connectButton;
    QPushButton *m_exitButton;
};
