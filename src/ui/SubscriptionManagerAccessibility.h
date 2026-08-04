#pragma once

#include <QString>
#include <QVector>

class QTableView;
class QWidget;

namespace zarya {

void configureSubscriptionManagerAccessibility(
    QTableView* table,
    const QVector<QWidget*>& actions,
    const QString& tableName);

} // namespace zarya
