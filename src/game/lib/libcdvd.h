// libcdvd, the console's disc and clock (the IOP's CDVD through SIF RPC): host stand-ins, the
// host's disc being the user's ISO (runtime/kernel/disc.h) and its reads done at once.
#ifndef RAC1_GAME_LIB_LIBCDVD_H
#define RAC1_GAME_LIB_LIBCDVD_H

#include <cstdint>
#include <ctime>

namespace rac1 {

int32_t cd_sync(int32_t mode);                                             // orig: (host) 0x00120c30 sceCdSync
int32_t cd_read(uint32_t lsn, uint32_t sectors, uint32_t buf, uint32_t mode);  // orig: (host) 0x00121450 sceCdRead
int32_t cd_get_error();                                                    // orig: (host) 0x00121630 sceCdGetError
int32_t cd_break();                                                        // orig: (host) 0x001216c8 sceCdBreak
int32_t cd_read_clock(uint32_t clock);                                     // orig: (host) 0x00121780 sceCdReadClock
void clock_to_local(uint32_t clock);                                       // orig: (host) 0x0012d6d8
// testing: a fixed time for the clock (0: the host's own clock)
extern std::time_t g_host_clock_fixed;

}  // namespace rac1

#endif
