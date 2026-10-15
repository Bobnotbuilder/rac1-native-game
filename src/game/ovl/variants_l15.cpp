// See variants.h / variants_l15.h. The same-code rows were checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps) and are proved by the per-level oracle (batch 240,
// ORACLE_LEVEL=15): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l15.h>

namespace rac1 {

namespace {
constexpr VariantRow kQuartu[] = {
    // Veldin function, level 15's address, bytes
    // the same code as Veldin's or as another level's row (the matcher could not place them: the alignment padding / a reordered schedule)
    {0x0024edc8, 0x0023e3c8, 0x100},  // moby_anim_time (the Novalis family's build)
    {0x00258490, 0x00247af8, 0x39c},
    {0x00259028, 0x00248868, 0x18c},  // doomlet_259028
    {0x00259fe8, 0x002498b0, 0x128},  // floor_light_add
    {0x00262840, 0x00251c10, 0x80},   // retire_children
    {0x002655d8, 0x00254b70, 0xe0},   // music2_start
    {0x00274de0, 0x00264790, 0x60},
    {0x0028dfc8, 0x0027bea0, 0x78},   // sound_emitters_update
    {0x002a99b0, 0x0029e9f8, 0xd8},
    {0x002e8648, 0x002f2888, 0x30},   // cam0_set_flag116
    {0x002ec678, 0x002f6918, 0x30},   // cam7_step
    // level 15's own code (transliterated at its addresses: quartu_hero_tr_*.cpp / quartu_classes_tr_*.cpp)
    {0x0023e268, 0x0022db50, 0x90},   // cutscene_level_extras (level 15's branch only)
    {0x00205000, 0x001fe778, 0xa0},   // sub_00205000
    {0x00205f60, 0x001ff6c0, 0x48},   // hero_footstep
    {0x002062b0, 0x001ff938, 0x428},  // hero_move_mode
    {0x002070d0, 0x00200228, 0x1e8},  // hero_frame_update
    {0x002076a8, 0x00200788, 0x50},   // sub_002076a8 (the held item's colour only)
    {0x002078a8, 0x002007d8, 0xf8},   // sub_002078a8 (state 0x6b only)
    {0x002079e8, 0x002008d0, 0x8},    // sub_002079e8 (a bare `jr ra`)
    {0x0020b960, 0x002044c8, 0x400},  // hero_surface_update
    {0x0020bed0, 0x002048c8, 0x7a8},  // hero_ledge_grab_check
    {0x0020c758, 0x00205070, 0x568},  // hero_ledge_check_v
    {0x00210b30, 0x00208ca8, 0x100},  // hero_mode_exit
    {0x00210ce8, 0x00208e10, 0x510},  // hero_hurt
    {0x00211380, 0x00209450, 0x278},  // hero_gait_update
    {0x00211be8, 0x00209c40, 0xc8},   // hero_stick_speed
    {0x00212318, 0x0020a328, 0x378},  // hero_wall_slide
    {0x002127b8, 0x0020a7a0, 0x7c8},  // hero_ground_update
    {0x002133a8, 0x0020b320, 0x3c8},  // hero_collide_push
    {0x00213880, 0x0020b7c0, 0x510},  // hero_move_integrate
    {0x00213e68, 0x0020bd50, 0x298},  // sub_00213e68
    {0x00214658, 0x0020c538, 0x728},  // hero_horizontal_velocity
    {0x00217970, 0x0020ed50, 0x6e50}, // hero_state_update
    {0x002215c8, 0x00215e58, 0x550},  // hero_timers_update
    {0x002223f8, 0x00216c38, 0x3f28}, // hero_set_state
    {0x00227638, 0x0021ab60, 0x78},   // hero_enter_idle
    {0x00227fa0, 0x0021b4b8, 0xbc0},  // hero_item_use
    {0x002293a8, 0x0021c8a8, 0x288},  // hero_land
    {0x00229b70, 0x0021d040, 0x5900}, // hero_input_update
    {0x00233210, 0x00223e48, 0x18},   // hero_mobys_set_flag1
    {0x00233248, 0x00223e60, 0x20},   // hero_mobys_clear_flag1
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l15(size_t* count) {
    *count = sizeof kQuartu / sizeof kQuartu[0];
    return kQuartu;
}

}  // namespace rac1
