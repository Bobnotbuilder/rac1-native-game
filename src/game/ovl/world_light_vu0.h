// The world's static lighting VU0 micro programs, ported (see world_light_vu0.cpp).
#ifndef RAC1_GAME_OVL_WORLD_LIGHT_VU0_H
#define RAC1_GAME_OVL_WORLD_LIGHT_VU0_H

#include <runtime/vu/vu_interpreter.h>

namespace rac1 {

// orig: 0x00100af0
bool world_light_vu0_step(VUState& s, VuPipe& p);
// orig: 0x00100af0 (the same program as basic blocks, world_light_vu0_fast.cpp: observably identical, several times faster;
// RAC1_VU_EXACT=1 runs the exact step function above instead)
bool world_light_vu0_fast(VUState& s, VuPipe& p);
// orig: (none) registers it with the runtime VU0
void world_light_vu0_register();

}  // namespace rac1

#endif
