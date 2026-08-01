#pragma once

#include "domain/RoutingRule.h"

#include <QDialog>

namespace zarya {

class ZaryaSelector;
class ZaryaTextArea;
class ZaryaTextField;
class ZaryaValidationMessage;

class RoutingRuleEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoutingRuleEditorDialog(const RoutingRule& rule, bool readOnly, QWidget* parent = nullptr);

    RoutingRule rule() const;

private Q_SLOTS:
    void onAccepted();

private:
    bool validateRule();

    ZaryaSelector* m_actionCombo = nullptr;
    ZaryaSelector* m_typeCombo = nullptr;
    ZaryaTextArea* m_valuesEdit = nullptr;
    ZaryaTextField* m_noteEdit = nullptr;
    ZaryaValidationMessage* m_validationMessage = nullptr;
    RoutingRule m_rule;
    bool m_readOnly = false;
};

} // namespace zarya
