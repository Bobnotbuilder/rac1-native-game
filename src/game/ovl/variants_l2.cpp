// See variants.h / variants_l2.h. Every row was checked by diffing the two functions' instruction
// streams (relocations and registers masked, tools in the session scratch): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l2.h>

namespace rac1 {

namespace {
constexpr VariantRow kAridia[] = {
    // the same code as Novalis's (the L1 ports run): Veldin function, Aridia's address, bytes
    {0x00205000, 0x00216068, 0x90},  // sub_00205000
    {0x00205f60, 0x00216fa0, 0x48},  // hero_footstep
    {0x002070d0, 0x002179c8, 0x1e8},  // hero_frame_update
    {0x002076a8, 0x00217f90, 0x170},  // sub_002076a8
    {0x00233210, 0x00236748, 0x18},  // hero_mobys_set_flag1
    {0x00233248, 0x00236760, 0x20},  // hero_mobys_clear_flag1
    {0x0020bed0, 0x0021ba80, 0x7a8},  // hero_ledge_grab_check
    {0x0020c758, 0x0021c228, 0x568},  // hero_ledge_check
    {0x00210a08, 0x0021fd38, 0x108},  // hero_mode_enter
    {0x00210b30, 0x0021fe40, 0xc8},  // hero_mode_exit
    {0x00212318, 0x00221258, 0x348},  // hero_wall_slide
    {0x00213e68, 0x00222c68, 0x298},  // sub_00213e68
    {0x002215c8, 0x0022a988, 0x510},  // hero_timers_update
    {0x00227638, 0x0022ea90, 0x58},  // hero_enter_idle
    {0x002293a8, 0x002301f0, 0x288},  // hero_land
    // Novalis's ports with Aridia's differences (`on_aridia()` forks inside them)
    {0x00210ce8, 0x0021ff70, 0x348},  // hero_hurt
    {0x00211380, 0x002203e8, 0x220},  // hero_gait_update
    {0x00211be8, 0x00220b80, 0xb8},  // hero_stick_speed
    {0x002127b8, 0x002216a0, 0x818},  // hero_ground_update
    {0x002133a8, 0x00222270, 0x390},  // hero_collide_push
    {0x00213880, 0x002226d8, 0x510},  // hero_move_integrate
    {0x00214658, 0x00223450, 0x728},  // hero_horizontal_velocity
    {0x00227fa0, 0x0022ee00, 0xbc0},  // hero_item_use
    {0x00217970, 0x00225c20, 0x4ab0},  // hero_state_update
    {0x002223f8, 0x0022b728, 0x3368},  // hero_set_state
    {0x00229b70, 0x00230988, 0x48b8},  // hero_input_update
    // ports of its own
    {0x0020b960, 0x0021b698, 0x3e8},  // hero_surface_update
    // the same code as Veldin's (the matcher could not place them: the next function's first words trail the Veldin
    // size; checked word by word, the relocations aside); the Veldin port runs
    {0x001fa8f0, 0x0020b828, 0x460},  // screen_sprite
    {0x0023a7a8, 0x0023c9d8, 0x2f0},  // hud_ammo_draw
    {0x0024edc8, 0x002511b0, 0x108},  // moby_anim_time
    {0x00259fe8, 0x0025c630, 0x128},  // floor_light_add
    {0x00263618, 0x00264b50, 0x1d8},  // glow_draw
    {0x00263fd8, 0x002655b0, 0x348},  // debris_spawn
    {0x002655d8, 0x00266c40, 0xe0},  // music2_start
    {0x0026ff10, 0x00271588, 0x7f8},  // (Veldin 0x26ff10)
    {0x0028dfc8, 0x0028d6c8, 0x78},  // sound_emitters_update
    {0x002a4530, 0x002a3d38, 0x320},  // region_height_sample_v
    {0x002e8648, 0x002f6548, 0x28},  // cam0_set_flag116
    {0x002eaa30, 0x002f89b0, 0x68},  // uid5_set_pvar_90
    {0x002ec678, 0x002fa6a8, 0x30},  // cam7_step
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l2(size_t* count) {
    *count = sizeof kAridia / sizeof kAridia[0];
    return kAridia;
}

}  // namespace rac1
