// The ship's glows on Aridia: Aridia's own copies of Veldin's unported draw callbacks 0x28e670 / 0x28efc8, transliterated
// (aridia_ship_asm.cpp). Proved against the original.
#ifndef RAC1_GAME_OVL_ARIDIA_SHIP_H
#define RAC1_GAME_OVL_ARIDIA_SHIP_H

#include <cstdint>

namespace rac1 {

// transliterated (aridia_ship_asm.cpp)
void ari_ship_glow_draw_l2(uint32_t m);         // orig: L2:0x0028dde0
void ari_ship_engine_glow_draw_l2(uint32_t m);  // orig: L2:0x0028e738
int64_t ari_point_on_screen_l2(uint32_t pt, float r);  // orig: L2:0x00211348
int32_t ari_screen_alpha_l2(uint32_t pt, uint32_t out, float r);  // orig: L2:0x00211440
void ari_ship_takeoff_draw_l2(uint32_t m);  // orig: L2:0x0028e9a8

}  // namespace rac1

#endif
