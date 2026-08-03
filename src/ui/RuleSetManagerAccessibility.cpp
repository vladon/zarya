#include "ui/RuleSetManagerAccessibility.h"

#include <QPlainTextEdit>
#include <QTableWidget>
#include <QWidget>

namespace zarya {

void configureRuleSetManagerAccessibility(
    QTableWidget* requiredTable,
    QTableWidget* allTable,
    QPlainTextEdit* logView,
    const QVector<QWidget*>& actions,
    const QString& requiredTableName,
    const QString& allTableName,
    const QString& logName)
{
    requiredTable->setAccessibleName(requiredTableName);
    allTable->setAccessibleName(allTableName);
    logView->setAccessibleName(logName);

    QWidget::setTabOrder(requiredTable, allTable);
    QWidget::setTabOrder(allTable, logView);
    QWidget* previous = logView;
    for (QWidget* action : actions) {
        QWidget::setTabOrder(previous, action);
        previous = action;
    }

    QWidget* initialFocus = !requiredTable->isHidden()
        ? static_cast<QWidget*>(requiredTable)
        : !allTable->isHidden()
            ? static_cast<QWidget*>(allTable)
            : !actions.isEmpty() ? actions.front() : static_cast<QWidget*>(logView);
    initialFocus->setFocus(Qt::OtherFocusReason);
}

} // namespace zarya
