// libcdvd as the game uses it: the disc's reads (the host's disc, the user's ISO,
// read at once) and the console's real-time clock. On the PS2 sceCdReadClock
// (0x121780) asks the IOP's CDVD for the clock (8 bytes: status, then second, minute, hour, a
// pad byte, day, month, year in BCD; the console keeps Japan's time) and 0x12d6d8 turns it into
// the local time with the console's time zone and summer time settings. The host has neither:
// its own local time is the clock, already local.
#include <game/lib/libcdvd.h>

#include <runtime/kernel/disc.h>
#include <runtime/kernel/ee_memory.h>

#include <platform/platform.h>

namespace rac1 {

std::time_t g_host_clock_fixed = 0;

namespace {
uint8_t bcd(int v) { return static_cast<uint8_t>(((v / 10) % 10) << 4 | (v % 10)); }  // orig: (helper) the clock's BCD bytes
}  // namespace

// The disc's reads are done at once on the host: none is ever in progress (0).
// orig: (host) stands in for 0x00120c30 (sceCdSync)
int32_t cd_sync(int32_t mode) {
    (void)mode;
    return 0;
}

// `sectors` sectors from `lsn` read to EE `buf` (the read mode at `mode`: retries, spindle; the
// host has none): 1 when started, 0 when the disc cannot be read.
// orig: (host) stands in for 0x00121450 (sceCdRead)
int32_t cd_read(uint32_t lsn, uint32_t sectors, uint32_t buf, uint32_t mode) {
    (void)mode;
    return disc_read_ee(buf, lsn, sectors) ? 1 : 0;
}

// The last read's error: none.
// orig: (host) stands in for 0x00121630 (sceCdGetError)
int32_t cd_get_error() { return 0; }

// A read stopped: the host's are already done. 1.
// orig: (host) stands in for 0x001216c8 (sceCdBreak)
int32_t cd_break() { return 1; }

// The clock into `clock` (8 bytes), the call's result 1.
// orig: (host) stands in for 0x00121780 (sceCdReadClock: the CDVD's clock through the IOP)
int32_t cd_read_clock(uint32_t clock) {
    const std::time_t now = g_host_clock_fixed != 0 ? g_host_clock_fixed : std::time(nullptr);
    std::tm t{};
    platform::local_time(now, &t);
    const uint8_t b[8] = {0, bcd(t.tm_sec), bcd(t.tm_min), bcd(t.tm_hour), 0, bcd(t.tm_mday), bcd(t.tm_mon + 1), bcd(t.tm_year % 100)};
    for (uint32_t i = 0; i < 8; ++i) ee_write<uint8_t>(clock + i, b[i]);
    return 1;
}

// The clock made local (the console's time zone and summer time): the host's clock already is.
// orig: (host) stands in for 0x0012d6d8 (the console's time zone applied to the clock)
void clock_to_local(uint32_t) {}

}  // namespace rac1
