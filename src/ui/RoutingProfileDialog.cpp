#include "ui/RoutingProfileDialog.h"

#include "domain/RoutingMode.h"
#include "routing/RoutingProfileValidator.h"
#include "routing/XrayRoutingGenerator.h"
#include "ui/RoutingJsonPreviewDialog.h"
#include "ui/RoutingRuleEditorDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <QJsonDocument>

namespace zarya {

namespace {

QStringList domainStrategyOptions()
{
    return {QStringLiteral("AsIs"), QStringLiteral("IPIfNonMatch"), QStringLiteral("IPOnDemand")};
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

    m_nameEdit = new QLineEdit(profile.name, this);
    m_enabledCheck = new QCheckBox(tr("Enabled"), this);
    m_enabledCheck->setChecked(profile.enabled);

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(routingModeDisplayString(RoutingMode::ProxyAll),
                         static_cast<int>(RoutingMode::ProxyAll));
    m_modeCombo->addItem(routingModeDisplayString(RoutingMode::BypassLan),
                         static_cast<int>(RoutingMode::BypassLan));
    m_modeCombo->addItem(routingModeDisplayString(RoutingMode::BypassRu),
                         static_cast<int>(RoutingMode::BypassRu));
    m_modeCombo->addItem(routingModeDisplayString(RoutingMode::BypassLanAndRu),
                         static_cast<int>(RoutingMode::BypassLanAndRu));
    m_modeCombo->addItem(routingModeDisplayString(RoutingMode::Custom),
                         static_cast<int>(RoutingMode::Custom));
    m_modeCombo->setCurrentIndex(m_modeCombo->findData(static_cast<int>(profile.mode)));

    m_domainStrategyCombo = new QComboBox(this);
    for (const QString& strategy : domainStrategyOptions()) {
        m_domainStrategyCombo->addItem(strategy);
    }
    const int strategyIndex = m_domainStrategyCombo->findText(profile.domainStrategy);
    m_domainStrategyCombo->setCurrentIndex(strategyIndex >= 0 ? strategyIndex : 0);

    m_rulesTable = new QTableWidget(this);
    m_rulesTable->setColumnCount(5);
    m_rulesTable->setHorizontalHeaderLabels(
        {tr("Enabled"), tr("Action"), tr("Type"),
         tr("Values"), tr("Note")});
    m_rulesTable->horizontalHeader()->setStretchLastSection(true);
    m_rulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rulesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_rulesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    refreshRulesTable();

    auto* addRuleButton = new QPushButton(tr("Add Rule"), this);
    auto* editRuleButton = new QPushButton(tr("Edit Rule"), this);
    auto* deleteRuleButton = new QPushButton(tr("Delete Rule"), this);
    auto* moveUpButton = new QPushButton(tr("Move Up"), this);
    auto* moveDownButton = new QPushButton(tr("Move Down"), this);
    auto* validateButton = new QPushButton(tr("Validate"), this);
    auto* previewButton = new QPushButton(tr("Preview Xray Routing JSON"), this);
    m_duplicateButton = new QPushButton(tr("Duplicate Profile"), this);

    connect(addRuleButton, &QPushButton::clicked, this, &RoutingProfileDialog::onAddRule);
    connect(editRuleButton, &QPushButton::clicked, this, &RoutingProfileDialog::onEditRule);
    connect(deleteRuleButton, &QPushButton::clicked, this, &RoutingProfileDialog::onDeleteRule);
    connect(moveUpButton, &QPushButton::clicked, this, &RoutingProfileDialog::onMoveUp);
    connect(moveDownButton, &QPushButton::clicked, this, &RoutingProfileDialog::onMoveDown);
    connect(validateButton, &QPushButton::clicked, this, &RoutingProfileDialog::onValidate);
    connect(previewButton, &QPushButton::clicked, this, &RoutingProfileDialog::onPreviewJson);
    connect(m_duplicateButton, &QPushButton::clicked, this, &RoutingProfileDialog::onDuplicate);

    auto* ruleButtons = new QHBoxLayout;
    ruleButtons->addWidget(addRuleButton);
    ruleButtons->addWidget(editRuleButton);
    ruleButtons->addWidget(deleteRuleButton);
    ruleButtons->addWidget(moveUpButton);
    ruleButtons->addWidget(moveDownButton);
    ruleButtons->addStretch();
    ruleButtons->addWidget(validateButton);
    ruleButtons->addWidget(previewButton);

    auto* rulesGroup = new QGroupBox(tr("Rules"), this);
    auto* rulesLayout = new QVBoxLayout(rulesGroup);
    rulesLayout->addWidget(m_rulesTable);
    rulesLayout->addLayout(ruleButtons);

    auto* form = new QFormLayout;
    form->addRow(tr("Name"), m_nameEdit);
    form->addRow(tr("Mode"), m_modeCombo);
    form->addRow(tr("Domain strategy"), m_domainStrategyCombo);
    form->addRow(QString(), m_enabledCheck);

    auto* buttons = new QDialogButtonBox(
        readOnly ? QDialogButtonBox::Close : (QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
        Qt::Horizontal, this);
    if (readOnly) {
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        m_duplicateButton->setVisible(true);
    } else {
        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            m_profile.name = m_nameEdit->text().trimmed();
            if (m_profile.name.isEmpty()) {
                QMessageBox::warning(this, tr("Profile"),
                                     tr("Name is required."));
                return;
            }
            m_profile.mode = static_cast<RoutingMode>(m_modeCombo->currentData().toInt());
            m_profile.domainStrategy = m_domainStrategyCombo->currentText();
            m_profile.enabled = m_enabledCheck->isChecked();
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
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

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(rulesGroup, 1);
    if (readOnly) {
        layout->addWidget(m_duplicateButton);
    }
    layout->addWidget(buttons);
}

RoutingProfile RoutingProfileDialog::profile() const
{
    return m_profile;
}

void RoutingProfileDialog::refreshRulesTable()
{
    m_rulesTable->setRowCount(m_profile.rules.size());
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
        QMessageBox::information(this, tr("Rules"),
                                 tr("Select a rule to edit."));
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
        QMessageBox::information(this, tr("Validation"),
                                 tr("No validation warnings."));
        return;
    }
    QMessageBox::warning(this, tr("Validation"), warnings.join(QStringLiteral("\n")));
}

void RoutingProfileDialog::onPreviewJson()
{
    RoutingProfile previewProfile = m_profile;
    previewProfile.name = m_nameEdit->text().trimmed();
    previewProfile.mode = static_cast<RoutingMode>(m_modeCombo->currentData().toInt());
    previewProfile.domainStrategy = m_domainStrategyCombo->currentText();
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

} // namespace zarya
