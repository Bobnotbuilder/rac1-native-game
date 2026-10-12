// The title screen: the boot module's title loop, its scene (a level of its own),
// logo, "press start", timed sounds (its music), attract movies and main menu. The boot module's
// own code, ported at its own addresses (`// orig: front:0x...`, see front_image.h); the engine
// code it shares with the levels is the ports' (src/game/ovl) relocated onto it. See

#ifndef RAC1_GAME_BOOT_TITLE_H
#define RAC1_GAME_BOOT_TITLE_H

#include <cstdint>

namespace rac1 {

// --- title_load.cpp ---
void title_texts(int32_t lang);             // orig: front:0x001eaf50
uint32_t title_instances(uint32_t p);       // orig: front:0x001e9b10
void title_load();                          // orig: front:0x001ea830

// --- title.cpp ---
int32_t title_sound_alive(int32_t handle, int32_t snd);          // orig: front:0x001eb740
int32_t title_sound_play(int32_t idx, uint32_t a1, uint32_t a2);  // orig: front:0x0022dba0
void title_sounds_timed();                  // orig: front:0x001eb798 (its timed sounds, each frame)
void title_camera();                        // orig: front:0x001eaf88
void title_sky();                           // orig: front:0x001e9ab8
void title_update();                        // orig: front:0x001eb0a8
void title_draw();                          // orig: front:0x001eb410
void title_menu_open();                     // orig: front:0x00218f68
// The title loop 0x1eb798 split in frames: its start (up to its wait: the frames to wait
// returned), after the wait, then one pass of its loop per frame (0; 1: an attract movie is due,
// the host plays it, then title_attract_after; 2: the loop is left, 0x15f5b0), then its end.
int32_t title_loop_start();                 // orig: front:0x001eb798 (its start)
void title_loop_ready();                    // orig: front:0x001eb798 (after its wait)
int32_t title_loop_frame();                 // orig: front:0x001eb798 (one pass)
void title_attract_after();                 // orig: front:0x001eb798 (after an attract movie)
void title_loop_end();                      // orig: front:0x001eb798 (its end)
// The attract counter (the loop's s3) and its frames without input (s5).
extern int32_t g_title_attract;
extern int32_t g_title_idle;

}  // namespace rac1

#endif
