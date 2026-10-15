// See variants.h / variants_l12.h. Every row was checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps, then the immediates, the cross-level
// compare of): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l12.h>

namespace rac1 {

namespace {
constexpr VariantRow kHoven[] = {
    // Veldin function, level 12's address, bytes
    // the same code as Veldin's (the matcher could not place them: Veldin's size counts the alignment padding / the next function's first words after the return)
    {0x001fa8f0, 0x0021d3b0, 0x464},  // screen_sprite
    {0x00258b50, 0x00271908, 0x4d4},  // walker_record_step
    {0x00259028, 0x00271e70, 0x18c},  // doomlet_259028
    {0x00259fe8, 0x00272eb8, 0x128},  // floor_light_add
    {0x00262b80, 0x0027b1a0, 0xcc},   // sub_00262b80
    {0x00262e40, 0x0027b668, 0x278},  // path_point
    {0x00263618, 0x0027bcd0, 0x1dc},  // glow_draw
    {0x00263fd8, 0x0027c5d0, 0x348},  // debris_spawn
    {0x002655d8, 0x0027dc60, 0xe0},   // music2_start
    {0x0028dfc8, 0x002a47e0, 0x78},   // sound_emitters_update
    {0x002a3cd8, 0x002ba840, 0x1c8},  // region_find
    {0x002a4530, 0x002bb410, 0x2f8},  // region_height
    {0x002e8648, 0x003107c8, 0x28},   // cam0_set_flag116
    {0x002eac18, 0x00312e18, 0x144},  // hydro_2eac18
    {0x002ec678, 0x00314888, 0x30},   // cam7_step
    {0x0023e268, 0x002567c8, 0x8},    // cutscene_level_extras (a bare `jr ra`; Veldin's holds the branches of levels 0 / 2 / 3)
    {0x0024dbc0, 0x00265db0, 0x28},   // (a DMA wait loop; Veldin's 0x24dbc0 sits after the padding the matcher counted)
    // level 12's own code (transliterated at its addresses: hoven_hero_tr_*.cpp)
    {0x00205000, 0x00227f10, 0xa0},   // sub_00205000
    {0x002062b0, 0x002290e8, 0x3a8},  // hero_move_mode (the update in a movement mode; the Novalis family's 0x3a8-byte build)
    {0x002070d0, 0x00229958, 0x1e8},  // hero_frame_update
    {0x002076a8, 0x00229f20, 0x170},  // sub_002076a8
    {0x0020b960, 0x0022de30, 0x298},  // hero_surface_update
    {0x0020bed0, 0x0022e0c8, 0x7a8},  // hero_ledge_grab_check
    {0x0020c758, 0x0022e870, 0x568},  // hero_ledge_check_v
    {0x00210a08, 0x00232380, 0x108},  // hero_mode_enter
    {0x00210b30, 0x00232488, 0xc8},   // hero_mode_exit
    {0x00210ce8, 0x002325b8, 0x430},  // hero_hurt
    {0x00211380, 0x00232b18, 0x278},  // hero_gait_update
    {0x00212318, 0x00233a38, 0x348},  // hero_wall_slide
    {0x002127b8, 0x00233e80, 0x7b0},  // hero_ground_update
    {0x002133a8, 0x002349e8, 0x3c8},  // hero_collide_push
    {0x00213880, 0x00234e88, 0x510},  // hero_move_integrate
    {0x00214658, 0x00235c08, 0x7f8},  // hero_horizontal_velocity
    {0x00217970, 0x00238900, 0x6778}, // hero_state_update
    {0x002215c8, 0x0023f330, 0x510},  // hero_timers_update
    {0x002223f8, 0x002400d0, 0x3c30}, // hero_set_state
    {0x00227638, 0x00243d00, 0x58},   // hero_enter_idle
    {0x00227fa0, 0x00244638, 0xbc0},  // hero_item_use
    {0x00229b70, 0x002461f0, 0x53c8}, // hero_input_update
    {0x00233210, 0x0024cac0, 0x18},   // hero_mobys_set_flag1
    {0x00233248, 0x0024cad8, 0x20},   // hero_mobys_clear_flag1
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l12(size_t* count) {
    *count = sizeof kHoven / sizeof kHoven[0];
    return kHoven;
}

}  // namespace rac1
