// The level change: the request that saves the level's state (0x24d430), the level
// exit that loads the next level (0x292e08). See level_change.cpp and

#ifndef RAC1_GAME_OVL_LEVEL_CHANGE_H
#define RAC1_GAME_OVL_LEVEL_CHANGE_H

#include <cstdint>

namespace rac1 {

uint32_t save_checksum(uint32_t p, int32_t n);                               // orig: 0x0024cf78
int32_t save_block_write(uint32_t dst, int32_t index, uint32_t list);        // orig: 0x0024d030
void talkers_save();                                                          // orig: 0x0024ab50
int32_t map_runs_encode(uint32_t dst, int32_t size, uint32_t spans, uint32_t bits);  // orig: 0x00200418
void map_area_reduce(uint32_t dst);                                           // orig: 0x0024a3c8
void map_area_save(uint32_t dst);                                             // orig: 0x00249ea0
bool level_change_request(int32_t force, int32_t level);                     // orig: 0x0024d430

// --- the level exit (level_exit.cpp) ---
int32_t cd_read_request(uint32_t buf, uint32_t lsn, uint32_t n);              // orig: 0x00265f40
int32_t front_read_back();                                                    // orig: 0x00243d38
int32_t level_load_step();                                                    // orig: 0x00243db0
void pad_update();                                                            // orig: 0x00267898
void dma_handlers_remove();                                                   // orig: 0x002a11b8
int32_t tex_image_upload(uint32_t img, uint32_t out);                         // orig: 0x0023f1d8
void loading_screen_setup();                                                  // orig: 0x00291d78
uint32_t flight_camera();                                                     // orig: 0x0028e540
void flight_sequence_update();                                                // orig: 0x0028f8c0
// How the level exit went: the loading screen's loops next (Load), the story's cards and movies
// first (Movie): the host then runs level_exit_frame each frame until it returns true; or the game
// left (Quit).
enum class LevelExit { Load, Movie, Quit };
LevelExit level_exit_start();                                                 // orig: 0x00292e08 (up to its loops)
bool level_exit_frame();                                                      // orig: 0x00292e08 (one pass of its loops)
void level_exit_finish();                                                     // orig: 0x00292e08 (its end)
bool level_exit_files_read();                                                 // orig: (helper) a part of 0x00292e08
bool level_exit_in_story();                                                   // orig: (helper) a part of 0x00292e08
// Whether the loading screen (game mode 6 step 4, the flight 0x2900a0) has been set up and the exit is not over: the memory holds the
// transition's data (its classes, textures, sky), not a level's (the MRP draws it from the "flight" assets)
bool level_exit_loading_screen();                                             // orig: (helper) a part of 0x00292e08

}  // namespace rac1

#endif
