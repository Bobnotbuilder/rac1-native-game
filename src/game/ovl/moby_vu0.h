// The moby pass's VU0 micro programs, ported (see moby_vu0.cpp).
#ifndef RAC1_GAME_OVL_MOBY_VU0_H
#define RAC1_GAME_OVL_MOBY_VU0_H

#include <runtime/vu/vu_interpreter.h>

namespace rac1 {

// orig: 0x00100090
bool moby_vu0_skin_step(VUState& s, VuPipe& p);
// orig: 0x0010e4d0
bool moby_vu0_shadow_step(VUState& s, VuPipe& p);
// orig: 0x00100090 / 0x0010e4d0 (the same programs as basic blocks, moby_vu0_fast.cpp: observably identical,
// several times faster; RAC1_VU_EXACT=1 runs the exact step functions above instead)
bool moby_vu0_skin_fast(VUState& s, VuPipe& p);
bool moby_vu0_shadow_fast(VUState& s, VuPipe& p);
// orig: (none) registers them with the runtime VU0
void moby_vu0_register();

}  // namespace rac1

#endif
