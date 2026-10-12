// The host's thread priorities on Windows (see host_priority.h): SetThreadPriority, which a normal
// process may use for its own threads (no rtkit to ask). Host code. Not yet run on a Windows machine.
#include "host_priority.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>

#include <cstdio>
#include <cstdlib>

namespace rac1 {

namespace {
int g_enabled = -1;  // -1: not decided (the default: on unless headless, decided by the host)
}

bool host_priority_enabled() {
    if (const char* p = std::getenv("RAC1_PRIORITY")) return std::atoi(p) != 0;
    return g_enabled > 0;
}

void host_priority_set_enabled(bool on) { g_enabled = on ? 1 : 0; }

bool host_priority_realtime(std::uint64_t tid, int prio) {
    if (!host_priority_enabled()) return false;
    const HANDLE h = OpenThread(THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION, FALSE, static_cast<DWORD>(tid));
    if (h == nullptr) return false;
    // the audio device's thread (10) time-critical, the IOP's (9) one step under it
    const bool ok = SetThreadPriority(h, prio >= 10 ? THREAD_PRIORITY_TIME_CRITICAL : THREAD_PRIORITY_HIGHEST) != 0;
    CloseHandle(h);
    return ok;
}

int host_priority_raise_all(int /*nice*/) {
    if (!host_priority_enabled()) return 0;
    const HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    const DWORD pid = GetCurrentProcessId();
    int changed = 0;
    THREADENTRY32 te{};
    te.dwSize = sizeof te;
    for (BOOL more = Thread32First(snap, &te); more; more = Thread32Next(snap, &te)) {
        if (te.th32OwnerProcessID != pid) continue;
        const HANDLE h = OpenThread(THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
        if (h == nullptr) continue;
        // (the audio threads, already higher, are left as they are)
        if (GetThreadPriority(h) < THREAD_PRIORITY_ABOVE_NORMAL && SetThreadPriority(h, THREAD_PRIORITY_ABOVE_NORMAL)) ++changed;
        CloseHandle(h);
    }
    CloseHandle(snap);
    return changed;
}

}  // namespace rac1
