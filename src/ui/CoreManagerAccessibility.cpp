#include "ui/CoreManagerAccessibility.h"
#include "ui/ManagerAccessibility.h"

#include <QPlainTextEdit>
#include <QTableWidget>
#include <QWidget>

namespace zarya {

void configureCoreManagerAccessibility(
    QTableWidget* table,
    QPlainTextEdit* logView,
    const QVector<QWidget*>& actions,
    const QString& tableName,
    const QString& logName)
{
    table->setAccessibleName(tableName);
    logView->setAccessibleName(logName);

    QWidget* previous = table;
    for (QWidget* action : actions) {
        QWidget::setTabOrder(previous, action);
        previous = action;
    }
    QWidget::setTabOrder(previous, logView);
    scheduleManagerInitialFocus(table);
}

} // namespace zarya
