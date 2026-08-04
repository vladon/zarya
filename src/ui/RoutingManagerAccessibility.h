#pragma once

#include <QString>
#include <QVector>

class QTableWidget;
class QWidget;

namespace zarya {

void configureRoutingManagerAccessibility(
    QTableWidget* table,
    const QVector<QWidget*>& actions,
    const QString& tableName);

} // namespace zarya
