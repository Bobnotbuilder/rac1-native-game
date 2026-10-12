// See variants.h / variants_l5.h. Every row was checked by diffing the two functions' instruction streams (relocations and
// registers masked, then the immediates, the cross-level compare of): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l5.h>

namespace rac1 {

namespace {
constexpr VariantRow kRilgar[] = {
    // Veldin function, Rilgar's address, bytes
    // the same code as Novalis's (the L1 ports run, the dispatch lines `on_novalis() || on_rilgar()`)
    {0x00205000, 0x00233ce8, 0x90},  // sub_00205000
    {0x00205f60, 0x00234c20, 0x48},  // hero_footstep
    {0x002070d0, 0x00235648, 0x1e8},  // hero_frame_update
    {0x0020bed0, 0x0023a4f8, 0x7a8},  // hero_ledge_grab_check
    {0x0020c758, 0x0023aca0, 0x568},  // hero_ledge_check
    {0x00210a08, 0x0023e830, 0x108},  // hero_mode_enter
    {0x00210b30, 0x0023e938, 0xc8},  // hero_mode_exit
    {0x00210ce8, 0x0023ea68, 0x430},  // hero_hurt
    {0x00211380, 0x0023efc8, 0x238},  // hero_gait_update
    {0x00211be8, 0x0023f778, 0xc8},  // hero_stick_speed
    {0x00212318, 0x0023fe60, 0x348},  // hero_wall_slide
    {0x00213e68, 0x00241a70, 0x298},  // sub_00213e68
    {0x002215c8, 0x0024c148, 0x510},  // hero_timers_update
    {0x00227638, 0x00251288, 0x58},  // hero_enter_idle
    {0x002293a8, 0x00253318, 0x288},  // hero_land
    {0x00233210, 0x0025c5c8, 0x18},  // hero_mobys_set_flag1
    {0x00233248, 0x0025c5e0, 0x20},  // hero_mobys_clear_flag1
    // Rilgar's own code (transliterated at its addresses: rilgar_hero_tr_*.cpp)
    {0x0020b960, 0x00239fc0, 0x538},  // hero_surface_update
    {0x0020fea0, 0x0023dc88, 0x8e8},
    {0x002127b8, 0x002402a8, 0x870},  // hero_ground_update
    {0x002133a8, 0x00240ed0, 0x438},  // hero_collide_push
    {0x00213880, 0x002413e0, 0x610},  // hero_move_integrate
    {0x00214658, 0x00242258, 0x728},  // hero_horizontal_velocity
    {0x00217970, 0x00244a70, 0x7350},  // hero_state_update
    {0x00221310, 0x0024bdc0, 0x388},
    {0x002223f8, 0x0024cee8, 0x43a0},  // hero_set_state
    {0x002279b0, 0x002515d0, 0x3b8},
    {0x00227fa0, 0x00251be8, 0xf00},  // hero_item_use
    {0x0020d5d0, 0x0023b378, 0x88},   // (per-kind parameter: more kinds than Veldin's)
    {0x00229b70, 0x00255960, 0x5738},  // hero_input_update
    {0x002321a8, 0x0025b538, 0x40},  // (the matcher paired it with 0x23f440: its call sites in hero_anim_set / hero_anim_advance call 0x25b538)
    // empty on Rilgar (`jr ra`): Veldin's port does nothing on level 5 either (its branches are keyed on levels 0 / 2 / 3)
    {0x0023e268, 0x002675a0, 0x8},    // cutscene_level_extras
    // the same code as Veldin's (the matcher could not place them: the Veldin port runs; checked word by word, the relocations aside)
    {0x00203338, 0x00232020, 0x20},
    {0x00203730, 0x00232418, 0x14c},
    {0x00203e00, 0x00232ae8, 0x8c4},
    {0x002040e0, 0x00232dc8, 0x5e4},
    {0x002043b0, 0x00233098, 0x314},
    {0x00204428, 0x00233110, 0x29c},
    {0x00204f60, 0x00233c48, 0x1c},
    {0x00239df8, 0x002626e0, 0x3fc},
    {0x0023cc30, 0x00266020, 0x180},
    {0x0023cdb8, 0x00266238, 0xe4},
    {0x002496f8, 0x002726c0, 0x2c},
    {0x00249728, 0x002726f0, 0x58},
    {0x0024ac88, 0x00273c50, 0x24},
    {0x0024acb0, 0x00273c78, 0xb0},
    {0x0024adb0, 0x00273d78, 0x64},
    {0x0024af08, 0x00273ed0, 0x60},
    {0x0024dfe8, 0x00278250, 0x28},
    {0x0024e1c0, 0x00278428, 0x28},
    {0x0024ec80, 0x00278ee8, 0x98},
    {0x00259fe8, 0x00284280, 0x128},
    {0x00261b48, 0x0028b9e8, 0x224},
    {0x00263618, 0x0028d788, 0x1d0},
    {0x002655d8, 0x0028f4e8, 0xe0},
    {0x00266298, 0x002901f8, 0x4c},
    {0x0026888c, 0x002927ec, 0x70},
    {0x0026d700, 0x00297660, 0x39c},
    {0x0026e150, 0x002980b0, 0x250},
    {0x00271be8, 0x0029bb48, 0x98},
    {0x002779c8, 0x002a1cd0, 0x8c},
    {0x0027a6e0, 0x002a49e8, 0x2c},
    {0x0027a710, 0x002a4a18, 0x14},  // planet map (the galactic map's widget function planet_map_enter; the matcher merged it into 0x2a49b0)
    {0x0027ba18, 0x002a5d20, 0x28},
    {0x0027f240, 0x002a9548, 0xc8},
    {0x0028dfc8, 0x002b66a8, 0x78},
    {0x002a3cd8, 0x002cc708, 0x1c4},
    {0x002a4530, 0x002cd2d8, 0x2f4},
    {0x002ec678, 0x00328f18, 0x2c},
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l5(size_t* count) {
    *count = sizeof kRilgar / sizeof kRilgar[0];
    return kRilgar;
}

}  // namespace rac1
