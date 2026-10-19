// The Apple host samples this before the existing host_input::compute / to_packet path.
#pragma once
#include <host/host_input.h>
#ifdef __OBJC__
@class GCExtendedGamepad;
#endif
namespace rac1::platform::apple {
host_input::HostState sample_controller();
#ifdef __OBJC__
// Shared by real device sampling and framework-backed snapshot contracts.
host_input::HostState decode_controller_snapshot(GCExtendedGamepad* pad);
#endif
}
