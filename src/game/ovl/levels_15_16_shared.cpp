// Shared code (at Veldin's addresses: every level's copy that is the same code runs it) first reached unported on Quartu (level 15)
// or Kalebo III (level 16): the registrations of their transliterations for the indirect calls (`jalr` through a code pointer the
// game holds: the particle table, a moby's update function). Proved by the oracle (batch 241).
#include <game/ovl/levels_15_16_shared.h>
#include <game/ovl/particle_47_tr.h>
#include <game/ovl/ovl_common.h>

namespace rac1 {

// orig: (none) -- see levels_15_16_shared.h
void levels_15_16_shared_register() {
    ovl_register_moby_fn(0x00274e48, particle_47_update);  // (particle type 0x47: Quartu)
}

}  // namespace rac1
