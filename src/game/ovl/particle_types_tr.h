// Particle types 0x43's (Veldin's 0x274518), 0x30's (0x271c88) and 0x32's (0x271df8) updates, 0x33's (0x271ec8), 0x3d's
// (0x273448), 0x41's (0x273ee0) and 0x3a's (0x272bc0) (met on Umbris), transliterated: the declarations of
// particle_types_tr.cpp (regen group veldin_particles).
#ifndef RAC1_GAME_OVL_PARTICLE_TYPES_TR_H
#define RAC1_GAME_OVL_PARTICLE_TYPES_TR_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>

namespace rac1 {

TrRet ovl_259430_tr(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // 0x00259430
TrRet ovl_2743a8_tr(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // 0x002743a8
void particle_43_update(uint32_t a0);  // 0x00274518
void particle_30_update(uint32_t a0);  // 0x00271c88
void particle_32_update(uint32_t a0);  // 0x00271df8
void particle_33_update(uint32_t a0);  // 0x00271ec8
void particle_3d_update(uint32_t a0);  // 0x00273448
void particle_41_update(uint32_t a0);  // 0x00273ee0
void particle_3a_update(uint32_t a0);  // 0x00272bc0

}  // namespace rac1

#endif
