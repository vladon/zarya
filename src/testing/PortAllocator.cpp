#include "testing/PortAllocator.h"

#include <QHostAddress>
#include <QTcpServer>

namespace zarya {

int PortAllocator::allocateFreeLocalPort()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        return -1;
    }
    const int port = server.serverPort();
    server.close();
    return port;
}

InboundPorts PortAllocator::allocateInboundPorts()
{
    InboundPorts ports;
    ports.socksPort = allocateFreeLocalPort();
    ports.httpPort = allocateFreeLocalPort();
    return ports;
}

} // namespace zarya
