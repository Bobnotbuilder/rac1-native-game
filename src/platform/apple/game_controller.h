// The Apple host samples this before the existing host_input::compute / to_packet path.
#pragma once
#include <host/host_input.h>
namespace rac1::platform::apple {
host_input::HostState sample_controller();
}
