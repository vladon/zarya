#include "ui/desktopapp/ZaryaBaseIntegration.h"

#include "logging/LogBuffer.h"

#include <QCoreApplication>
#include <QMetaObject>

namespace zarya {

ZaryaBaseIntegration::ZaryaBaseIntegration(int argc, char** argv)
    : base::Integration(argc, argv)
{
}

void ZaryaBaseIntegration::enterFromEventLoop(FnMut<void()>&& method)
{
    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        [callable = std::move(method)]() mutable { callable(); },
        Qt::QueuedConnection);
}

bool ZaryaBaseIntegration::logSkipDebug()
{
    return false;
}

void ZaryaBaseIntegration::logMessageDebug(const QString& message)
{
    LogBuffer::instance().append(QStringLiteral("[desktop-app] %1").arg(message));
}

void ZaryaBaseIntegration::logMessage(const QString& message)
{
    LogBuffer::instance().append(QStringLiteral("[desktop-app] %1").arg(message));
}

} // namespace zarya
