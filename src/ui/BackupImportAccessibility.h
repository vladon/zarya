#pragma once

#include <QString>
#include <QVector>

class QTableWidget;
class QWidget;

namespace zarya {

void configureBackupImportAccessibility(
    QTableWidget* previewTable,
    const QVector<QWidget*>& modeSelectors,
    QWidget* machineSpecificCheck,
    QWidget* browseButton,
    QWidget* importButton,
    QWidget* cancelButton,
    const QString& previewTableName,
    bool previewAvailable);

} // namespace zarya
