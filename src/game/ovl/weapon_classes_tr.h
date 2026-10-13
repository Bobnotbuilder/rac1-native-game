// The shared (Veldin-address) moby classes no level ported before release 0.2.0: 1143 (the Vendor Orb), 479 (the Drone Device's
// drones), 270 (the Morph-o-Ray's chicken), 428 (the chicken's debris), 804 (on the Morph-o-Ray's hand moby) and 166, the Morph-o-Ray's
// HUD element and particle types 5's and 0x37's updates; transliterated at
// Veldin's addresses (tools/codegen/gen/mkall/veldin_weapon_classes.json), registered by weapon_classes_register().
#ifndef RAC1_GAME_OVL_WEAPON_CLASSES_TR_H
#define RAC1_GAME_OVL_WEAPON_CLASSES_TR_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>

namespace rac1 {

TrRet ovl_2725d8_tr(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // 0x002725d8
TrRet ovl_2cde78_tr(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // 0x002cde78
TrRet ovl_2cdf28_tr(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // 0x002cdf28
TrRet ovl_236128_tr(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // 0x00236128
TrRet ovl_2cbf68_tr(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15);  // 0x002cbf68
void vendor_orb_update_tr(uint32_t a0);  // 0x002df730 (class 1143, the Vendor Orb)
void drone_update_tr(uint32_t a0);       // 0x002ce358 (class 479, the Drone Device's drones)
void chicken_update_tr(uint32_t a0);     // 0x002c97e0 (class 270, the Morph-o-Ray's chicken)
void chicken_hook_c_tr(uint32_t a0);     // 0x002caf10 (class 270's hook +0xc, its class table 0x1ea7d4)
void class428_update_tr(uint32_t a0);    // 0x002cc210 (class 428, the chicken's debris)
void class804_update_tr(uint32_t a0);    // 0x002d7f88 (class 804, on the Morph-o-Ray's hand moby)
void class166_update_tr(uint32_t a0);    // 0x002b3458 (class 166)
void morph_hud_start_tr(uint32_t a0);    // 0x00236b10 (the Morph-o-Ray's HUD element: start, a0 = the slot 0x17dc50 + 4 * 0x90)
void morph_hud_update_tr(uint32_t a0);   // 0x00236d00 (its update, slot +0x14)
void morph_hud_draw_tr(uint32_t a0);     // 0x00236e50 (its draw, slot +0x18)
void particle_05_update_tr(uint32_t a0); // 0x00269c70 (particle type 5's update, the table 0x1b1e80)
void particle_37_update_tr(uint32_t a0); // 0x002726d8 (particle type 0x37's update: the drones' particles, spawned through 0x2725d8)

// Registers the updates and callbacks at Veldin's addresses (ovl_register_moby_fn; every level's own overlay maps its copy to them).
void weapon_classes_register();

}  // namespace rac1

#endif
