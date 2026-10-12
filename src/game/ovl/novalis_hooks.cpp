// Novalis's class hooks for the Suck Cannon: the dispatch by code address. The hooks themselves are
// transliterated in novalis_hooks_asm.cpp.
#include <game/ovl/novalis_hooks.h>
#include <game/ovl/novalis_mobys.h>
#include <game/ovl/ovl_reloc.h>

namespace rac1 {

// orig: (host) see novalis_hooks.h
bool class_hook_call_l1(uint32_t fn, uint32_t m, uint32_t a1, uint32_t a2, float f12, int32_t& ret) {
    if (ovl_reloc_level() != 1) return false;
    switch (fn) {
    case 0x002f1c78: ret = peckbot_hook0_l1(m, a1, a2, f12); return true;
    case 0x002f1cc8: ret = peckbot_hook4_l1(m, a1, a2, f12); return true;
    case 0x002f1d18: ret = peckbot_hook8_l1(m, a1, a2, f12); return true;
    case 0x002f1dc8: ret = peckbot_hookc_l1(m, a1, a2, f12); return true;
    case 0x002efa88: ret = amoeboid_hook0_l1(m, a1, a2, f12); return true;
    case 0x002efaf8: ret = amoeboid_hook4_l1(m, a1, a2, f12); return true;
    case 0x002efb48: ret = amoeboid_hook8_l1(m, a1, a2, f12); return true;
    case 0x002efb98: ret = amoeboid_hookc_l1(m, a1, a2, f12); return true;
    default: return false;
    }
}

// orig: (host) see novalis_hooks.h
bool class_hook10_call_l1(uint32_t fn, uint32_t m, uint32_t& ret) {
    if (ovl_reloc_level() != 1) return false;
    switch (fn) {
    case 0x002f1df8: ret = peckbot_hook10_l1(m); return true;
    case 0x002efbc8: ret = amoeboid_hook10_l1(m); return true;
    default: return false;
    }
}

namespace {
// (the indirect calls of the original, `jalr` through moby +0xc of a class's hook table, pass only the moby)
// orig: L1:0x002f1c78 (the indirect-call form: the moby only)
void peckbot_hook0_m(uint32_t m) { peckbot_hook0_l1(m, 0, 0, 0.0f); }
// orig: L1:0x002f1cc8 (the indirect-call form: the moby only)
void peckbot_hook4_m(uint32_t m) { peckbot_hook4_l1(m, 0, 0, 0.0f); }
// orig: L1:0x002f1d18 (the indirect-call form: the moby only)
void peckbot_hook8_m(uint32_t m) { peckbot_hook8_l1(m, 0, 0, 0.0f); }
// orig: L1:0x002f1dc8 (the indirect-call form: the moby only)
void peckbot_hookc_m(uint32_t m) { peckbot_hookc_l1(m, 0, 0, 0.0f); }
// orig: L1:0x002efa88 (the indirect-call form: the moby only)
void amoeboid_hook0_m(uint32_t m) { amoeboid_hook0_l1(m, 0, 0, 0.0f); }
// orig: L1:0x002efaf8 (the indirect-call form: the moby only)
void amoeboid_hook4_m(uint32_t m) { amoeboid_hook4_l1(m, 0, 0, 0.0f); }
// orig: L1:0x002efb48 (the indirect-call form: the moby only)
void amoeboid_hook8_m(uint32_t m) { amoeboid_hook8_l1(m, 0, 0, 0.0f); }
// orig: L1:0x002efb98 (the indirect-call form: the moby only)
void amoeboid_hookc_m(uint32_t m) { amoeboid_hookc_l1(m, 0, 0, 0.0f); }
}  // namespace

// orig: (none) -- Novalis's hooks for the indirect calls (ovl_call_moby_fn)
void novalis_hooks_fns_register() {
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f1c78), peckbot_hook0_m);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f1cc8), peckbot_hook4_m);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f1d18), peckbot_hook8_m);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f1dc8), peckbot_hookc_m);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002efa88), amoeboid_hook0_m);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002efaf8), amoeboid_hook4_m);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002efb48), amoeboid_hook8_m);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002efb98), amoeboid_hookc_m);
}

}  // namespace rac1
