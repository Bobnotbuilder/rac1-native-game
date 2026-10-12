// The moby pass's VU1 micro programs, ported (see moby_vu1.cpp).
#ifndef RAC1_GAME_OVL_MOBY_VU1_H
#define RAC1_GAME_OVL_MOBY_VU1_H

#include <runtime/vu/vu_interpreter.h>

namespace rac1 {

// orig: 0x0010faa8
bool moby_vu1_step(VUState& s, VuPipe& p);
// orig: 0x0010faa8 (the same program as basic blocks, moby_vu1_fast.cpp; RAC1_VU_EXACT=1 runs the exact step function above)
bool moby_vu1_fast(VUState& s, VuPipe& p);
// orig: (none) registers it with the VU1 backend
void moby_vu1_register();

}  // namespace rac1

#endif
