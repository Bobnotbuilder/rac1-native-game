// The full-screen menu's save slot widgets and the saves they start: see
// save_menus.cpp.
#ifndef RAC1_GAME_OVL_SAVE_MENUS_H
#define RAC1_GAME_OVL_SAVE_MENUS_H

#include <cstdint>

namespace rac1 {

int32_t save_slots_update(uint32_t w);       // orig: 0x00280bd0
int32_t load_slots_update(uint32_t w);       // orig: 0x00280f20
int32_t new_game_slots_update(uint32_t w);   // orig: 0x00281200
int32_t save_slots_draw(uint32_t w);         // orig: 0x00281628
int32_t save_slot_planet_draw(uint32_t w);   // orig: 0x002809e0
int32_t save_slots_focus_in(uint32_t w);     // orig: 0x00280b60
int32_t save_slots_focus_out(uint32_t w);    // orig: 0x00280ba0
int32_t question_focus_in(uint32_t w);       // orig: 0x0027f690
int32_t question_focus_out(uint32_t w);      // orig: 0x0027f6d0
int32_t question_update(uint32_t w);         // orig: 0x0027f700
int32_t question_draw(uint32_t w);           // orig: 0x0027f798
void save_game_image(uint32_t buf, int32_t n);       // orig: 0x00284808
void new_game_image(uint32_t buf, int32_t n);        // orig: 0x002848b8
void challenge_game_image(int32_t n);                // orig: 0x00284950
int32_t text_centered_med(int32_t x, int32_t y, uint64_t colour, uint32_t s, int32_t n);  // orig: 0x001fba40

}  // namespace rac1

#endif
