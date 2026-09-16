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
// on that day, backed by /api/events. Simple recurrence (daily/weekly/monthly,
// optionally with an UNTIL date) is expanded client-side into occurrences
// for the day list and the month highlight.
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
    struct Occurrence {
        QJsonObject event;
        QDateTime start;
        QDateTime end;
    };

    int selectedRow() const;
    void editEventAt(int row);
    void populateTableForDate(const QDate &date);
    void highlightEventDates();
    // If `event` occurs (base or recurring) on `date`, fills start/end and returns true.
    static bool occurrenceOnDate(const QJsonObject &event, const QDate &date, QDateTime &start, QDateTime &end);

    ApiClient *m_api;
    QCalendarWidget *m_calendar;
    QTableWidget *m_table;
    QLineEdit *m_search;
    QVector<QJsonObject> m_allEvents;
};
