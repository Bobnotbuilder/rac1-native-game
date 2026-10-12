// Aridia's ocean and the background ships' glows (see aridia_ocean.h): the registrations, and the host's wrapper of the
// ocean's draw callback (the MRP draws VU1 program 7's prim lists in the world's projection: mrp_fx.h). The game's code itself is
// transliterated: aridia_ocean_asm.cpp.
#include <game/ovl/aridia_ocean.h>

#include <game/ovl/aridia_mobys.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/variants_l2.h>
#include <game/render/mrp_fx.h>

namespace rac1 {

// orig: (host) L2:0x2ea048 inside the MRP's prim-list scope (its set-up L2:0x2a40f0 notes the matrix as 0x1fc740 does)
void ari_ocean_draw_l2(uint32_t m) {
    const MrpFxScope fx(kMrpFxPrimList);
    ari_ocean_draw_tr_l2(m);
}

// orig: (none) -- registers the ocean's / glows' functions for the indirect calls (ovl_call_moby_fn)
void aridia_ocean_fns_register() {
    ovl_register_level_moby_fn(ARIDIA, L2_ADDR(0x002ea048u), ari_ocean_draw_l2);           // class 854's draw callback
    ovl_register_level_moby_fn(ARIDIA, L2_ADDR(0x002ec308u), ari_bg_ship1_glow_draw_l2);  // class 1212's draw callback
    ovl_register_level_moby_fn(ARIDIA, L2_ADDR(0x002ecc68u), ari_bg_ship2_glow_draw_l2);  // class 1213's draw callback
    ovl_register_level_moby_fn(ARIDIA, L2_ADDR(0x002ebf20u), ari_class1208_update_l2);    // class 1208's update
}

}  // namespace rac1
