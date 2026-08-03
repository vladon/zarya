#include "ui/MainWindowAccessibility.h"

#include "ui/desktopapp/ZaryaSelector.h"

#include <QPlainTextEdit>
#include <QTableView>
#include <QWidget>

namespace zarya {

void configureMainWindowDataAccessibility(
    QTableView* profileTable,
    const QString& profileTableName,
    ZaryaSelector* logFilter,
    const QString& logFilterName,
    const QVector<QWidget*>& logActions,
    QPlainTextEdit* logView,
    const QString& logViewName)
{
    Q_ASSERT(profileTable);
    Q_ASSERT(logFilter);
    Q_ASSERT(logFilter->focusProxy());
    Q_ASSERT(logView);

    profileTable->setAccessibleName(profileTableName);
    logFilter->setAccessibleLabel(logFilterName);
    logView->setAccessibleName(logViewName);
    logView->setTabChangesFocus(true);

    QVector<QWidget*> focusOrder = {profileTable, logFilter->focusProxy()};
    for (QWidget* action : logActions) {
        if (action) {
            focusOrder.push_back(action);
        }
    }
    focusOrder.push_back(logView);
    for (qsizetype index = 1; index < focusOrder.size(); ++index) {
        QWidget::setTabOrder(focusOrder[index - 1], focusOrder[index]);
    }
}

} // namespace zarya
