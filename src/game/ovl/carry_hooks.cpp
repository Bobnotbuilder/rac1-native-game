// Ported overlay code: the trivial class hooks of the carried / sucked mobys: the default class's
// four hooks (each returns 0: `jr ra; move v0,zero`) and the hook +0x10 of class 0x10e (its pvars +0x14). The
// generic bodies and the other classes' hooks are transliterated in carry_hooks_asm.cpp.
#include <game/ovl/carry_hooks.h>
#include <game/ovl/ovl_common.h>

namespace rac1 {

// The default class's hook +0.
// orig: 0x002db848
int32_t hook_default_0(uint32_t) { return 0; }

// The default class's hook +4.
// orig: 0x002db850
int32_t hook_default_4(uint32_t) { return 0; }

// The default class's hook +8.
// orig: 0x002db858
int32_t hook_default_8(uint32_t) { return 0; }

// The default class's hook +0xc.
// orig: 0x002db860
int32_t hook_default_c(uint32_t) { return 0; }

// Class 0x10e's hook +0x10: the word at its pvars +0x14 (0 for no moby or no pvars).
// orig: 0x002caf40
uint32_t enemy10e_hook_10(uint32_t m) {
    if (m == 0) return 0;
    const uint32_t pv = lwu(m + 0x78);
    if (pv == 0) return 0;
    return lwu(pv + 0x14);
}

}  // namespace rac1
