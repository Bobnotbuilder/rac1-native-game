// The ground enemies' stepping (shared by the classes that walk: Novalis's Peckbot, Birdbot...): the probe of a step and
// the step itself, on the walker record (the pvar block's record that
// 0x26d930 on Novalis / its inlined copy fills: speed +0x10, vertical speed +0x18, acceleration +0x1c,
// drag +0x20, top speed +0x24, flags +0x38, the turn angle +0x4c).
#ifndef RAC1_GAME_OVL_WALKER_H
#define RAC1_GAME_OVL_WALKER_H

#include <cstdint>

#include <runtime/kernel/vu0.h>  // rac1::Vec4

namespace rac1 {

int32_t walker_probe(float up, float down, uint32_t pos, uint32_t self, uint32_t no_moby);  // orig: 0x002583f0
uint32_t walker_record_step(float scale, uint32_t m, uint32_t rec, uint32_t target, uint32_t out);  // orig: 0x00258b50

}  // namespace rac1

#endif
