// The desktop RealtimeKit policy is not an iOS scheduling API. CoreAudio owns
// its audio callback scheduling. Do not reinterpret a numeric thread ID as a
// pthread handle, claim real-time elevation, or attempt Linux D-Bus calls.
#include "src/host/host_priority.h"

namespace rac1 {
bool host_priority_enabled() { return false; }
void host_priority_set_enabled(bool) {} // Optional desktop policy is unavailable.
bool host_priority_realtime(std::uint64_t, int) { return false; }
int host_priority_raise_all(int) { return 0; }
} // namespace rac1
