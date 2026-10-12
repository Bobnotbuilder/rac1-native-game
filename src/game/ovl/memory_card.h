// The memory card's machines, a new game's state and game mode 4's screens: see
// memory_card.cpp, screens.cpp
#ifndef RAC1_GAME_OVL_MEMORY_CARD_H
#define RAC1_GAME_OVL_MEMORY_CARD_H

#include <cstdint>

namespace rac1 {

// --- memory_card.cpp ---
void card_ui_machine();                                                    // orig: 0x0024ac20
void card_machine();                                                       // orig: 0x0024b690
int32_t level_buffer_room(uint32_t size, uint32_t a, uint32_t b);           // orig: 0x00203358
uint32_t save_list_size(uint32_t list);                                    // orig: 0x0024cf40
bool save_block_check(uint32_t p);                                         // orig: 0x0024cff0
int32_t save_block_read(uint32_t src, int32_t index, uint32_t list);       // orig: 0x0024d1d8
void save_blocks_read(uint32_t p);                                         // orig: 0x0024b550
void new_game_state();                                                     // orig: 0x0024b628

// --- screens.cpp (game mode 4) ---
int32_t screen_prev_mode();                                                // orig: 0x00201720
void screen_open(int32_t kind, uint32_t arg);                              // orig: 0x00201730
void screen_update();                                                      // orig: 0x00202aa0
void screen_frame(int32_t a, int32_t b, int32_t c, int32_t d, uint64_t colour);  // orig: 0x001faea0
void screen_draw();                                                        // orig: 0x002018c8
void screen_draw_mode4();                                                  // orig: 0x001f8ec8

}  // namespace rac1

#endif
