// Veldin's engine functions first reached unported on Hoven (see hoven_shared.h).
#include <game/ovl/hoven_shared.h>

#include <game/ovl/ovl_common.h>
#include <game/ovl/particle_49_tr.h>

namespace rac1 {

// orig: (none) -- registers the functions for the indirect calls (ovl_call_moby_fn)
void hoven_shared_fns_register() {
    ovl_register_moby_fn(0x00275320, particle_49_update);  // (particle type 0x49: met on Hoven, L12:0x28dca8)
}

}  // namespace rac1
