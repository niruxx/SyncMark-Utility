#pragma once

#include <QWidget>
#include <QStringList>
#include <QVector>

class QTableWidget;
class QPushButton;

// A small reusable "list of rows" editor: a table with N text columns plus
// Add/Remove Row buttons. Used for the various free-form array fields on a
// contact (addresses, social profiles, messaging handles, custom fields,
// key dates, emails, phones, ...). Each row is a plain QStringList aligned
// with the configured column headers; empty trailing rows are dropped when
// reading rows() back out.
class RepeatingTableEditor : public QWidget {
    Q_OBJECT
public:
    explicit RepeatingTableEditor(const QStringList &columnHeaders, QWidget *parent = nullptr);

    void setRows(const QVector<QStringList> &rows);
    QVector<QStringList> rows() const;

    void setColumnPlaceholder(int column, const QString &placeholder);
    // Restrict a column's cell editor to a fixed set of choices via combo box.
    void setColumnChoices(int column, const QStringList &choices);

private slots:
    void addRow();
    void removeSelectedRow();

private:
    void appendRow(const QStringList &values);

    QTableWidget *m_table;
    QStringList m_headers;
    QVector<QStringList> m_columnChoices;
};
