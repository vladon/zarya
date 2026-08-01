#include "ui/RoutingProfileDialog.h"

#include "base/algorithm.h"
#include "domain/RoutingMode.h"
#include "routing/RoutingProfileValidator.h"
#include "routing/XrayRoutingGenerator.h"
#include "ui/RoutingJsonPreviewDialog.h"
#include "ui/RoutingRuleEditorDialog.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaControls.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

#include <QJsonDocument>

namespace zarya {

namespace {

QStringList domainStrategyOptions()
{
    return {QStringLiteral("AsIs"), QStringLiteral("IPIfNonMatch"), QStringLiteral("IPOnDemand")};
}

QString enumKey(int value)
{
    return QString::number(value);
}

} // namespace

RoutingProfileDialog::RoutingProfileDialog(const RoutingProfile& profile, bool readOnly,
                                             QWidget* parent)
    : QDialog(parent)
    , m_profile(profile)
    , m_readOnly(readOnly)
{
    setWindowTitle(readOnly ? tr("View Routing Profile")
                            : tr("Edit Routing Profile"));
    resize(760, 520);

    m_nameEdit = new ZaryaTextField(tr("Name"), this);
    m_nameEdit->setText(profile.name);
    m_enabledCheck = new ZaryaCheckBox(tr("Enabled"), this, profile.enabled);

    m_modeCombo = new ZaryaSelector(this);
    m_modeCombo->setItems({
        {enumKey(static_cast<int>(RoutingMode::ProxyAll)),
         routingModeDisplayString(RoutingMode::ProxyAll)},
        {enumKey(static_cast<int>(RoutingMode::BypassLan)),
         routingModeDisplayString(RoutingMode::BypassLan)},
        {enumKey(static_cast<int>(RoutingMode::BypassRu)),
         routingModeDisplayString(RoutingMode::BypassRu)},
        {enumKey(static_cast<int>(RoutingMode::BypassLanAndRu)),
         routingModeDisplayString(RoutingMode::BypassLanAndRu)},
        {enumKey(static_cast<int>(RoutingMode::Custom)),
         routingModeDisplayString(RoutingMode::Custom)},
    }, enumKey(static_cast<int>(profile.mode)));

    m_domainStrategyCombo = new ZaryaSelector(this);
    QVector<ZaryaSelectorItem> strategyItems;
    for (const QString& strategy : domainStrategyOptions()) {
        strategyItems.push_back({strategy, strategy});
    }
    m_domainStrategyCombo->setItems(
        std::move(strategyItems),
        domainStrategyOptions().contains(profile.domainStrategy)
            ? profile.domainStrategy
            : QStringLiteral("AsIs"));

    m_rulesTable = new QTableWidget(this);
    m_rulesTable->setColumnCount(5);
    m_rulesTable->setHorizontalHeaderLabels(
        {tr("Enabled"), tr("Action"), tr("Type"),
         tr("Values"), tr("Note")});
    m_rulesTable->horizontalHeader()->setStretchLastSection(true);
    m_rulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rulesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_rulesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto* addRuleButton = new ZaryaActionButton(tr("Add Rule"), this, ZaryaButtonRole::Primary);
    auto* editRuleButton = new ZaryaActionButton(tr("Edit Rule"), this);
    auto* deleteRuleButton = new ZaryaActionButton(
        tr("Delete Rule"), this, ZaryaButtonRole::Destructive);
    auto* moveUpButton = new ZaryaActionButton(tr("Move Up"), this);
    auto* moveDownButton = new ZaryaActionButton(tr("Move Down"), this);
    auto* validateButton = new ZaryaActionButton(tr("Validate"), this);
    auto* previewButton = new ZaryaActionButton(tr("Preview Xray Routing JSON"), this);
    m_duplicateButton = new ZaryaActionButton(tr("Duplicate Profile"), this);

    connect(addRuleButton, &ZaryaActionButton::clicked, this, &RoutingProfileDialog::onAddRule);
    connect(editRuleButton, &ZaryaActionButton::clicked, this, &RoutingProfileDialog::onEditRule);
    connect(deleteRuleButton, &ZaryaActionButton::clicked, this, &RoutingProfileDialog::onDeleteRule);
    connect(moveUpButton, &ZaryaActionButton::clicked, this, &RoutingProfileDialog::onMoveUp);
    connect(moveDownButton, &ZaryaActionButton::clicked, this, &RoutingProfileDialog::onMoveDown);
    connect(validateButton, &ZaryaActionButton::clicked, this, &RoutingProfileDialog::onValidate);
    connect(previewButton, &ZaryaActionButton::clicked, this, &RoutingProfileDialog::onPreviewJson);
    connect(m_duplicateButton, &ZaryaActionButton::clicked, this, &RoutingProfileDialog::onDuplicate);

    auto* ruleButtons = new QHBoxLayout;
    ruleButtons->addWidget(addRuleButton);
    ruleButtons->addWidget(editRuleButton);
    ruleButtons->addWidget(deleteRuleButton);
    ruleButtons->addWidget(moveUpButton);
    ruleButtons->addWidget(moveDownButton);
    ruleButtons->addStretch();
    ruleButtons->addWidget(validateButton);
    ruleButtons->addWidget(previewButton);

    auto* ruleActions = new QWidget(this);
    ruleActions->setLayout(ruleButtons);
    m_emptyRules = new ZaryaBodyText(
        tr("No routing rules yet. Add a rule to build a custom route."), this);
    auto* rulesSection = new ZaryaFormSection(tr("Rules"), this);
    rulesSection->addWidget(m_emptyRules);
    rulesSection->addWidget(m_rulesTable);
    rulesSection->addWidget(ruleActions);

    QWidget* actions = nullptr;
    if (readOnly) {
        auto* closeButton = new ZaryaActionButton(tr("Close"), this, ZaryaButtonRole::Primary);
        connect(closeButton, &ZaryaActionButton::clicked, this, &QDialog::reject);
        actions = closeButton;
        m_duplicateButton->setVisible(true);
    } else {
        auto* actionRow = new ZaryaDialogActionRow(tr("Save"), tr("Cancel"), this);
        connect(actionRow, &ZaryaDialogActionRow::accepted, this, &RoutingProfileDialog::tryAccept);
        connect(actionRow, &ZaryaDialogActionRow::rejected, this, &QDialog::reject);
        actions = actionRow;
        m_duplicateButton->setVisible(false);
    }

    if (readOnly) {
        m_nameEdit->setReadOnly(true);
        m_modeCombo->setEnabled(false);
        m_domainStrategyCombo->setEnabled(false);
        m_enabledCheck->setEnabled(false);
        addRuleButton->setEnabled(false);
        editRuleButton->setEnabled(true);
        deleteRuleButton->setEnabled(false);
        moveUpButton->setEnabled(false);
        moveDownButton->setEnabled(false);
    }

    m_validationMessage = new ZaryaValidationMessage(this);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(new ZaryaFormRow(tr("Name"), m_nameEdit, this));
    layout->addWidget(new ZaryaFormRow(tr("Mode"), m_modeCombo, this));
    layout->addWidget(new ZaryaFormRow(tr("Domain strategy"), m_domainStrategyCombo, this));
    layout->addWidget(m_enabledCheck);
    layout->addWidget(rulesSection, 1);
    if (readOnly) {
        layout->addWidget(m_duplicateButton);
    }
    layout->addWidget(m_validationMessage);
    layout->addWidget(actions);
    refreshRulesTable();
}

RoutingProfile RoutingProfileDialog::profile() const
{
    return m_profile;
}

void RoutingProfileDialog::refreshRulesTable()
{
    m_rulesTable->setRowCount(m_profile.rules.size());
    m_emptyRules->setVisible(m_profile.rules.isEmpty());
    m_rulesTable->setVisible(!m_profile.rules.isEmpty());
    for (int row = 0; row < m_profile.rules.size(); ++row) {
        const RoutingRule& rule = m_profile.rules[row];
        m_rulesTable->setItem(row, 0,
                              new QTableWidgetItem(rule.enabled ? tr("Yes")
                                                                : tr("No")));
        m_rulesTable->setItem(row, 1,
                              new QTableWidgetItem(routingActionDisplayString(rule.action)));
        m_rulesTable->setItem(row, 2,
                              new QTableWidgetItem(routingRuleTypeDisplayString(rule.type)));
        m_rulesTable->setItem(row, 3, new QTableWidgetItem(rule.values.join(QStringLiteral(", "))));
        m_rulesTable->setItem(row, 4, new QTableWidgetItem(rule.note));
    }
}

int RoutingProfileDialog::selectedRuleRow() const
{
    const QList<QTableWidgetItem*> selected = m_rulesTable->selectedItems();
    if (selected.isEmpty()) {
        return -1;
    }
    return selected.first()->row();
}

void RoutingProfileDialog::setRules(const QVector<RoutingRule>& rules)
{
    m_profile.rules = rules;
    refreshRulesTable();
}

void RoutingProfileDialog::onAddRule()
{
    RoutingRuleEditorDialog dialog(RoutingRule::createDefault(), false, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QVector<RoutingRule> rules = m_profile.rules;
    rules.append(dialog.rule());
    setRules(rules);
}

void RoutingProfileDialog::onEditRule()
{
    const int row = selectedRuleRow();
    if (row < 0) {
        UiMessagePresenter::information(
            this, tr("Rules"), tr("Select a rule to edit."));
        return;
    }
    RoutingRuleEditorDialog dialog(m_profile.rules[row], m_readOnly, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    if (m_readOnly) {
        return;
    }
    QVector<RoutingRule> rules = m_profile.rules;
    rules[row] = dialog.rule();
    setRules(rules);
}

void RoutingProfileDialog::onDeleteRule()
{
    const int row = selectedRuleRow();
    if (row < 0) {
        return;
    }
    QVector<RoutingRule> rules = m_profile.rules;
    rules.removeAt(row);
    setRules(rules);
}

void RoutingProfileDialog::onMoveUp()
{
    const int row = selectedRuleRow();
    if (row <= 0) {
        return;
    }
    QVector<RoutingRule> rules = m_profile.rules;
    rules.swapItemsAt(row, row - 1);
    setRules(rules);
    m_rulesTable->selectRow(row - 1);
}

void RoutingProfileDialog::onMoveDown()
{
    const int row = selectedRuleRow();
    if (row < 0 || row >= m_profile.rules.size() - 1) {
        return;
    }
    QVector<RoutingRule> rules = m_profile.rules;
    rules.swapItemsAt(row, row + 1);
    setRules(rules);
    m_rulesTable->selectRow(row + 1);
}

void RoutingProfileDialog::onValidate()
{
    const QStringList warnings = RoutingProfileValidator::warnings(m_profile);
    if (warnings.isEmpty()) {
        UiMessagePresenter::information(
            this, tr("Validation"), tr("No validation warnings."));
        return;
    }
    UiMessagePresenter::warning(
        this, tr("Validation"), warnings.join(QStringLiteral("\n")));
}

void RoutingProfileDialog::onPreviewJson()
{
    RoutingProfile previewProfile = m_profile;
    previewProfile.name = m_nameEdit->text().trimmed();
    previewProfile.mode = static_cast<RoutingMode>(m_modeCombo->currentKey().toInt());
    previewProfile.domainStrategy = m_domainStrategyCombo->currentKey();
    previewProfile.enabled = m_enabledCheck->isChecked();

    const XrayRoutingGenerator generator;
    const QJsonObject routing = generator.generate(previewProfile);
    const QString json =
        QString::fromUtf8(QJsonDocument(routing).toJson(QJsonDocument::Indented));
    RoutingJsonPreviewDialog previewDialog(json, this);
    previewDialog.exec();
}

void RoutingProfileDialog::onDuplicate()
{
    done(2);
}

void RoutingProfileDialog::tryAccept()
{
    m_profile.name = m_nameEdit->text().trimmed();
    if (m_profile.name.isEmpty()) {
        m_nameEdit->showError();
        m_validationMessage->showMessage(tr("Name is required."));
        return;
    }
    m_nameEdit->showError(false);
    m_validationMessage->clear();
    m_profile.mode = static_cast<RoutingMode>(m_modeCombo->currentKey().toInt());
    m_profile.domainStrategy = m_domainStrategyCombo->currentKey();
    m_profile.enabled = m_enabledCheck->isChecked();
    accept();
}

} // namespace zarya
