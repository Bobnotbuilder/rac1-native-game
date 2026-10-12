// See kerwan_hooks.h.
#include <game/ovl/kerwan_hooks.h>
#include <game/ovl/kerwan_part2_tr.h>
#include <game/ovl/ovl_reloc.h>

namespace rac1 {

// orig: (host) see kerwan_hooks.h
bool kerwan_sound_emitter_call(uint32_t fn, uint32_t em) {
    switch (fn) {
    case L3_ADDR(0x002ed800): kerwan_emitter_2ed800_l3(em); return true;
    case L3_ADDR(0x002ed960): kerwan_emitter_2ed960_l3(em); return true;
    default: return false;
    }
}

}  // namespace rac1
