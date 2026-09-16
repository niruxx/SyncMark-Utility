#pragma once

#include <QDialog>

class ApiClient;
class QCheckBox;
class QLabel;

// Admin: enable/disable server modules (GET/PUT /api/features). Disabled
// modules' routes 404 server-side, so this also determines what the client
// shows.
class FeatureTogglesDialog : public QDialog {
    Q_OBJECT
public:
    explicit FeatureTogglesDialog(ApiClient *api, QWidget *parent = nullptr);

signals:
    void featuresChanged();

private slots:
    void load();
    void save();

private:
    ApiClient *m_api;
    QCheckBox *m_bookmarks;
    QCheckBox *m_contacts;
    QCheckBox *m_calendar;
    QCheckBox *m_files;
    QCheckBox *m_passwords;
    QLabel *m_status;
};
