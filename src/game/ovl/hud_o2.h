// The underwater oxygen indicator: the HUD element that hero_water_air (0x00227778) requests in
// controller slot 4 with the key 0x753f -- a field of 100 bubbles filling a frame as the hero's
// air (P+0x22a0, 10000 full) runs out.
#ifndef RAC1_GAME_OVL_HUD_O2_H
#define RAC1_GAME_OVL_HUD_O2_H

#include <cstdint>

namespace rac1 {

void hud_o2_start(uint32_t e);    // orig: 0x00236268
void hud_o2_update(uint32_t e);   // orig: 0x00236458
int32_t hud_o2_draw(uint32_t e);  // orig: 0x002365a8
void hud_o2_fns_register();       // orig: (none)

}  // namespace rac1
#endif
