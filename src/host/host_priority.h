// The host's thread priorities: the desktop's RealtimeKit (rtkit, over the system D-Bus, libdbus loaded at run
// time) asked for a real-time audio thread and a higher priority (a lower nice) for the game's threads, so that a CPU loaded
// by other programs takes less from the game's frame and nothing from the audio device's thread. An unprivileged process
// cannot raise its own priorities (RLIMIT_RTPRIO / RLIMIT_NICE are 0 by default); rtkit is how desktop audio clients and games
// get them. Without rtkit or libdbus nothing happens. Host code.
#ifndef RAC1_TOOLS_RUNTIME_HOST_HOST_PRIORITY_H
#define RAC1_TOOLS_RUNTIME_HOST_HOST_PRIORITY_H

#include <sys/types.h>

namespace rac1 {

// orig: (host) the priorities wanted? (RAC1_PRIORITY=0: none; the default: on in a window, off in a headless run)
bool host_priority_enabled();
void host_priority_set_enabled(bool on);
// orig: (host) the thread `tid` of this process made real-time (SCHED_RR at `prio`, at most rtkit's MaxRealtimePriority);
// false when refused or unavailable. Blocks on the D-Bus call (up to 1 s): not from a real-time callback.
bool host_priority_realtime(pid_t tid, int prio);
// orig: (host) every thread of this process given the nice level `nice` (at least rtkit's MinNiceLevel), but those already
// real-time; returns how many were changed. Blocks on the D-Bus calls.
int host_priority_raise_all(int nice);

}  // namespace rac1

#endif
