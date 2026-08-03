#pragma once

#include <QString>
#include <QVector>

class QPlainTextEdit;
class QTableWidget;
class QWidget;

namespace zarya {

void configureRuleSetManagerAccessibility(
    QTableWidget* requiredTable,
    QTableWidget* allTable,
    QPlainTextEdit* logView,
    const QVector<QWidget*>& actions,
    const QString& requiredTableName,
    const QString& allTableName,
    const QString& logName);

} // namespace zarya
