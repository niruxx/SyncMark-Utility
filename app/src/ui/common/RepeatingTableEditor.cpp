#include "RepeatingTableEditor.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

RepeatingTableEditor::RepeatingTableEditor(const QStringList &columnHeaders, QWidget *parent)
    : QWidget(parent), m_headers(columnHeaders) {
    m_columnChoices.resize(m_headers.size());

    m_table = new QTableWidget(this);
    m_table->setColumnCount(m_headers.size());
    m_table->setHorizontalHeaderLabels(m_headers);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i < m_headers.size(); ++i)
        m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setMinimumHeight(90);
    m_table->setMaximumHeight(160);

    auto *addBtn = new QPushButton(tr("+ Add"), this);
    connect(addBtn, &QPushButton::clicked, this, &RepeatingTableEditor::addRow);
    auto *removeBtn = new QPushButton(tr("- Remove"), this);
    connect(removeBtn, &QPushButton::clicked, this, &RepeatingTableEditor::removeSelectedRow);

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(addBtn);
    btnRow->addWidget(removeBtn);
    btnRow->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table);
    layout->addLayout(btnRow);
}

void RepeatingTableEditor::setColumnPlaceholder(int column, const QString &placeholder) {
    if (column < 0 || column >= m_table->columnCount()) return;
    m_table->setProperty(QStringLiteral("placeholder%1").arg(column).toUtf8().constData(), placeholder);
}

void RepeatingTableEditor::setColumnChoices(int column, const QStringList &choices) {
    if (column < 0 || column >= m_columnChoices.size()) return;
    m_columnChoices[column] = choices;
}

void RepeatingTableEditor::appendRow(const QStringList &values) {
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    for (int col = 0; col < m_headers.size(); ++col) {
        const QString value = col < values.size() ? values[col] : QString();
        if (!m_columnChoices[col].isEmpty()) {
            auto *combo = new QComboBox(m_table);
            combo->setEditable(true);
            combo->addItems(m_columnChoices[col]);
            combo->setCurrentText(value);
            m_table->setCellWidget(row, col, combo);
        } else {
            auto *edit = new QLineEdit(value, m_table);
            const QVariant placeholder = m_table->property(QStringLiteral("placeholder%1").arg(col).toUtf8().constData());
            if (placeholder.isValid())
                edit->setPlaceholderText(placeholder.toString());
            m_table->setCellWidget(row, col, edit);
        }
    }
}

void RepeatingTableEditor::addRow() {
    appendRow({});
}

void RepeatingTableEditor::removeSelectedRow() {
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        if (m_table->rowCount() > 0)
            m_table->removeRow(m_table->rowCount() - 1);
        return;
    }
    m_table->removeRow(rows.first().row());
}

void RepeatingTableEditor::setRows(const QVector<QStringList> &rows) {
    m_table->setRowCount(0);
    for (const QStringList &row : rows)
        appendRow(row);
}

QVector<QStringList> RepeatingTableEditor::rows() const {
    QVector<QStringList> result;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QStringList values;
        bool allEmpty = true;
        for (int col = 0; col < m_headers.size(); ++col) {
            QString value;
            if (auto *combo = qobject_cast<QComboBox *>(m_table->cellWidget(row, col)))
                value = combo->currentText().trimmed();
            else if (auto *edit = qobject_cast<QLineEdit *>(m_table->cellWidget(row, col)))
                value = edit->text().trimmed();
            if (!value.isEmpty()) allEmpty = false;
            values << value;
        }
        if (!allEmpty)
            result << values;
    }
    return result;
}
