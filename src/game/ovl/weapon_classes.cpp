// The registrations of the shared (Veldin-address) classes of weapon_classes_tr.h, generated from the disc
// (tools/codegen/gen/mkall/veldin_weapon_classes.json).
#include <game/ovl/ovl.h>
#include <game/ovl/weapon_classes_tr.h>

namespace rac1 {

// orig: (none) — registers the functions for the indirect calls (ovl_call_moby_fn)
void weapon_classes_register() {
    ovl_register_moby_fn(0x002df730, vendor_orb_update_tr);  // (class 1143, the Vendor Orb: spawned by the Vendor's update 0x2a57a8)
    ovl_register_moby_fn(0x002ce358, drone_update_tr);       // (class 479, the drones the Drone Device spawns: 0x2cdcc0 / 0x2ce1f0)
    ovl_register_moby_fn(0x002c97e0, chicken_update_tr);     // (class 270, the Morph-o-Ray's chicken: 0x2c9348)
    ovl_register_moby_fn(0x002caf10, chicken_hook_c_tr);     // (class 270's hook +0xc, the table 0x1ea7d4: called by 0x2dc9f0, which the chicken's update calls)
    ovl_register_moby_fn(0x002cc210, class428_update_tr);    // (class 428, spawned by the chicken's update through 0x2cbf68)
    ovl_register_moby_fn(0x002d7f88, class804_update_tr);    // (class 804, on the Morph-o-Ray's hand moby, class 0xb9)
    ovl_register_moby_fn(0x002b3458, class166_update_tr);    // (class 166)
    ovl_register_moby_fn(0x00236b10, morph_hud_start_tr);    // (the Morph-o-Ray's HUD element, slot 4 key 0x7533, from 0x2bc7e8: start)
    ovl_register_moby_fn(0x00236d00, morph_hud_update_tr);   // (its update)
    ovl_register_moby_fn(0x00236e50, morph_hud_draw_tr);     // (its draw)
    ovl_register_moby_fn(0x00269c70, particle_05_update_tr); // (particle type 5, the table 0x1b1e80)
    ovl_register_moby_fn(0x002726d8, particle_37_update_tr); // (particle type 0x37: the drones' particles)
}

}  // namespace rac1
