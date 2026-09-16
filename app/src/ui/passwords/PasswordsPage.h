#pragma once

#include "../common/PageWidget.h"
#include <QJsonObject>
#include <QVector>

class ApiClient;
class QTableWidget;
class QLineEdit;

// Password vault module. List responses never include the plaintext/
// encrypted password; it is fetched on demand via GET /passwords/:id (edit)
// or /passwords/:id/reveal (copy to clipboard).
class PasswordsPage : public PageWidget {
    Q_OBJECT
public:
    explicit PasswordsPage(ApiClient *api, QWidget *parent = nullptr);
    void reload() override;

private slots:
    void reloadEntries();
    void addEntry();
    void editSelectedEntry();
    void deleteSelectedEntry();
    void toggleSelectedFavorite();
    void copySelectedPassword();
    void copySelectedUsername();
    void importEntries();
    void exportEntries();
    void onContextMenu(const QPoint &pos);

private:
    int selectedRow() const;
    qlonglong idAt(int row) const;

    ApiClient *m_api;
    QTableWidget *m_table;
    QLineEdit *m_search;
    QVector<QJsonObject> m_entries;
};
