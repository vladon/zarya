#pragma once

#include <QString>
#include <QVector>

class QPlainTextEdit;
class QTableView;
class QWidget;

namespace zarya {

class ZaryaSelector;

void configureMainWindowDataAccessibility(
    QTableView* profileTable,
    const QString& profileTableName,
    ZaryaSelector* logFilter,
    const QString& logFilterName,
    const QVector<QWidget*>& logActions,
    QPlainTextEdit* logView,
    const QString& logViewName);

} // namespace zarya
