// The title's main menu: the boot module's own variants of the full-screen menu's
// page start, update, closing and drawing (the overlays' 0x276368 / 0x276bd0 / 0x276908 /
// 0x2772c0, cut down to the main menu's pages: New Game, Load Game, Options), ported at the
// boot module's addresses (`// orig: front:0x...`). Its block is 0x1d5bf0 (the overlays'
// 0x1b9cf0, see src/game/ovl/pause_menu.h), its 14 mobys 0x1d5d90, the slots in use 0x1ce2c0.
#ifndef RAC1_GAME_BOOT_TITLE_MENU_H
#define RAC1_GAME_BOOT_TITLE_MENU_H

#include <cstdint>

namespace rac1 {

void title_menu_page_start();   // orig: front:0x00218f98
void title_menu_close();        // orig: front:0x002191b8
void title_menu_update();       // orig: front:0x002192a8
void title_menu_draw();         // orig: front:0x002196b8

// The main menu's widget functions of its own (the overlays' are in
// src/game/ovl/pause_menu_widgets.cpp)
int32_t title_options_list_focus_in(uint32_t w);  // orig: front:0x0021a1b0
int32_t title_label_update(uint32_t w);           // orig: front:0x00219d80
int32_t title_nop_focus(uint32_t w);              // orig: front:0x0021a308

}  // namespace rac1

#endif
