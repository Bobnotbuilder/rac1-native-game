// Game mode 6: the ship sequences. The ship moby *(0x13e030) (struct 0x13e030: +0 the ship, +8
// the backpack moby, +0x10 / +0x14 Ratchet and Clank riding it, +0x20 the step, +0x24 the frames
// in it, +0x26 the ship's variant, +0x28 / +0x2c the start's options, +0x30 the flight path,
// +0x34.. the flight) and a scene streamed from the disc (struct 0x16c860: +0x34 the frame, +0x38
// the frame in the block, +0x3c the block, +0x40 the frames, +0x44 the mobys, +0x54 the camera's
// frames, +0x58 / +0x5c the buffers, +0x60 the blocks, +0x178 the mobys): step 8 the landing on
// a planet, 0 the take-off (then the planet map), 3 the flight to another planet (while it loads),
// 4 the title screen's (not ported).
#ifndef RAC1_GAME_OVL_ARRIVAL_H
#define RAC1_GAME_OVL_ARRIVAL_H

#include <cstdint>

namespace rac1 {

// ---- the scene file (arrival_scene.cpp)
int32_t stream_reads_wait(int32_t wait);          // orig: 0x002660c0
void scene_block_decode();                        // orig: 0x00245270
void scene_block_load(int32_t block);             // orig: 0x00245720
void scene_read(int32_t scene, int32_t fade);     // orig: 0x00245610

// ---- the mobys of the sequences (arrival_mobys.cpp)
void ship_thrusters(uint32_t ship, float z);      // orig: 0x002903c8
void clank_ship_update(uint32_t m);               // orig: 0x00207430
void moby_shadow_probe(uint32_t m);               // orig: 0x0025a1e0
void hero_cam_target_set(uint32_t pos, float t, uint32_t flag);  // orig: 0x00233ee8
void hero_ship_floor_fix();                       // orig: 0x00234d40
void hero_ship_parts(uint32_t m);                 // orig: 0x00234a50
void hero_ship_update(uint32_t m);                // orig: 0x00234b38
void hero_backpack_attach(uint32_t m, uint32_t pack);  // orig: 0x00234c78
void camera_scripted_set(uint32_t pos, uint32_t rot, int32_t kind, uint32_t a3, uint32_t t0);  // orig: 0x002eaaa0
void hero_hold_clear();                           // orig: 0x002118b0
void hero_anim_set(uint32_t seq, uint32_t frame); // orig: 0x002325a0
void moby_light_zone_pick(uint32_t m);            // orig: 0x002500b8
void hero_place(uint32_t pos, uint32_t rot, int32_t state, int32_t cam);  // orig: 0x00216f90

// ---- the sequences (arrival.cpp)
void ship_sequence_start(int32_t step);           // orig: 0x0028e9c8
void ship_sequence_update();                      // orig: 0x00290590
void ship_sequence_draw();                        // orig: 0x00291d00
void mode2_draw();                                // orig: 0x001f8e90
void planet_travel(int32_t planet);               // orig: 0x0028ed58
void ship_takeoff_start();                        // orig: 0x0028ecd8

}  // namespace rac1

#endif
