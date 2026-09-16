#include "ContactGroupEditDialog.h"
#include "../common/RepeatingTableEditor.h"

#include <QLineEdit>
#include <QRadioButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QButtonGroup>
#include <QJsonDocument>

ContactGroupEditDialog::ContactGroupEditDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Contact Group"));
    setMinimumWidth(460);

    m_name = new QLineEdit(this);

    m_manualRadio = new QRadioButton(tr("Manual (pick members yourself)"), this);
    m_smartRadio = new QRadioButton(tr("Smart (auto-match by rules)"), this);
    m_manualRadio->setChecked(true);
    auto *typeGroup = new QButtonGroup(this);
    typeGroup->addButton(m_manualRadio);
    typeGroup->addButton(m_smartRadio);
    connect(m_manualRadio, &QRadioButton::toggled, this, &ContactGroupEditDialog::updateRulesEnabled);

    auto *typeRow = new QVBoxLayout;
    typeRow->addWidget(m_manualRadio);
    typeRow->addWidget(m_smartRadio);

    m_rules = new RepeatingTableEditor({tr("Field"), tr("Operator"), tr("Value")}, this);
    m_rules->setColumnChoices(0, {"tag", "organization", "title", "favorite", "addedWithinDays"});
    m_rules->setColumnChoices(1, {"equals", "not_equals", "contains", "greater_than", "less_than"});

    auto *form = new QFormLayout;
    form->addRow(tr("Name:"), m_name);

    m_error = new QLabel(this);
    m_error->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ContactGroupEditDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(typeRow);
    layout->addWidget(new QLabel(tr("Match contacts where (up to 10 rules, all must match):")));
    layout->addWidget(m_rules);
    layout->addWidget(m_error);
    layout->addWidget(buttons);

    updateRulesEnabled();
}

void ContactGroupEditDialog::updateRulesEnabled() {
    m_rules->setEnabled(m_smartRadio->isChecked());
}

void ContactGroupEditDialog::setGroup(const QJsonObject &group) {
    m_name->setText(group.value("name").toString());
    const QString t = group.value("type").toString();
    m_smartRadio->setChecked(t == "smart");
    m_manualRadio->setChecked(t != "smart");

    QJsonArray rules = group.value("smartRules").toArray();
    if (rules.isEmpty()) {
        const QString raw = group.value("smart_rules").toString();
        if (!raw.isEmpty())
            rules = QJsonDocument::fromJson(raw.toUtf8()).array();
    }
    QVector<QStringList> rows;
    for (const QJsonValue &v : rules) {
        const QJsonObject o = v.toObject();
        rows << QStringList{o.value("field").toString(), o.value("operator").toString("equals"),
                             o.value("value").toVariant().toString()};
    }
    m_rules->setRows(rows);
    updateRulesEnabled();
}

QString ContactGroupEditDialog::name() const { return m_name->text().trimmed(); }
QString ContactGroupEditDialog::type() const { return m_smartRadio->isChecked() ? "smart" : "manual"; }

QJsonArray ContactGroupEditDialog::smartRules() const {
    QJsonArray rules;
    for (const QStringList &row : m_rules->rows()) {
        if (row.size() < 3 || row[0].isEmpty()) continue;
        rules.append(QJsonObject{{"field", row[0]}, {"operator", row[1].isEmpty() ? "equals" : row[1]}, {"value", row[2]}});
    }
    return rules;
}

void ContactGroupEditDialog::validateAndAccept() {
    if (m_name->text().trimmed().isEmpty()) {
        m_error->setText(tr("Name is required."));
        return;
    }
    accept();
}
