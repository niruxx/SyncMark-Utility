#pragma once

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QLabel;

// Add/edit dialog for a single bookmark. Does not talk to the network itself;
// the caller reads formData() and performs the POST/PUT.
class BookmarkEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit BookmarkEditDialog(const QStringList &existingFolders, QWidget *parent = nullptr);

    void setBookmark(const QJsonObject &bookmark);
    QJsonObject formData() const;

private slots:
    void validateAndAccept();

private:
    QLineEdit *m_title;
    QLineEdit *m_url;
    QComboBox *m_folder;
    QCheckBox *m_favorite;
    QLabel *m_error;
};
