// Novalis's own sound emitter updates (the update functions of its emitters, +4 of the 0x90-byte
// records at *(0x13f2e4)), transliterated at Novalis's addresses, proved against the original.
#ifndef RAC1_GAME_OVL_NOVALIS_SOUND_TR_H
#define RAC1_GAME_OVL_NOVALIS_SOUND_TR_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>

namespace rac1 {

TrRet nov_2a1968_l1(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // L1:0x002a1968
void nov_emitter_319928_l1(uint32_t a0);  // L1:0x00319928
TrRet nov_2a1a90_l1(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // L1:0x002a1a90
void nov_emitter_319f18_l1(uint32_t a0);  // L1:0x00319f18
void nov_emitter_31a078_l1(uint32_t a0);  // L1:0x0031a078
TrRet nov_27a248_l1(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // L1:0x0027a248
void nov_emitter_31a128_l1(uint32_t a0);  // L1:0x0031a128

}  // namespace rac1

#endif
