// Pokitaru (level 11): the call helpers of the dispatch lines of the Veldin hero ports. Level 11's own hero controller
// functions (pokitaru_hero.h, uniform ABI) are called with the Veldin port's own arguments in a0..a3 (and f12).
#ifndef RAC1_GAME_OVL_POKITARU_CALL_H
#define RAC1_GAME_OVL_POKITARU_CALL_H

#include <cstdint>

#include <game/ovl/pokitaru_hero.h>
#include <game/ovl/pokitaru_mobys.h>
#include <game/ovl/novalis_mobys2.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/variants_l11.h>
#include <game/math/vecmath.h>

namespace rac1 {

// orig: (host) a uniform-ABI variant of level 11 called with its first four integer arguments (the dispatch lines of the Veldin ports)
template <class F>
inline TrRet pok_call(F f, uint32_t a0 = 0, uint32_t a1 = 0, uint32_t a2 = 0, uint32_t a3 = 0) {
    return f(a0, a1, a2, a3, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

// orig: (host) a uniform-ABI variant of level 11 called with one float argument ($f12)
template <class F>
inline TrRet pok_callf(F f, float f12) {
    return f(0, 0, 0, 0, 0, 0, 0, 0, f12, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

// orig: (host) hero_ledge_check_v (Veldin 0x20c758, level 11's 0x23d240): the position and the reference yaw are passed by EE address (the yaw may be
// null); the yaw is copied in and out through the EE stack
inline int32_t pok_ledge_check_call(const Vec4& pos, const float* ref_yaw) {
    const uint32_t sp = ee_stack_alloc(0x20);
    ee_write<float>(sp, pos.x); ee_write<float>(sp + 4, pos.y); ee_write<float>(sp + 8, pos.z); ee_write<float>(sp + 12, pos.w);
    if (ref_yaw != nullptr) ee_write<float>(sp + 0x10, *ref_yaw);
    const int32_t r = static_cast<int32_t>(pok_call(pok_hero_ledge_check_v_u, sp, ref_yaw != nullptr ? sp + 0x10 : 0u).v0);
    if (ref_yaw != nullptr) *const_cast<float*>(ref_yaw) = ee_read<float>(sp + 0x10);
    ee_stack_free(0x20);
    return r;
}

}  // namespace rac1

#endif
