// Camera types 2 / 3 / 4 of the type table 0x1ea880 (ids 5 / 6 / 7) and the helpers under
// them. Rows of the table (id, accept, init, update, exit):
//   2: 5, 0x2eb210 (v0 = 0), 0x2ead60, 0x2eb218, 0x2eb270 (empty)
//   3: 6, 0x2ebaf8 (v0 = 0), 0x2ebb00, 0x2ebbb0, 0x2ebbd0 (empty)
//   4: 7, 0x2ec6b8 (v0 = 0), 0x2ebbd8, 0x2ec6c0, 0x2ec7b0

#ifndef RAC1_GAME_OVL_CAMERA_TYPES_H
#define RAC1_GAME_OVL_CAMERA_TYPES_H

#include <cstdint>

namespace rac1 {

// helpers
void cam_springs_clear(uint32_t p);                                  // orig: 0x001eb0b0
void cam_angles_copy_turned(uint32_t d, uint32_t s, float yaw);      // orig: 0x001eb0c8
void camera_polar_point(uint32_t out, uint32_t ang, uint32_t base);  // orig: 0x001ed580
void camera_collide_sweep(uint32_t pos, float radius);               // orig: 0x001ed6a8
void vec3_clear_xyz(uint32_t p);                                     // orig: 0x001ff250
float angle_cos_ease(float a, float b, float t);                     // orig: 0x00257ef0
void vec3_cos_ease(uint32_t out, uint32_t a, uint32_t b, float t);   // orig: 0x00257f70
void angles_cos_ease(uint32_t out, uint32_t a, uint32_t b, float t); // orig: 0x00258050

// type 2 (id 5): the scripted camera
void cam5_init(uint32_t cam);    // orig: 0x002ead60
void cam5_step(uint32_t cam);    // orig: 0x002eae20
void cam5_update(uint32_t cam);  // orig: 0x002eb218

// type 3 (id 6): the orbit about a point
void cam6_place(uint32_t cam);   // orig: 0x002eb278
void cam6_reset(uint32_t cam);   // orig: 0x002eb3b0
void cam6_step(uint32_t cam);    // orig: 0x002eb480
void cam6_init(uint32_t cam);    // orig: 0x002ebb00
void cam6_update(uint32_t cam);  // orig: 0x002ebbb0

// type 4 (id 7): the camera of the hero states 0x24..0x26 / 0x2c
void cam7_init(uint32_t cam);    // orig: 0x002ebbd8
void cam7_track(uint32_t cam);   // orig: 0x002ebe70
void cam7_aim(uint32_t cam);     // orig: 0x002ec4a8
void cam7_step(uint32_t cam);    // orig: 0x002ec678
void cam7_update(uint32_t cam);  // orig: 0x002ec6c0
void cam7_exit(uint32_t cam);    // orig: 0x002ec7b0

}  // namespace rac1

#endif
