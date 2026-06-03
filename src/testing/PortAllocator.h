#pragma once

namespace zarya {

struct InboundPorts {
    int socksPort = -1;
    int httpPort = -1;
};

class PortAllocator {
public:
    static int allocateFreeLocalPort();
    static InboundPorts allocateInboundPorts();
};

} // namespace zarya
