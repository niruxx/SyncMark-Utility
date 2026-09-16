#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QVector>

class ApiClient;
class QListWidget;
class QWidget;
class QVBoxLayout;
class QLabel;

// Lists potential duplicate contact groups (GET /contacts/duplicates) and
// lets the user pick a primary contact to merge the others into, or dismiss
// a group as "not actually duplicates".
class DuplicatesDialog : public QDialog {
    Q_OBJECT
public:
    explicit DuplicatesDialog(ApiClient *api, QWidget *parent = nullptr);

signals:
    void contactsChanged();

private slots:
    void reloadGroups();
    void showGroup(int index);
    void mergeCurrentGroup();
    void dismissCurrentGroup();

private:
    static QVector<QJsonObject> contactsInGroup(const QJsonValue &groupValue);
    static QString contactLabel(const QJsonObject &c);

    ApiClient *m_api;
    QListWidget *m_groupList;
    QWidget *m_detailPanel;
    QVBoxLayout *m_detailLayout;
    QLabel *m_status;
    QVector<QJsonValue> m_groups;
    int m_currentGroup = -1;
};
