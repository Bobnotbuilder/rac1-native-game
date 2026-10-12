// Class updates that are only `jr ra` (see empty_class_updates.h).
#include <game/ovl/empty_class_updates.h>

#include <game/ovl/ovl.h>

namespace rac1 {

// orig: 0x002cc358
void sonic_summoner_update(uint32_t) {}

// orig: 0x002d3830
void armor_magnetizer_update(uint32_t) {}

// orig: 0x002e0398
void o2_mask_update(uint32_t) {}

// orig: (none) — registers the empty updates for the indirect calls (ovl_call_moby_fn)
void empty_class_updates_register() {
    ovl_register_moby_fn(0x002cc358, sonic_summoner_update);
    ovl_register_moby_fn(0x002d3830, armor_magnetizer_update);
    ovl_register_moby_fn(0x002e0398, o2_mask_update);
}

}  // namespace rac1
