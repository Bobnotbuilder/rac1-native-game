// Umbris (level 7): the call helper of the dispatch lines of the Veldin ports. Level 7's own engine functions (umbris_mobys.h,
// uniform ABI) are called with the Veldin port's own arguments in a0..a3.
#ifndef RAC1_GAME_OVL_UMBRIS_CALL_H
#define RAC1_GAME_OVL_UMBRIS_CALL_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>
#include <game/ovl/umbris_mobys.h>
#include <game/ovl/variants_l7.h>

namespace rac1 {

// orig: (host) a uniform-ABI variant of level 7 called with its first four integer arguments (the dispatch lines of the Veldin ports)
template <class F>
inline TrRet umb_call(F f, uint32_t a0 = 0, uint32_t a1 = 0, uint32_t a2 = 0, uint32_t a3 = 0) {
    return f(a0, a1, a2, a3, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

}  // namespace rac1

#endif
