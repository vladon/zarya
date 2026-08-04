#include "ui/BackupImportAccessibility.h"

#include <QTableWidget>
#include <QWidget>

namespace zarya {

void configureBackupImportAccessibility(
    QTableWidget* previewTable,
    const QVector<QWidget*>& modeSelectors,
    QWidget* machineSpecificCheck,
    QWidget* browseButton,
    QWidget* importButton,
    QWidget* cancelButton,
    const QString& previewTableName,
    bool previewAvailable)
{
    previewTable->setAccessibleName(previewTableName);

    QWidget* previous = previewTable;
    for (QWidget* selector : modeSelectors) {
        QWidget::setTabOrder(previous, selector);
        previous = selector;
    }
    QWidget::setTabOrder(previous, machineSpecificCheck);
    QWidget::setTabOrder(machineSpecificCheck, browseButton);
    QWidget::setTabOrder(browseButton, importButton);
    QWidget::setTabOrder(importButton, cancelButton);

    QWidget* initialFocus = previewAvailable
        ? static_cast<QWidget*>(previewTable)
        : browseButton;
    initialFocus->setFocus(Qt::OtherFocusReason);
}

} // namespace zarya
