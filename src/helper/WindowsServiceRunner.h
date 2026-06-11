#pragma once

namespace zarya {

class WindowsServiceRunner {
public:
    static bool shouldRunAsService(int argc, char** argv);
    static int runAsService(int argc, char** argv);
};

} // namespace zarya
