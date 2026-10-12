// Novalis's own draw callbacks registered for the indirect calls (the functions: novalis_draw_cbs_tr.cpp).
#include <game/ovl/novalis_draw_cbs.h>

#include <game/ovl/novalis_mobys.h>
#include <game/ovl/variants_l1.h>

namespace rac1 {

// orig: (none) — registers the draw callbacks for the indirect calls (ovl_call_moby_fn)
void novalis_draw_cbs_register() {
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x00302438u), nov_class832_draw_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x003094f0u), nov_teleporter_draw_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x0030bfa8u), nov_class1510_draw_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x0030de68u), nov_class1818_glow_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x0030f0e0u), nov_class1848_draw_l1);
}

}  // namespace rac1
