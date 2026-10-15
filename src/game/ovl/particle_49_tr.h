// Particle type 0x49's update (Veldin's 0x275320, met on Hoven, level 12), transliterated: the declarations of particle_49_tr.cpp
// (regen group veldin_particle49).
#ifndef RAC1_GAME_OVL_PARTICLE_49_TR_H
#define RAC1_GAME_OVL_PARTICLE_49_TR_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>

namespace rac1 {

TrRet pt49_263468_tr(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // 0x00263468
TrRet pt49_268f10_tr(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // 0x00268f10
TrRet pt49_2634f8_tr(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // 0x002634f8
void particle_49_update(uint32_t a0);  // 0x00275320

}  // namespace rac1

#endif
