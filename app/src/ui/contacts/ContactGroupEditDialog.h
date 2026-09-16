#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>

class QLineEdit;
class QRadioButton;
class QLabel;
class RepeatingTableEditor;

// Add/edit dialog for a contact group. Manual groups just have a name;
// smart groups additionally define up to 10 rules (field/operator/value)
// evaluated server-side against SMART_FIELDS = tag, organization, title,
// favorite, addedWithinDays.
class ContactGroupEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit ContactGroupEditDialog(QWidget *parent = nullptr);

    void setGroup(const QJsonObject &group);
    QString name() const;
    QString type() const; // "manual" or "smart"
    QJsonArray smartRules() const;

private slots:
    void updateRulesEnabled();
    void validateAndAccept();

private:
    QLineEdit *m_name;
    QRadioButton *m_manualRadio;
    QRadioButton *m_smartRadio;
    RepeatingTableEditor *m_rules;
    QLabel *m_error;
};
