#include "ui/SubscriptionManagerAccessibility.h"
#include "ui/ManagerAccessibility.h"

#include <QTableView>
#include <QWidget>

namespace zarya {

void configureSubscriptionManagerAccessibility(
    QTableView* table,
    const QVector<QWidget*>& actions,
    const QString& tableName)
{
    table->setAccessibleName(tableName);

    QWidget* previous = table;
    for (QWidget* action : actions) {
        QWidget::setTabOrder(previous, action);
        previous = action;
    }

    QWidget* initialFocus = !table->isHidden()
        ? static_cast<QWidget*>(table)
        : !actions.isEmpty() ? actions.front() : static_cast<QWidget*>(table);
    scheduleManagerInitialFocus(initialFocus);
}

} // namespace zarya
