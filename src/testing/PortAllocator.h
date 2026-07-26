#pragma once

namespace zarya {

struct InboundPorts {
    int mixedPort = -1;
};

class PortAllocator {
public:
    static int allocateFreeLocalPort();
    static InboundPorts allocateInboundPorts();
};

} // namespace zarya
