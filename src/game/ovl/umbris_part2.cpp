// Umbris's part 2: its own draw callbacks and particle type 0x50 registered for the indirect calls (the functions:
// umbris_part2_tr.cpp).
#include <game/ovl/umbris_part2.h>

#include <game/ovl/novalis_mobys.h>
#include <game/ovl/variants_l7.h>

namespace rac1 {

// orig: (none) -- registers the draw callbacks (queued by the classes' updates through 0x2211c8 / 0x221290 / 0x2213d0) and the
// particle update of type 0x50 (the table entry 0x140 the particle table's setup L7:0x290bfc writes)
void umbris_part2_register() {
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x002f8038u), umb_class460_draw_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x003048b0u), umb_class823_draw_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x00304c40u), umb_class832_draw_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x0030cd20u), umb_class902_draw_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x0030d420u), umb_class1018_draw_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x00312420u), umb_class1106_draw_a_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x00312dc8u), umb_class1106_draw_b_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x003131b0u), umb_class1106_draw_c_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x00317910u), umb_draw_317910_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x0031ce40u), umb_class1135_draw_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x0029e4a0u), umb_particle_50_update_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x00262078u), umb_hud_262078_l7);
    ovl_register_level_moby_fn(UMBRIS, L7_ADDR(0x0030dcf0u), umb_class1046_draw_l7);
}

}  // namespace rac1
