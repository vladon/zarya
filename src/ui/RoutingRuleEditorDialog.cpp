#include "ui/RoutingRuleEditorDialog.h"

#include "domain/RoutingMode.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"

#include <QTimer>
#include <QVBoxLayout>

namespace zarya {

RoutingRuleEditorDialog::RoutingRuleEditorDialog(const RoutingRule& rule, bool readOnly,
                                                 QWidget* parent)
    : QDialog(parent)
    , m_rule(rule)
    , m_readOnly(readOnly)
{
    setWindowTitle(readOnly ? tr("View Rule") : tr("Edit Rule"));

    m_actionCombo = new ZaryaSelector(this);
    m_actionCombo->setItems(
        {
            {QString::number(static_cast<int>(RoutingAction::Proxy)),
             routingActionDisplayString(RoutingAction::Proxy), true},
            {QString::number(static_cast<int>(RoutingAction::Direct)),
             routingActionDisplayString(RoutingAction::Direct), true},
            {QString::number(static_cast<int>(RoutingAction::Block)),
             routingActionDisplayString(RoutingAction::Block), true},
        },
        QString::number(static_cast<int>(rule.action)));

    m_typeCombo = new ZaryaSelector(this);
    m_typeCombo->setItems(
        {
            {QString::number(static_cast<int>(RoutingRuleType::Domain)),
             routingRuleTypeDisplayString(RoutingRuleType::Domain), true},
            {QString::number(static_cast<int>(RoutingRuleType::Ip)),
             routingRuleTypeDisplayString(RoutingRuleType::Ip), true},
            {QString::number(static_cast<int>(RoutingRuleType::Port)),
             routingRuleTypeDisplayString(RoutingRuleType::Port), true},
            {QString::number(static_cast<int>(RoutingRuleType::Protocol)),
             routingRuleTypeDisplayString(RoutingRuleType::Protocol), true},
        },
        QString::number(static_cast<int>(rule.type)));

    m_valuesEdit = new ZaryaTextArea(
        tr("One value per line\ngeosite:private\ndomain:example.com"), this, 150);
    m_valuesEdit->setText(rule.values.join(QStringLiteral("\n")));

    m_noteEdit = new ZaryaTextField(tr("Note"), this);
    m_noteEdit->setText(rule.note);
    m_validationMessage = new ZaryaValidationMessage(this);

    if (readOnly) {
        m_actionCombo->setEnabled(false);
        m_typeCombo->setEnabled(false);
        m_valuesEdit->setReadOnly(true);
        m_noteEdit->setReadOnly(true);
    }

    QWidget* buttons = nullptr;
    if (readOnly) {
        auto* close = new ZaryaActionButton(tr("Close"), this);
        connect(close, &ZaryaActionButton::clicked, this, &QDialog::reject);
        buttons = close;
    } else {
        auto* actions = new ZaryaDialogActionRow(tr("OK"), tr("Cancel"), this);
        connect(actions, &ZaryaDialogActionRow::accepted,
                this, &RoutingRuleEditorDialog::onAccepted);
        connect(actions, &ZaryaDialogActionRow::rejected, this, &QDialog::reject);
        buttons = actions;
    }

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(new ZaryaFormRow(tr("Action"), m_actionCombo, this));
    layout->addWidget(new ZaryaFormRow(tr("Type"), m_typeCombo, this));
    layout->addWidget(new ZaryaFormRow(tr("Values"), m_valuesEdit, this));
    layout->addWidget(new ZaryaFormRow(tr("Note"), m_noteEdit, this));
    layout->addWidget(m_validationMessage);
    layout->addWidget(buttons);
    resize(620, 430);

    QTimer::singleShot(0, m_actionCombo, [this] {
        m_actionCombo->setFocus(Qt::OtherFocusReason);
    });
}

RoutingRule RoutingRuleEditorDialog::rule() const
{
    return m_rule;
}

void RoutingRuleEditorDialog::onAccepted()
{
    if (!validateRule()) {
        return;
    }
    accept();
}

bool RoutingRuleEditorDialog::validateRule()
{
    m_validationMessage->clear();
    m_rule.action = static_cast<RoutingAction>(m_actionCombo->currentKey().toInt());
    m_rule.type = static_cast<RoutingRuleType>(m_typeCombo->currentKey().toInt());
    m_rule.note = m_noteEdit->text().trimmed();

    QStringList values;
    const QStringList lines = m_valuesEdit->text().split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            values.append(trimmed);
        }
    }
    if (values.isEmpty()) {
        m_valuesEdit->setFocus(Qt::OtherFocusReason);
        m_validationMessage->showMessage(tr("At least one value is required."));
        return false;
    }
    m_rule.values = values;
    return true;
}

} // namespace zarya
