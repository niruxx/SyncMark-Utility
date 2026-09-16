#pragma once

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QPlainTextEdit;
class QDateTimeEdit;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;

// Add/edit dialog for a calendar event, including simple recurrence
// (none/daily/weekly/monthly with an optional "until" date).
class EventEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit EventEditDialog(QWidget *parent = nullptr);

    void setEvent(const QJsonObject &event);
    QJsonObject formData() const;

private slots:
    void validateAndAccept();
    void updateRecurrenceState();

private:
    QLineEdit *m_title;
    QLineEdit *m_location;
    QPlainTextEdit *m_description;
    QDateTimeEdit *m_start;
    QDateTimeEdit *m_end;
    QCheckBox *m_allDay;
    QComboBox *m_recurrence;
    QDateEdit *m_until;
    QLabel *m_error;
};
