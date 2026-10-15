// See variants.h / variants_l16.h. The same-code rows were checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps) and are proved by the per-level oracle (batch 250,
// ORACLE_LEVEL=16): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l16.h>

namespace rac1 {

namespace {
constexpr VariantRow kKalebo[] = {
    // Veldin function, level 16's address, bytes
    // the same code as Veldin's or as another level's row (the matcher could not place them: the alignment padding / a reordered schedule)
    {0x00239df8, 0x00234df0, 0x408},
    {0x0023cc30, 0x00238730, 0x180},
    {0x0023cdb8, 0x00238948, 0xec},
    {0x0023e268, 0x00239cb0, 0x8},    // cutscene_level_extras (a bare `jr ra`; Veldin's holds the branches of levels 0 / 2 / 3)
    {0x00259028, 0x00254778, 0x18c},  // doomlet_259028
    {0x00263618, 0x0025e770, 0x1dc},  // glow_draw
    {0x00263b38, 0x0025ea60, 0x28},   // moby_set_70
    {0x002655d8, 0x002609c8, 0xe0},   // music2_start
    {0x00289420, 0x00284250, 0x180},
    {0x0028dfc8, 0x00286e30, 0x78},   // sound_emitters_update
    {0x002c9348, 0x002c4060, 0x490},  // morph_2c9348
    {0x002ec678, 0x002f3a20, 0x30},   // cam7_step
    // level 16's own code (transliterated at its addresses: kalebo_hero_tr_*.cpp / kalebo_classes_tr_*.cpp)
    {0x00205000, 0x00203868, 0x90},   // sub_00205000
    {0x00205f60, 0x002047a0, 0x48},   // hero_footstep
    {0x002062b0, 0x00204958, 0x3a8},  // hero_move_mode (the Novalis family's build)
    {0x002070d0, 0x002051c8, 0x1f0},  // hero_frame_update
    {0x002076a8, 0x00205798, 0x170},  // sub_002076a8
    {0x0020b960, 0x002097a0, 0x390},  // hero_surface_update
    {0x0020bed0, 0x00209b30, 0x7a8},  // hero_ledge_grab_check
    {0x0020c758, 0x0020a2d8, 0x568},  // hero_ledge_check_v
    {0x0020d5d0, 0x0020afa0, 0x88},   // sub_0020d5d0 (per-kind parameter: Rilgar's build)
    {0x0020fea0, 0x0020d8b0, 0x8e8},  // hero_slot_update (Rilgar's build)
    {0x00210a08, 0x0020e458, 0x108},  // hero_mode_enter
    {0x00210b30, 0x0020e560, 0xc8},   // hero_mode_exit
    {0x00210ce8, 0x0020e690, 0x450},  // hero_hurt
    {0x00211380, 0x0020ec10, 0x238},  // hero_gait_update
    {0x00212318, 0x0020fb68, 0x348},  // hero_wall_slide
    {0x002127b8, 0x0020ffb0, 0x7f8},  // hero_ground_update
    {0x002133a8, 0x00210b60, 0x460},  // hero_collide_push
    {0x00213880, 0x00211098, 0x688},  // hero_move_integrate
    {0x00213e68, 0x002117a0, 0x298},  // sub_00213e68
    {0x00214658, 0x00211f88, 0x728},  // hero_horizontal_velocity
    {0x00217970, 0x00214598, 0x8cd8}, // hero_state_update
    {0x00221310, 0x0021d270, 0x388},  // hero_motion_update (Rilgar's build)
    {0x002215c8, 0x0021d5f8, 0x510},  // hero_timers_update
    {0x002223f8, 0x0021e398, 0x4658}, // hero_set_state
    {0x00227638, 0x002229f0, 0x58},   // hero_enter_idle
    {0x002279b0, 0x00222d38, 0x3b8},  // hero_water_entry (Rilgar's build)
    {0x00227fa0, 0x00223350, 0xf00},  // hero_item_use
    {0x002293a8, 0x00224a80, 0x288},  // hero_land
    {0x00229b70, 0x002270c8, 0x66e0}, // hero_input_update
    {0x002321a8, 0x0022dc48, 0x40},   // sub_002321a8 (Rilgar's build; the matcher paired it with 0x20f088)
    {0x00233210, 0x0022ecd8, 0x18},   // hero_mobys_set_flag1
    {0x00233248, 0x0022ecf0, 0x20},   // hero_mobys_clear_flag1
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l16(size_t* count) {
    *count = sizeof kKalebo / sizeof kKalebo[0];
    return kKalebo;
}

}  // namespace rac1
