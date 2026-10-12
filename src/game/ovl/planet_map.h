// The planet map (menu page 0x1b3518, state 0xb): its state block 0x1841f0, its set up at the
// overlay's start, the planets' data streamed from the disc / the stash, its widgets. See
// planet_map.cpp
#ifndef RAC1_GAME_OVL_PLANET_MAP_H
#define RAC1_GAME_OVL_PLANET_MAP_H

#include <cstdint>

namespace rac1 {

void map_nop();                                                     // orig: 0x0024abf0
void map_bits_unpack(uint32_t dst, uint32_t runs, uint32_t spans);  // orig: 0x00249fc0
void map_bits_widen(uint32_t dst, uint32_t src);                    // orig: 0x0024a1f0
void map_mask_unpack(uint32_t dst, uint32_t src, uint32_t spans);   // orig: 0x00249f48
void map_mask_from_tiles(uint32_t dst, uint32_t base, uint32_t table);  // orig: 0x00248278
uint64_t image_upload_at(int32_t lw_, int32_t lh_, uint32_t pix, uint32_t pal, int32_t vpal, int32_t vpix);  // orig: 0x00245b60
void map_bytes_select(uint32_t dst, uint32_t a, uint32_t b, uint32_t mask);  // orig: 0x00246f40
int32_t map_buffer_free(int32_t down);                              // orig: 0x00246a60
void map_buffer_move(int32_t to, int32_t from);                     // orig: 0x00246b68
int32_t map_buffer_for_read();                                      // orig: 0x00246ac8
int32_t map_buffer_of(int32_t p);                                   // orig: 0x00246c08
int32_t map_planet_index(int32_t p);                                // orig: 0x00246d88
int32_t map_planet_to_read();                                       // orig: 0x00246c50
int32_t map_buffer_to_reuse();                                      // orig: 0x00246de0
int32_t map_zoom_scroll();                                          // orig: 0x00246fa8
int32_t disc_read_sync(int32_t wait);                               // orig: 0x002660c0
void map_marker_text(int32_t idx, uint32_t out);                    // orig: 0x0024a618
void map_markers_setup(int32_t planet, int32_t hero);               // orig: 0x0024e248
void planet_map_init();                                             // orig: 0x002464a8
void planet_map_page_start();                                       // orig: 0x002468b8
int32_t planet_map_update(uint32_t w);                              // orig: 0x00279aa8
int32_t planet_map_leave(uint32_t w);                               // orig: 0x002799e8
int32_t planet_map_enter(uint32_t w);                               // orig: 0x0027a710

// --- its drawing (planet_map_draw.cpp) ---
void debug_text(int32_t x, int32_t y, uint32_t colour, uint32_t s);        // orig: 0x001f3680
int32_t debug_text_centred(int32_t x, int32_t y, uint32_t colour, uint32_t s);  // orig: 0x001f39e0
void text_med(int32_t x, int32_t y, uint64_t colour, uint32_t s, int32_t n);  // orig: 0x001fb3f0
int32_t text_width_med(uint32_t s, int32_t n);                      // orig: 0x001fb0b0
void sprite_uv(uint64_t t, int32_t x, int32_t y, int32_t lw_, int32_t lh_, int32_t w, int32_t h, int32_t u, int32_t v, int32_t a);  // orig: 0x0023c108
void sprite_rotated(int32_t tw, int32_t th, uint64_t t, float x, float y, float hw, float hh, float ang);  // orig: 0x0023c2a0
void map_slots_draw(int32_t x0, int32_t y0, int32_t x1, int32_t y1);  // orig: 0x0024a8e8
void planet_map_draw();                                             // orig: 0x002471a8
int32_t planet_map_widget_draw(uint32_t w);                         // orig: 0x0027cf78
int32_t planet_picture_draw(uint32_t w);                            // orig: 0x0027e498
int32_t planet_map_legend_draw(uint32_t w);                         // orig: 0x0027cfb0

// --- the missions page (planet_missions.cpp) ---
int32_t missions_list(uint32_t texts, uint32_t done, uint32_t pics, int32_t alt);  // orig: 0x0024deb8
int32_t text_wrapped(int32_t x, int32_t y, int32_t w, int32_t h, uint64_t colour, uint32_t s, int32_t n, uint64_t tex0, uint32_t font);  // orig: 0x001fbb70
int32_t text_wrapped_med(int32_t x, int32_t y, int32_t w, int32_t h, uint64_t colour, uint32_t s, int32_t n);  // orig: 0x001fbe88
void mission_box(int32_t x, int32_t y, int32_t done);              // orig: 0x0027d530
int32_t missions_focus_in(uint32_t w);                             // orig: 0x0027a068
int32_t missions_update(uint32_t w);                               // orig: 0x0027a108
int32_t missions_draw(uint32_t w);                                 // orig: 0x0027d2d0
int32_t mission_picture_update(uint32_t w);                        // orig: 0x0027d5d8
int32_t mission_picture_draw(uint32_t w);                          // orig: 0x0027d8b0

}  // namespace rac1

#endif
