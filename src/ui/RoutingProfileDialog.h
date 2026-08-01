#pragma once

#include "domain/RoutingProfile.h"

#include <QDialog>

class QTableWidget;

namespace zarya {

class ZaryaActionButton;
class ZaryaBodyText;
class ZaryaCheckBox;
class ZaryaSelector;
class ZaryaTextField;
class ZaryaValidationMessage;

class RoutingProfileDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoutingProfileDialog(const RoutingProfile& profile, bool readOnly,
                                  QWidget* parent = nullptr);

    RoutingProfile profile() const;

private Q_SLOTS:
    void onAddRule();
    void onEditRule();
    void onDeleteRule();
    void onMoveUp();
    void onMoveDown();
    void onValidate();
    void onPreviewJson();
    void onDuplicate();
    void tryAccept();

private:
    void refreshRulesTable();
    int selectedRuleRow() const;
    void setRules(const QVector<RoutingRule>& rules);

    RoutingProfile m_profile;
    bool m_readOnly = false;

    ZaryaTextField* m_nameEdit = nullptr;
    ZaryaSelector* m_modeCombo = nullptr;
    ZaryaSelector* m_domainStrategyCombo = nullptr;
    ZaryaCheckBox* m_enabledCheck = nullptr;
    QTableWidget* m_rulesTable = nullptr;
    ZaryaActionButton* m_duplicateButton = nullptr;
    ZaryaBodyText* m_emptyRules = nullptr;
    ZaryaValidationMessage* m_validationMessage = nullptr;
};

} // namespace zarya
