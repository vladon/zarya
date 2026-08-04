#include "ui/RoutingManagerAccessibility.h"

#include <QTableWidget>
#include <QWidget>

namespace zarya {

void configureRoutingManagerAccessibility(
    QTableWidget* table,
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
    initialFocus->setFocus(Qt::OtherFocusReason);
}

} // namespace zarya
