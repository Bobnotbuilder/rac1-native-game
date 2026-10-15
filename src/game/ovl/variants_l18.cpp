// See variants.h / variants_l18.h. Every row was checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps, then the immediates, the cross-level
// compare of): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l18.h>

namespace rac1 {

namespace {
constexpr VariantRow kVeldinFinale[] = {
    // Veldin function, level 18's address, bytes
    // the same code as Veldin's (the matcher could not place them: Veldin's size counts the alignment padding / the next function's first words after the return)
    {0x001fa8f0, 0x002035d8, 0x464}, // screen_sprite
    {0x00239df8, 0x0023c948, 0x418}, // hud_bolts_draw (the matcher counted the next function, the boss's HUD draw 0x23cd60, in)
    {0x002779c8, 0x00279950, 0x8c},  // menu_label_update (the end-of-game menu's widgets: functions the function-start finder missed)
    {0x0027fed8, 0x00281e60, 0x144}, // end_menu_choice_update
    {0x00280020, 0x00281fd8, 0x14},  // end_menu_page23_focus_in
    {0x00259028, 0x0025b310, 0x190}, // doomlet_259028
    {0x00262840, 0x00264b80, 0x94},  // retire_children
    {0x00262b80, 0x00265188, 0xcc},  // sub_00262b80
    {0x00263618, 0x00265e28, 0x1d0}, // glow_draw
    {0x002655d8, 0x00267a08, 0xe0},  // music2_start
    {0x0028dd40, 0x0028d7e8, 0x94},  // sound_play_indexed
    {0x0028dfc8, 0x0028db00, 0x78},  // sound_emitters_update
    {0x002a99b0, 0x002ab6e8, 0xd8},  // spawn_moby_593
    {0x002d3830, 0x002db458, 0x8},   // armor_magnetizer_update (class 618's update: a bare `jr ra`)
    {0x002d8138, 0x002e2248, 0x7c},  // crate_spawn_companion
    {0x002e8648, 0x00301230, 0x28},  // cam0_set_flag116
    {0x002ec678, 0x003052b0, 0x30},  // cam7_step
    // level 18's cutscene_level_extras is empty (a `jr ra`): Veldin's port does nothing on level 18 either (its branches are keyed on levels 0 / 2 / 3)
    {0x0023e268, 0x002408c0, 0x8},    // cutscene_level_extras
    // level 18's own hero controller build (the Novalis family's; transliterated at its addresses: veldin_finale_hero_tr_*.cpp)
    {0x00205000, 0x0020dde8, 0xa0},  // sub_205000
    {0x00205f60, 0x0020ed30, 0x48},  // hero_footstep
    {0x002062b0, 0x0020efa8, 0x428}, // hero_move_mode
    {0x002070d0, 0x0020f898, 0x1f0}, // hero_frame_update
    {0x002076a8, 0x0020fe00, 0x50},  // sub_2076a8
    {0x002078a8, 0x0020fe50, 0xf8},  // sub_2078a8
    {0x002079e8, 0x0020ff48, 0x8},   // sub_2079e8
    {0x0020b960, 0x00213b40, 0x418}, // hero_surface_update
    {0x0020bed0, 0x00213f58, 0x7a8}, // hero_ledge_grab_check
    {0x0020c758, 0x00214700, 0x568}, // hero_ledge_check_v
    {0x00210b30, 0x00218928, 0x100}, // hero_mode_exit
    {0x00210ce8, 0x00218a90, 0x530}, // hero_hurt
    {0x00211380, 0x002190f0, 0x278}, // hero_gait_update
    {0x00212318, 0x0021a010, 0x378}, // hero_wall_slide
    {0x002127b8, 0x0021a488, 0x7c8}, // hero_ground_update
    {0x00213e68, 0x0021bac8, 0x298}, // sub_213e68
    {0x00214658, 0x0021c2b0, 0x728}, // hero_horizontal_velocity
    {0x00217970, 0x0021eac8, 0x8270}, // hero_state_update
    {0x002215c8, 0x00226ff0, 0x550}, // hero_timers_update
    {0x002223f8, 0x00227dd0, 0x4648}, // hero_set_state
    {0x00227638, 0x0022c418, 0x78},  // hero_enter_idle
    {0x00227fa0, 0x0022cd70, 0xbc0}, // hero_item_use
    {0x002293a8, 0x0022e160, 0x288}, // hero_land
    {0x00229b70, 0x0022e8f8, 0x6a30}, // hero_input_update
    {0x00233210, 0x00236830, 0x18},  // hero_mobys_set_flag1
    {0x00233248, 0x00236848, 0x20},  // hero_mobys_clear_flag1
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l18(size_t* count) {
    *count = sizeof kVeldinFinale / sizeof kVeldinFinale[0];
    return kVeldinFinale;
}

}  // namespace rac1
