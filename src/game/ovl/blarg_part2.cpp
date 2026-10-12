// Blarg's Nebula G34 part 2: its own draw callbacks registered for the indirect calls (the functions: blarg_part2_tr.cpp).
#include <game/ovl/blarg_part2.h>

#include <game/ovl/novalis_mobys.h>
#include <game/ovl/variants_l6.h>

namespace rac1 {

// orig: (none) -- registers the draw callbacks for the indirect calls (ovl_call_moby_fn)
void blarg_part2_register() {
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x002d8738u), blg_class302_draw_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x002e2258u), blg_class615_draw_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x002ea830u), blg_class832_draw_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x002f37d0u), blg_class857_draw_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x002f6dd0u), blg_class1035_draw_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x002fd1a0u), blg_class1062_draw_a_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x002fd460u), blg_class1062_draw_b_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x002fd748u), blg_class1062_draw_c_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x002ff680u), blg_class1068_draw_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x003003f0u), blg_class1083_draw_a_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x00300588u), blg_class1083_draw_b_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x00300720u), blg_class1083_draw_c_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x00300958u), blg_class1083_draw_d_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x003021d8u), blg_class1108_draw_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x00305170u), blg_class1123_draw_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x00307578u), blg_class1302_draw_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x0030adb0u), blg_class1818_draw_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x0030bfa8u), blg_class1898_draw_l6);
    ovl_register_level_moby_fn(BLARG, L6_ADDR(0x00249738u), blg_class1051_hud_draw_l6);  // (a HUD element's draw function)
}

}  // namespace rac1
