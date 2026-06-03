#include "ui/RoutingRuleEditorDialog.h"

#include "domain/RoutingMode.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace zarya {

RoutingRuleEditorDialog::RoutingRuleEditorDialog(const RoutingRule& rule, bool readOnly,
                                                 QWidget* parent)
    : QDialog(parent)
    , m_rule(rule)
    , m_readOnly(readOnly)
{
    setWindowTitle(readOnly ? QStringLiteral("View Rule") : QStringLiteral("Edit Rule"));

    m_actionCombo = new QComboBox(this);
    m_actionCombo->addItem(routingActionDisplayString(RoutingAction::Proxy),
                           static_cast<int>(RoutingAction::Proxy));
    m_actionCombo->addItem(routingActionDisplayString(RoutingAction::Direct),
                           static_cast<int>(RoutingAction::Direct));
    m_actionCombo->addItem(routingActionDisplayString(RoutingAction::Block),
                           static_cast<int>(RoutingAction::Block));
    m_actionCombo->setCurrentIndex(m_actionCombo->findData(static_cast<int>(rule.action)));

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(routingRuleTypeDisplayString(RoutingRuleType::Domain),
                         static_cast<int>(RoutingRuleType::Domain));
    m_typeCombo->addItem(routingRuleTypeDisplayString(RoutingRuleType::Ip),
                         static_cast<int>(RoutingRuleType::Ip));
    m_typeCombo->addItem(routingRuleTypeDisplayString(RoutingRuleType::Port),
                         static_cast<int>(RoutingRuleType::Port));
    m_typeCombo->addItem(routingRuleTypeDisplayString(RoutingRuleType::Protocol),
                         static_cast<int>(RoutingRuleType::Protocol));
    m_typeCombo->setCurrentIndex(m_typeCombo->findData(static_cast<int>(rule.type)));

    m_valuesEdit = new QPlainTextEdit(this);
    m_valuesEdit->setPlaceholderText(
        QStringLiteral("One value per line\ngeosite:private\ndomain:example.com"));
    m_valuesEdit->setPlainText(rule.values.join(QStringLiteral("\n")));

    m_noteEdit = new QLineEdit(rule.note, this);

    if (readOnly) {
        m_actionCombo->setEnabled(false);
        m_typeCombo->setEnabled(false);
        m_valuesEdit->setReadOnly(true);
        m_noteEdit->setReadOnly(true);
    }

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Action"), m_actionCombo);
    form->addRow(QStringLiteral("Type"), m_typeCombo);
    form->addRow(QStringLiteral("Values"), m_valuesEdit);
    form->addRow(QStringLiteral("Note"), m_noteEdit);

    auto* buttons = new QDialogButtonBox(
        readOnly ? QDialogButtonBox::Close : (QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
        Qt::Horizontal, this);
    if (readOnly) {
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    } else {
        connect(buttons, &QDialogButtonBox::accepted, this, &RoutingRuleEditorDialog::onAccepted);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
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
    m_rule.action = static_cast<RoutingAction>(m_actionCombo->currentData().toInt());
    m_rule.type = static_cast<RoutingRuleType>(m_typeCombo->currentData().toInt());
    m_rule.note = m_noteEdit->text().trimmed();

    QStringList values;
    const QStringList lines = m_valuesEdit->toPlainText().split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            values.append(trimmed);
        }
    }
    if (values.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Rule"),
                             QStringLiteral("At least one value is required."));
        return false;
    }
    m_rule.values = values;
    return true;
}

} // namespace zarya
