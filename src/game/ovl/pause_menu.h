// The full-screen menu (game mode 3: the pause menu and its pages) State block M = 0x1b9cf0: +0 the menu state (the page's id, 1 a
// page change in progress, 0x14 closing), +4 the page shown, +8 the page to go to, +0xc the exit
// kind, +0x14 a countdown, +0x110 the frames since it opened. A page: +0..0x34 the animation of
// each of the 14 menu mobys, +0x38 its parent, +0x3c its state, +0x40 the focused widget,
// +0x44..0x78 its 14 widgets, +0x80 the widget to focus next. A widget: +0 update (a nonzero
// result closes the menu), +4 draw, +8 focus in, +0xc focus out (a0 the widget, a1 1 for a focus
// change), +0x10 draw flags, +0x14 its moby, +0x18..0x24 its screen box, +0x30 flags, +0x34 its
// entries (0xc bytes: text, action, target, sub-text, highlight timer), +0x38 / +0x3c the widgets
// above / below, +0x40 the selected entry. The 14 menu mobys (class 0x472) at 0x1b9e90.
#ifndef RAC1_GAME_OVL_PAUSE_MENU_H
#define RAC1_GAME_OVL_PAUSE_MENU_H

#include <cstdint>

namespace rac1 {

// --- the level goals the menu shows (pause_menu_goals.cpp) ---------------------------------
int32_t goal_cond(int32_t kind, uint32_t arg);  // orig: 0x0024dda8
int32_t goals_update();                         // orig: 0x0024dc08
int32_t skill_points_count();                   // orig: 0x00264640
int32_t gold_bolts_count();                     // orig: 0x002645f8
void cheats_menu_entries();                     // orig: 0x00284ca0
int32_t goal_callback(uint32_t fn, uint32_t a0);  // orig: (helper) `jalr` to a goal record's test

// --- the menu's update (pause_menu.cpp) -------------------------------------------------------
void menu_open(int32_t state);        // orig: 0x00276190
void menu_update();                   // orig: 0x00276bd0
void menu_page_start();               // orig: 0x00276368
void menu_close();                    // orig: 0x00276908
void menu_camera_set();               // orig: 0x00276128
void menu_vram_layout(int32_t full);  // orig: 0x00283908
uint32_t menu_moby_spawn(uint32_t cls);  // orig: 0x002830d8
int32_t menu_moby_free(uint32_t m);   // orig: 0x00283178
void menu_hud_tex_load(int32_t which);  // orig: 0x00234fd0
void moby_lists_swap(int32_t menu);   // orig: 0x00235668
void hud_tex_unrelocate(int32_t set); // orig: 0x00235310
void cheat_code_input();              // orig: 0x002831c0
int32_t menu_button_update(uint32_t w);   // orig: 0x00278840
int32_t menu_save_entry_focus(uint32_t w);  // orig: 0x0027ba18
// `jalr` to a widget function (the ported ones; others are reported once and return 0)
int32_t menu_widget_call(uint32_t fn, uint32_t w, int32_t a1);  // orig: (helper)

void moby_joint_positions_local(uint32_t m, int32_t n, uint32_t list, uint32_t out);  // orig: 0x00253e48
void moby_joint_positions(uint32_t m, int32_t n, uint32_t list, uint32_t out);  // orig: 0x0024f850
void menu_moby_corners(uint32_t m);   // orig: 0x002df690
void menu_moby_update(uint32_t m);    // orig: 0x002df638
void pause_menu_fns_register();       // orig: (none)

// --- its drawing (pause_menu_draw.cpp) --------------------------------------------------------
void menu_draw_mode3();               // orig: 0x001f8f50
void menu_draw(int32_t skip);         // orig: 0x002772c0
int32_t menu_button_draw(uint32_t w); // orig: 0x00278e10
uint32_t menu_highlight_colour(int32_t t, int32_t c0, int32_t c1);  // orig: 0x00279320
void text_colour_codes_off();         // orig: 0x001fb038
void menu_box_project(uint32_t p0, uint32_t p1, uint32_t w, uint32_t h, uint32_t x, uint32_t y);  // orig: 0x00299f40
void gs_flat_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint64_t rgba, int32_t pixels);  // orig: 0x0023caa8
void gs_flat_rect_z(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint64_t rgba, uint64_t z, int32_t pixels);  // orig: 0x0023cc30
void render_to_texture_begin(int32_t lw, int32_t lh, int32_t second, float aspect);  // orig: 0x001fc928
void render_target_view(int32_t w, int32_t h, float aspect, float a, float b, float c, float d);  // orig: 0x001f7fa0
void render_target_set(int32_t lw, int32_t lh, int32_t fbp);  // orig: 0x002010b0
void render_to_texture_end();         // orig: 0x001fca18
void menu_panel_draw(uint32_t m);     // orig: 0x00281a70
void hud_sprite_xyz(int32_t frame, int32_t x, int32_t y, int32_t w, int32_t h, int32_t a);  // orig: 0x0023bd20
void gs_upload_rows(uint32_t src, int32_t dbp, int32_t bytes);  // orig: 0x002a0fc8
// The reverse: `bytes` (rounded up to 16 KB) of the GS at block `dbp` (+ 1/256 per byte) stored at EE `dst`,
// 64 x 64 32-bit pixels a transfer (the frame captured behind the pause menu and the Vendor's screens).
void gs_store_rows(uint32_t dst, int32_t dbp, int32_t bytes);  // orig: 0x002a0f18
void spr_moby_fill();                 // orig: 0x0024fdc8
void spr_moby_load();                 // orig: 0x0024fe20
void spr_moby_save();                 // orig: 0x0024fdf0

}  // namespace rac1

#endif
