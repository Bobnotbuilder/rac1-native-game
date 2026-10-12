// Rilgar's part 2: its own draw callbacks registered for the indirect calls (the functions: rilgar_part2_tr.cpp).
#include <game/ovl/rilgar_part2.h>

#include <game/ovl/novalis_mobys.h>
#include <game/ovl/variants_l5.h>

namespace rac1 {

// orig: (none) -- registers the draw callbacks for the indirect calls (ovl_call_moby_fn)
void rilgar_part2_register() {
    ovl_register_level_moby_fn(RILGAR, L5_ADDR(0x002d6f48u), ril_class79_draw_a_l5);
    ovl_register_level_moby_fn(RILGAR, L5_ADDR(0x002d7920u), ril_class79_draw_b_l5);
    ovl_register_level_moby_fn(RILGAR, L5_ADDR(0x0030c220u), ril_class823_draw_l5);
    ovl_register_level_moby_fn(RILGAR, L5_ADDR(0x0030de90u), ril_class838_draw_l5);
    ovl_register_level_moby_fn(RILGAR, L5_ADDR(0x00316070u), ril_class879_draw_l5);
    ovl_register_level_moby_fn(RILGAR, L5_ADDR(0x0031cf58u), ril_class1511_draw_l5);
    ovl_register_level_moby_fn(RILGAR, L5_ADDR(0x0031e730u), ril_class1818_draw_l5);
}

}  // namespace rac1
