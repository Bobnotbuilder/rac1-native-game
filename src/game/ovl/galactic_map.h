// The galactic map (the ship's planet menu): the widget functions of its two pages, the galaxy page 0x1b6088 (menu
// state 0xf, opened by the take-off's end 0x290590: 0x276190(0), state 0xe) and the planet page 0x1b63f8 (state 0x10).

#ifndef RAC1_GAME_OVL_GALACTIC_MAP_H
#define RAC1_GAME_OVL_GALACTIC_MAP_H

#include <cstdint>

namespace rac1 {

int32_t galmap_nop_focus(uint32_t w);         // orig: 0x0027f440
int32_t galmap_list_focus_in(uint32_t w);     // orig: 0x0027f448
int32_t galmap_view_update(uint32_t w);       // orig: 0x0027f550
int32_t galmap_view_draw(uint32_t w);         // orig: 0x0027f578
int32_t galmap_planet_update(uint32_t w);     // orig: 0x0027f5b0
int32_t galmap_planet_draw(uint32_t w);       // orig: 0x0027cbc0
// The galactic map's widget functions by their (Veldin) address: false when `fn` is none of them.
bool galactic_map_widget_call(uint32_t fn, uint32_t w, int32_t a1, int32_t& r);  // orig: (helper)

}  // namespace rac1

#endif
