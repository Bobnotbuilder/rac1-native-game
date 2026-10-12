// Novalis (level 1) camera type 3 (the path camera of the slides and rails, hero camera request 0x1415d4 = 3): init / update / exit and their level-only callee, transliterated.
#ifndef RAC1_GAME_OVL_NOVALIS_CAMERA_TR_H
#define RAC1_GAME_OVL_NOVALIS_CAMERA_TR_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>

namespace rac1 {

TrRet nov_314e98_l1(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // L1:0x00314e98
void nov_cam_type3_init_l1(uint32_t a0);  // L1:0x00315de0
void nov_cam_type3_update_l1(uint32_t a0);  // L1:0x00315358
void nov_cam_type3_exit_l1(uint32_t a0);  // L1:0x00316030

}  // namespace rac1

#endif
