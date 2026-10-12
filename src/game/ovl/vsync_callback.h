#ifndef RAC1_GAME_OVL_VSYNC_CALLBACK_H
#define RAC1_GAME_OVL_VSYNC_CALLBACK_H
// The game's vsync callback (base ELF 0x12f1c8, set by the boot with sceGsSyncVCallback):,

#include <cstdint>

namespace rac1 {

// At each vsync: the vsync count 0x15ed48 (64 bit) + 1, and 0x15ed50 ($gp - 0x7eb0, 64 bit) = the time
// 0x15ed40 (the main loop's sum of its frames' T1 counts) + T1_COUNT now. Returns 0.
int32_t vsync_callback();  // orig: 0x0012f1c8

}  // namespace rac1

#endif
