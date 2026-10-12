// See variants.h / variants_l3.h. Every row was checked by diffing the two functions' instruction
// streams (relocations and registers masked, immediates compared): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l3.h>

namespace rac1 {

namespace {
constexpr VariantRow kKerwan[] = {
    // the same code as Novalis's (the L1 ports run): Veldin function, Kerwan's address, bytes
    {0x00205000, 0x001fe998, 0x90},  // sub_00205000
    {0x00205f60, 0x001ff8d0, 0x48},  // hero_footstep
    {0x002076a8, 0x002008c8, 0x170},  // sub_002076a8
    {0x00233210, 0x00222a20, 0x18},  // hero_mobys_set_flag1
    {0x00233248, 0x00222a38, 0x20},  // hero_mobys_clear_flag1
    {0x0020b960, 0x002047d8, 0x348},  // hero_surface_update
    {0x0020bed0, 0x00204b20, 0x7a8},  // hero_ledge_grab_check
    {0x0020c758, 0x002052c8, 0x568},  // hero_ledge_check
    {0x00210a08, 0x00209140, 0x108},  // hero_mode_enter
    {0x00210b30, 0x00209248, 0xc8},  // hero_mode_exit
    {0x00210ce8, 0x00209378, 0x430},  // hero_hurt
    {0x00211380, 0x002098d8, 0x238},  // hero_gait_update
    {0x00211be8, 0x0020a088, 0xc8},  // hero_stick_speed
    {0x00212318, 0x0020a770, 0x348},  // hero_wall_slide
    {0x002127b8, 0x0020abb8, 0x7c8},  // hero_ground_update
    {0x002133a8, 0x0020b738, 0x3c8},  // hero_collide_push
    {0x00213e68, 0x0020c168, 0x298},  // sub_00213e68
    {0x002215c8, 0x002158a8, 0x510},  // hero_timers_update
    {0x00227638, 0x0021a190, 0x58},  // hero_enter_idle
    {0x002293a8, 0x0021bed0, 0x288},  // hero_land
    // the same code as Aridia's (the L1 ports with the `on_aridia() || on_kerwan()` branches)
    {0x00213880, 0x0020bbd8, 0x510},  // hero_move_integrate
    {0x00214658, 0x0020c950, 0x728},  // hero_horizontal_velocity
    // ports of its own (Novalis's with the swingshot states and the grind rail)
    {0x002070d0, 0x002002f8, 0x1f0},  // hero_frame_update
    {0x00217970, 0x0020ef60, 0x6690},  // hero_state_update
    {0x002223f8, 0x00216648, 0x3b48},  // hero_set_state
    {0x00229b70, 0x0021c668, 0x4eb0},  // hero_input_update
    // the same code as Veldin's (the matcher could not place them: the next function's first words trail the Veldin
    // size; checked word by word, the relocations aside); the Veldin port runs
    {0x00259fe8, 0x00248a50, 0x128},  // floor_light_add
    {0x00263618, 0x002517b0, 0x1d8},  // glow_draw
    {0x00263fd8, 0x002520b0, 0x348},  // debris_spawn
    {0x002655d8, 0x00253740, 0xe0},  // music2_start
    {0x0026ff10, 0x0025e1e8, 0x7f8},  // (Veldin 0x26ff10)
    {0x0028dfc8, 0x0027a8a0, 0x78},  // sound_emitters_update
    {0x002a4530, 0x00290e78, 0x320},  // region_height_sample_v
    {0x002e8648, 0x002e6f30, 0x28},  // cam0_set_flag116
    {0x002ec678, 0x002eb3a0, 0x30},  // cam7_step
    {0x00259028, 0x00247c10, 0x190},  // (Veldin 0x259028)
    {0x0025fc08, 0x0024e650, 0xa8},  // vec_rotate_axis
    {0x00261358, 0x0024fa88, 0x120},  // (Veldin 0x261358)
    {0x00262840, 0x002509f8, 0x80},  // retire_children
    {0x00262e40, 0x00251320, 0x278},  // path_point
    {0x0026a6f0, 0x002588a8, 0x188},  // (Veldin 0x26a6f0)
    {0x0028dd40, 0x0027a588, 0x90},  // level_sound_play
    {0x0024dbc0, 0x0023be98, 0x38},  // (Veldin 0x24dbc0)
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l3(size_t* count) {
    *count = sizeof kKerwan / sizeof kKerwan[0];
    return kKerwan;
}

}  // namespace rac1
