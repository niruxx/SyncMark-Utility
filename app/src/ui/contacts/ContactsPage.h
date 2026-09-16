#pragma once

#include "../common/PageWidget.h"
#include <QJsonObject>
#include <QVector>

class ApiClient;
class QTableWidget;
class QLineEdit;
class QPushButton;

// Address book module: search, list, add/edit/delete, favorite toggle,
// photo upload/removal, import (CSV/vCard) and export (CSV/vCard).
class ContactsPage : public PageWidget {
    Q_OBJECT
public:
    explicit ContactsPage(ApiClient *api, QWidget *parent = nullptr);
    void reload() override;

private slots:
    void reloadContacts();
    void addContact();
    void editSelectedContact();
    void deleteSelectedContact();
    void toggleSelectedFavorite();
    void setPhotoForSelected();
    void removePhotoForSelected();
    void importContacts();
    void exportContacts(const QString &format);
    void onContextMenu(const QPoint &pos);

private:
    int selectedRow() const;
    void editContactAt(int row);

    ApiClient *m_api;
    QTableWidget *m_table;
    QLineEdit *m_search;
    QVector<QJsonObject> m_contacts;
};
