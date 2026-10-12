// The ship's draw callbacks: queued with 0x1f93e0 by the ship's arrival / take-off / flight code
// (arrival.cpp) and by Veldin's parked ship (veldin_mobys.cpp), run through ovl_call_moby_fn on every level. Transliterated
// at Veldin's addresses but point_on_screen_depth, by hand
// (ship_draw_cbs.cpp).
#ifndef RAC1_GAME_OVL_SHIP_DRAW_CBS_H
#define RAC1_GAME_OVL_SHIP_DRAW_CBS_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>

namespace rac1 {

void ship_shadow_draw(uint32_t a0);  // orig: 0x0028e670
void ship_flame_draw(uint32_t a0);   // orig: 0x0028efc8
void ship_trails_draw(uint32_t a0);  // orig: 0x0028f238
// point_on_screen 0x2002e0 on `pt` (radius r), and to *out the depth byte of its VU0 result (vf3.y, the radius past the near
// plane, as an integer >> 7, within 0..0x80; 0 when the point is off the screen)
int64_t point_on_screen_depth(uint32_t pt, uint32_t out, float r);  // orig: 0x002003d8
// orig: (none) — registers the callbacks for the indirect calls (ovl_call_moby_fn)
void ship_draw_cbs_register();

}  // namespace rac1
#endif
