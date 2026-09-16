#pragma once

#include "../common/PageWidget.h"
#include <QJsonObject>
#include <QVector>
#include <QDate>

class ApiClient;
class QCalendarWidget;
class QTableWidget;
class QLineEdit;

// Calendar module: a month calendar for picking a day plus a table of events
// on that day, backed by /api/events. Recurrence is edited but not expanded
// into individual occurrences client-side (each event shows its base time).
class CalendarPage : public PageWidget {
    Q_OBJECT
public:
    explicit CalendarPage(ApiClient *api, QWidget *parent = nullptr);
    void reload() override;

private slots:
    void reloadEvents();
    void applyDateFilter();
    void addEvent();
    void editSelectedEvent();
    void deleteSelectedEvent();
    void importEvents();
    void exportEvents();
    void onContextMenu(const QPoint &pos);

private:
    int selectedRow() const;
    void editEventAt(int row);
    void populateTableForDate(const QDate &date);
    void highlightEventDates();

    ApiClient *m_api;
    QCalendarWidget *m_calendar;
    QTableWidget *m_table;
    QLineEdit *m_search;
    QVector<QJsonObject> m_allEvents;
};
