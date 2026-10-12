// The movement-mode hero update of level 2's own build, transliterated: the declaration of hero_move_mode_l2_tr_1.cpp
// (generated with the unported callees stubbed; `hero_move_mode_l2_u` = L2:0x00217158 in the uniform ABI (`TrRet`)).
// hero_frame_update_l1 (hero_l1_small.cpp) calls it while PLAYER+0x20a4 (the movement mode)
// is not 0.
#ifndef RAC1_GAME_OVL_HERO_MOVE_MODE_L2_H
#define RAC1_GAME_OVL_HERO_MOVE_MODE_L2_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>

namespace rac1 {

TrRet hero_move_mode_l2_u(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L2:0x00217158

}  // namespace rac1

#endif
