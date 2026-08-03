#pragma once

#include <QString>
#include <QVector>

class QPlainTextEdit;
class QTableWidget;
class QWidget;

namespace zarya {

void configureCoreManagerAccessibility(
    QTableWidget* table,
    QPlainTextEdit* logView,
    const QVector<QWidget*>& actions,
    const QString& tableName,
    const QString& logName);

} // namespace zarya
