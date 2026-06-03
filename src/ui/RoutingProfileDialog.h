#pragma once

#include "domain/RoutingProfile.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace zarya {

class RoutingProfileDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoutingProfileDialog(const RoutingProfile& profile, bool readOnly,
                                  QWidget* parent = nullptr);

    RoutingProfile profile() const;

private slots:
    void onAddRule();
    void onEditRule();
    void onDeleteRule();
    void onMoveUp();
    void onMoveDown();
    void onValidate();
    void onPreviewJson();
    void onDuplicate();

private:
    void refreshRulesTable();
    int selectedRuleRow() const;
    void setRules(const QVector<RoutingRule>& rules);

    RoutingProfile m_profile;
    bool m_readOnly = false;

    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_modeCombo = nullptr;
    QComboBox* m_domainStrategyCombo = nullptr;
    QCheckBox* m_enabledCheck = nullptr;
    QTableWidget* m_rulesTable = nullptr;
    QPushButton* m_duplicateButton = nullptr;
};

} // namespace zarya
