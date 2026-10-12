// See variants_l1.h. Every row was checked by diffing the two functions' instruction streams
// (relocations and registers masked): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l1.h>

namespace rac1 {

namespace {
constexpr VariantRow kNovalis[] = {
    {0x00205000, 0x00226f10, 0x90},  // sub_00205000
    {0x00205f60, 0x00227e48, 0x48},  // hero_footstep
    {0x002070d0, 0x00228870, 0x1e8},  // hero_frame_update
    {0x002076a8, 0x00228e38, 0x170},  // sub_002076a8
    {0x00233210, 0x002488e8, 0x18},  // hero_mobys_set_flag1
    {0x00233248, 0x00248900, 0x20},  // hero_mobys_clear_flag1
    {0x0020b960, 0x0022cd48, 0x348},  // hero_surface_update
    {0x0020c758, 0x0022d838, 0x568},  // hero_ledge_check
    {0x0020bed0, 0x0022d090, 0x7a8},  // hero_ledge_grab_check
    {0x00210a08, 0x00231348, 0x108},  // hero_mode_enter
    {0x00210b30, 0x00231450, 0xc8},  // hero_mode_exit
    {0x00210ce8, 0x00231580, 0x430},  // hero_hurt
    {0x00211380, 0x00231ae0, 0x238},  // hero_gait_update
    {0x00211be8, 0x00232290, 0xc8},  // hero_stick_speed
    {0x00212318, 0x00232978, 0x348},  // hero_wall_slide
    {0x002127b8, 0x00232dc0, 0x7c8},  // hero_ground_update
    {0x002133a8, 0x00233940, 0x3c8},  // hero_collide_push
    {0x00213880, 0x00233de0, 0x4f8},  // hero_move_integrate
    {0x00213e68, 0x00234358, 0x298},  // sub_00213e68
    {0x00214658, 0x00234b40, 0x690},  // hero_horizontal_velocity
    {0x002215c8, 0x0023c710, 0x510},  // hero_timers_update
    {0x00227638, 0x002405a0, 0x58},  // hero_enter_idle
    {0x002293a8, 0x00242198, 0x288},  // hero_land
    {0x00227fa0, 0x00240ed8, 0xa90},  // hero_item_use
    {0x00217970, 0x002370b8, 0x53a0},  // hero_state_update
    {0x002223f8, 0x0023cf98, 0x3608},  // hero_set_state
    {0x00229b70, 0x00242930, 0x4ab0},  // hero_input_update
    // the same code as Veldin's (the matcher could not place them: the next function's first words trail the Veldin size); the Veldin port runs
    {0x0024edc8, 0x00263920, 0xf4},  // moby_anim_time
    {0x00261b48, 0x00276820, 0x224},  // path_crossing
    {0x00262e40, 0x00277d40, 0x278},  // path_point
    {0x00263618, 0x002781d0, 0x1d0},  // glow_draw
    {0x00263fd8, 0x00278ad8, 0x348},  // debris_spawn
    {0x002655d8, 0x0027a168, 0xe0},  // music2_start
    {0x0028dfc8, 0x002a1a18, 0x78},  // sound_emitters_update
    {0x0028e990, 0x002a2450, 0x2c},  // ship_hide
    {0x002a3cd8, 0x002b7d28, 0x1c4},  // region_find
    {0x002a4530, 0x002b8910, 0x2f4},  // region_height
    {0x002eaa30, 0x00316e28, 0x58},  // uid5_set_pvar_90
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l1(size_t* count) {
    *count = sizeof kNovalis / sizeof kNovalis[0];
    return kNovalis;
}

}  // namespace rac1
