#include "testing/TcpPingTester.h"

#include <QElapsedTimer>
#include <QTcpSocket>

namespace zarya {

TestResult TcpPingTester::run(const Profile& profile, int timeoutMs)
{
    TestResult result;
    if (profile.address.trimmed().isEmpty() || profile.port < 1 || profile.port > 65535) {
        result.status = TestStatus::Failed;
        result.errorMessage = QStringLiteral("Invalid profile address or port.");
        return result;
    }

    QTcpSocket socket;
    QElapsedTimer timer;
    timer.start();
    socket.connectToHost(profile.address.trimmed(), static_cast<quint16>(profile.port));

    if (!socket.waitForConnected(timeoutMs)) {
        if (socket.error() == QAbstractSocket::SocketTimeoutError) {
            result.status = TestStatus::Timeout;
            result.errorMessage = QStringLiteral("TCP connection timed out.");
        } else {
            result.status = TestStatus::Failed;
            result.errorMessage =
                socket.errorString().isEmpty()
                    ? QStringLiteral("TCP connection failed.")
                    : socket.errorString();
        }
        return result;
    }

    result.tcpPingMs = static_cast<int>(timer.elapsed());
    result.status = TestStatus::Available;
    socket.disconnectFromHost();
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.waitForDisconnected(1000);
    }
    return result;
}

} // namespace zarya
