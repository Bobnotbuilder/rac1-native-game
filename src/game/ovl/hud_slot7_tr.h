// HUD slot 7's start (Veldin's 0x235dc0), update (0x23a218) and draw (0x23a2d0, the Metal Detector's display), transliterated (met on
// Batalia, level 8; the draw on the levels whose build is Veldin's): the declarations of hud_slot7_tr.cpp (regen group veldin_hud7).
#ifndef RAC1_GAME_OVL_HUD_SLOT7_TR_H
#define RAC1_GAME_OVL_HUD_SLOT7_TR_H

#include <cstdint>

namespace rac1 {

void hud_slot7_start_tr(uint32_t a0);  // 0x00235dc0
void hud_slot7_update_tr(uint32_t a0);  // 0x0023a218
void hud_slot7_draw_tr(uint32_t a0);  // 0x0023a2d0

}  // namespace rac1

#endif
