// The movement-mode hero update, transliterated: the declaration of hero_move_mode_tr_N.cpp (generated with the unported callees stubbed;
// `hero_move_mode_u` = Veldin 0x002062b0 in the uniform ABI (`TrRet`); the same code on Eudora, Umbris, Gaspar and Orxon). hero_frame_update (hero_update.cpp) calls it while
// PLAYER+0x20a4 (the movement mode) is not 0.
#ifndef RAC1_GAME_OVL_HERO_MOVE_MODE_H
#define RAC1_GAME_OVL_HERO_MOVE_MODE_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>

namespace rac1 {

TrRet hero_move_mode_u(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // 0x002062b0

}  // namespace rac1

#endif
