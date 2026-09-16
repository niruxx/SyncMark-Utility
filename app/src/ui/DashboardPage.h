#pragma once

#include "common/PageWidget.h"

class ApiClient;
class Session;
class QLabel;

// Landing page: connection info plus the /api/stats overview counts.
class DashboardPage : public PageWidget {
    Q_OBJECT
public:
    DashboardPage(ApiClient *api, Session *session, QWidget *parent = nullptr);
    void reload() override;

private:
    ApiClient *m_api;
    Session *m_session;
    QLabel *m_welcomeLabel;
    QLabel *m_bookmarksStat;
    QLabel *m_foldersStat;
    QLabel *m_contactsStat;
    QLabel *m_eventsStat;
    QLabel *m_passwordsStat;
};
