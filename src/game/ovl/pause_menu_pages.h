// The full-screen menu's pages below the pause page (see pause_menu.h): their widget functions (+0 update, +4 draw, +8 focus in,
// +0xc focus out; a0 the widget, a1 1 for a change of focus) and the helpers they share.
#ifndef RAC1_GAME_OVL_PAUSE_MENU_PAGES_H
#define RAC1_GAME_OVL_PAUSE_MENU_PAGES_H

#include <cstdint>

namespace rac1 {

// `jalr` to a widget function of these pages: true (and its result in r) when it is ported
bool menu_page_widget_call(uint32_t fn, uint32_t w, int32_t a1, int32_t& r);  // orig: (helper)

// --- shared widgets (pause_menu_widgets.cpp) -------------------------------------------------
int32_t menu_label_update(uint32_t w);        // orig: 0x002779c8
int32_t menu_label_focus(uint32_t w);         // orig: 0x00277f60
int32_t menu_label_draw(uint32_t w);          // orig: 0x00277f70
int32_t menu_nop_focus(uint32_t w);           // orig: 0x00277f50
int32_t menu_nop_focus2(uint32_t w);          // orig: 0x00277f58
int32_t menu_nop_update(uint32_t w);          // orig: 0x002799e0
int32_t menu_buffers_full(uint32_t w);        // orig: 0x0027a728
int32_t menu_picture_update(uint32_t w);      // orig: 0x0027da10
int32_t menu_picture_draw(uint32_t w);        // orig: 0x0027df90
int32_t menu_picture_focus_in(uint32_t w);    // orig: 0x0027d928
int32_t menu_picture_focus_out(uint32_t w);   // orig: 0x0027d9c0
uint32_t menu_buf_alloc(int32_t big);         // orig: 0x00283a60
int32_t menu_buf_free(uint32_t buf);          // orig: 0x00283b20
int32_t menu_buf_size(uint32_t buf);          // orig: 0x00283bd0
int32_t menu_buf_busy(uint32_t buf);          // orig: 0x00283c20
int32_t menu_buf_ready(uint32_t buf);         // orig: 0x00283c68
void disc_read_break();                       // orig: 0x00265f00
uint64_t image_upload(uint32_t img);          // orig: 0x00245a20

// --- the Goodies page's (state 9) sub-pages (pause_menu_special.cpp) ---------------------------------
int32_t cheats_list_focus_in(uint32_t w);          // orig: 0x00277e28
int32_t level_movies_focus_in(uint32_t w);  // orig: 0x0027f970
int32_t sketchbook_update(uint32_t w);         // orig: 0x0027f9b0
int32_t epilogue_update(uint32_t w);         // orig: 0x0027fa98
int32_t epilogue_arrows_draw(uint32_t w);           // orig: 0x0027fba0
int32_t skill_points_text_draw(uint32_t w);             // orig: 0x0027fd38

// --- Options' sub-pages (pause_menu_options.cpp) ---------------------------------------------
void text_small(int32_t x, int32_t y, uint64_t colour, uint32_t s, int32_t n);  // orig: 0x001fb370
void text_box_small(uint32_t b, uint64_t colour, uint32_t s, int32_t n);       // orig: 0x001fc438
void hud_sprite_uv(int32_t frame, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t u0, int32_t v0, int32_t u1, int32_t v1,
                   int32_t a);                // orig: 0x0023c5f8
int32_t menu_sound_update(uint32_t w);        // orig: 0x0027a778
int32_t menu_sound_leave(uint32_t w);         // orig: 0x0027a748
int32_t menu_sound_draw(uint32_t w);          // orig: 0x0027aa48
int32_t menu_settings_update(uint32_t w);     // orig: 0x0027ef00
int32_t menu_settings_draw(uint32_t w);       // orig: 0x0027f0a8
int32_t menu_switches_update(uint32_t w);     // orig: 0x0027ea70
int32_t menu_switches_draw(uint32_t w);       // orig: 0x0027ec78
int32_t menu_quit_update(uint32_t w);         // orig: 0x0027c410
int32_t menu_quit_draw(uint32_t w);           // orig: 0x0027c4d8

// --- the item grids (pause_menu_items.cpp) -----------------------------------------------------
int32_t menu_grid_update(uint32_t w);                 // orig: 0x002794a0
int32_t menu_grid_focus_in(uint32_t w, int32_t change);  // orig: 0x002793f0
int32_t menu_grid_draw(uint32_t w);                   // orig: 0x0027b590

// --- the 3D item panels (pause_menu_models.cpp) ------------------------------------------------
void widget_text_box(uint32_t b, uint32_t w);         // orig: 0x002845c0
int32_t menu_model_focus_in(uint32_t w);              // orig: 0x0027bb78
int32_t menu_model_focus_out(uint32_t w);             // orig: 0x0027bba0
int32_t menu_model_update(uint32_t w);                // orig: 0x0027be78
int32_t menu_model_draw(uint32_t w);                  // orig: 0x0027c250
void menu_item_moby_update(uint32_t m);               // orig: 0x0027c2e0
int32_t menu_ammo_update(uint32_t w);                 // orig: 0x0027bbe0
void menu_ammo_moby_update(uint32_t m);               // orig: 0x0027be40
int32_t menu_ammo_draw(uint32_t w);                   // orig: 0x0027bd58
int32_t menu_ammo_count_draw(uint32_t w);             // orig: 0x0027cda0
int32_t menu_hero_draw(uint32_t w);                   // orig: 0x0027ba40

// --- Quick Select, the Items statistics (pause_menu_quick.cpp) --------------------------------
void text_right_big(int32_t x, int32_t y, uint64_t colour, uint32_t s, int32_t n);  // orig: 0x001fb918
int32_t bytes_14bec0_count();                         // orig: 0x00264588
int32_t bytes_14bec0_left();                          // orig: 0x00264540
int32_t menu_quick_update(uint32_t w);                // orig: 0x0027a3e8
int32_t menu_quick_focus_in(uint32_t w);              // orig: 0x0027a610
int32_t menu_quick_focus_out(uint32_t w);             // orig: 0x0027a6a8
int32_t menu_quick_draw(uint32_t w);                  // orig: 0x0027e768
int32_t menu_stats_focus_in(uint32_t w);              // orig: 0x0027c690
int32_t menu_stats_focus_out(uint32_t w);             // orig: 0x0027c738
void menu_stats_moby_update(uint32_t m);              // orig: 0x0027cd68
int32_t menu_stats_draw(uint32_t w);                  // orig: 0x0027c768

// --- the Help page's sub-pages (pause_menu_help.cpp) -------------------------------------------
int32_t help_log_list_focus_in(uint32_t w);           // orig: 0x0027adb0
int32_t help_log_list_focus_out(uint32_t w);          // orig: 0x0027ae38
int32_t help_log_text_focus_in(uint32_t w);           // orig: 0x0027ae40
int32_t help_log_text_focus_out(uint32_t w);          // orig: 0x0027af10
int32_t help_log_text_update(uint32_t w);             // orig: 0x0027af80
int32_t text_list_draw(uint32_t w);                   // orig: 0x0027b0f0
int32_t two_pictures_update(uint32_t w);              // orig: 0x0027e290
int32_t two_pictures_draw(uint32_t w);                // orig: 0x0027e3d8
int32_t items_help_focus_in(uint32_t w);              // orig: 0x0027a6e0
int32_t icon_list_update(uint32_t w);                 // orig: 0x00277a58
int32_t icon_list_draw(uint32_t w);                   // orig: 0x00277be8
int32_t weapons_help_focus_in(uint32_t w);            // orig: 0x0027f240
int32_t gadgets_help_focus_in(uint32_t w);            // orig: 0x0027f308

// --- the Weapons / Gadgets hero panel (pause_menu_hero.cpp) -------------------------------------
int32_t menu_hero_update(uint32_t w);                 // orig: 0x00281fb0
int32_t menu_hero_focus_in(uint32_t w);               // orig: 0x00281d10
int32_t menu_hero_focus_out(uint32_t w);              // orig: 0x00281f00
void menu_moby_nop_a(uint32_t m);                     // orig: 0x002827a8
void menu_moby_nop_b(uint32_t m);                     // orig: 0x002827b0
void menu_weapon_moby_update(uint32_t m);             // orig: 0x002827b8
void menu_item2_moby_update(uint32_t m);              // orig: 0x00282970
void menu_item1_moby_update(uint32_t m);              // orig: 0x00282a60
void menu_item3_moby_update(uint32_t m);              // orig: 0x00282c08
void menu_gadget_moby_update(uint32_t m);             // orig: 0x00282d38
void menu_orb_moby_update(uint32_t m);                // orig: 0x00282dc8
int32_t hero_anim_update(uint32_t hero, uint32_t weapon, uint32_t item2, uint32_t o1, uint32_t o2, uint32_t o3);  // orig: 0x00283cb8
int32_t hero_anim_queue(uint32_t seq, uint32_t kind, uint32_t arg, uint32_t weapon, uint32_t cls, uint32_t e1, uint32_t s1,
                        uint32_t e2, uint32_t s2, uint32_t e3, uint32_t s3, uint32_t first, uint32_t count);  // orig: 0x00284420
int32_t hero_anim_dequeue();                          // orig: 0x002844b8
int32_t hero_anim_clear();                            // orig: 0x00284560
int32_t hero_anim_extra_free();                       // orig: 0x00284600
void hero_anim_extra_load(int32_t first, int32_t count);  // orig: 0x00284690
void hero_model_refresh(uint32_t m);                  // orig: 0x00234a50
void hero_model_setup(uint32_t m);                    // orig: 0x00234b38
void menu_sequences_stash();                          // orig: 0x00276078

}  // namespace rac1

#endif
