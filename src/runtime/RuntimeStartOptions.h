#pragma once

namespace zarya {

struct RuntimeStartOptions {
    bool fromAutostart = false;
    bool allowMissingPrivileges = false;
};

} // namespace zarya
