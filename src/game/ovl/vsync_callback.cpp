// The game's vsync callback 0x12f1c8 (base ELF). The host calls it at every vsync the game
// waits for (src/host/orig_main.cpp vsync_passed).
#include <game/ovl/vsync_callback.h>

#include <game/ovl/ovl_common.h>
#include <runtime/ee/ee_timer.h>

namespace rac1 {

// ld a0, 0x15ed48; daddiu a0, a0, 1; sd a0, 0x15ed48; lw v1, T1_COUNT (dsll32 / dsrl32: zero-extended);
// ld a2, 0x15ed40; daddu a2, a2, v1; sd a2, -0x7eb0($gp); v0 = 0.
// orig: 0x0012f1c8
int32_t vsync_callback() {
    sd(0x0015ed48, ld(0x0015ed48) + 1);
    const uint64_t t1 = ee_t1_count();
    sd(0x0015ed50, ld(0x0015ed40) + t1);
    return 0;
}

}  // namespace rac1
