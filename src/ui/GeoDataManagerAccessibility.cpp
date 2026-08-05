#include "ui/GeoDataManagerAccessibility.h"
#include "ui/ManagerAccessibility.h"

#include <QPlainTextEdit>
#include <QTableWidget>
#include <QWidget>

namespace zarya {

void configureGeoDataManagerAccessibility(
    QWidget* sourceSelector,
    QTableWidget* table,
    const QVector<QWidget*>& options,
    QPlainTextEdit* logView,
    const QVector<QWidget*>& actions,
    const QString& tableName,
    const QString& logName)
{
    table->setAccessibleName(tableName);
    logView->setAccessibleName(logName);

    QWidget* previous = sourceSelector;
    QWidget::setTabOrder(previous, table);
    previous = table;
    for (QWidget* option : options) {
        QWidget::setTabOrder(previous, option);
        previous = option;
    }
    QWidget::setTabOrder(previous, logView);
    previous = logView;
    for (QWidget* action : actions) {
        QWidget::setTabOrder(previous, action);
        previous = action;
    }
    scheduleManagerInitialFocus(sourceSelector);
}

} // namespace zarya
