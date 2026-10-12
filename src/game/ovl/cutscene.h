// Game mode 2: the level's cutscenes (in-engine scenes streamed from the disc block by block, their
// sound the dialog stream), and what starts them: the "talk" scripts of the level's talking mobys
// (0x1b1678[i], one per moby of the list 0x1791b8, records of 0x1c bytes) run near the hero or at
// once, and the Veldin ship moby (class 834) whose script opens the game with scene 4.
// Scene state 0x16c860 (the same struct as game mode 6's, arrival.h): +0x30 the scene, +0x34 the
// frame, +0x38 the frame in the block, +0x3c the block, +0x40 the frames, +0x44 the mobys, +0x46
// put the hero back at +0x10 / +0x20 at the end, +0x48 the dialog's end frame, +0x54 the camera's
// frames, +0x58 / +0x5c the buffers, +0x178.. the mobys. The talk state 0x179100: +8 the moby,
// +0xc its talk struct, +0x10 the frame from which a talk may start, +0x14 the text shown.

#ifndef RAC1_GAME_OVL_CUTSCENE_H
#define RAC1_GAME_OVL_CUTSCENE_H

#include <cstdint>

namespace rac1 {

// ---- game mode 2 (cutscene.cpp)
void cutscene_start(int32_t scene);           // orig: 0x00298840
int32_t cutscene_block_read(int32_t block);   // orig: 0x002451b8
void cutscene_update();                       // orig: 0x00298f90
int32_t cutscene_camera();                    // orig: 0x00298de8
void cutscene_end();                          // orig: 0x00298b18
void cutscene_level_extras();                 // orig: 0x0023e268
void cutscene_subtitle_draw();                // orig: 0x001f9a20

// ---- the talk scripts (talk.cpp); `t` is the moby's talk struct (+4 the record played last,
// +8 starts by itself, +9 set up, +0xc its reach, +0x36 the record, +0x38 its start frame, +0x3c
// the script)
int32_t talker_index(uint32_t m);                       // orig: 0x002667d0
int32_t talk_init(uint32_t m, uint32_t t);              // orig: 0x002668a0
int32_t talk_update(uint32_t m, uint32_t t);            // orig: 0x00266448
void talk_step(uint32_t m, uint32_t t, int32_t next);   // orig: 0x00266970
void level_exit_to(int32_t level);                      // orig: 0x0028eeb0
void veldin_ship_update(uint32_t m);                    // orig: 0x002d9dc8
void talk_fns_register();  // orig: (none) — the class update above for the mobys' indirect calls

}  // namespace rac1

#endif
