// See levels13_14_shared.h.
#include <game/ovl/levels13_14_shared.h>

#include <game/ovl/ovl.h>
#include <game/ovl/particle_44_tr.h>

namespace rac1 {

// orig: (none) -- registers the functions for the indirect calls (ovl_call_moby_fn)
void levels13_14_shared_register() {
    ovl_register_moby_fn(0x00274818, particle_44_update);  // (particle type 0x44: Gemlik Base)
}

}  // namespace rac1
