#pragma once

#include "domain/RoutingRule.h"

#include <QDialog>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;

namespace zarya {

class RoutingRuleEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoutingRuleEditorDialog(const RoutingRule& rule, bool readOnly, QWidget* parent = nullptr);

    RoutingRule rule() const;

private slots:
    void onAccepted();

private:
    bool validateRule();

    QComboBox* m_actionCombo = nullptr;
    QComboBox* m_typeCombo = nullptr;
    QPlainTextEdit* m_valuesEdit = nullptr;
    QLineEdit* m_noteEdit = nullptr;
    RoutingRule m_rule;
    bool m_readOnly = false;
};

} // namespace zarya
