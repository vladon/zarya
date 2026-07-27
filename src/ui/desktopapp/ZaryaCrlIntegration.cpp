#include "ui/desktopapp/ZaryaCrlIntegration.h"

#include <rpl/never.h>

namespace crl {

rpl::producer<> on_main_update_requests()
{
    // Hosted by the app (tdesktop wires this to Sandbox update requests).
    // Spike: never fires; animation Manager still schedules its own timers.
    return rpl::never<>();
}

} // namespace crl
