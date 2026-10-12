// See variants.h / variants_l8.h. Every row was checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps, then the immediates, the cross-level
// compare of): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l8.h>

namespace rac1 {

namespace {
constexpr VariantRow kBatalia[] = {
    // Veldin function, level 8's address, bytes
    // the same code as Veldin's (the matcher could not place them: Veldin's size counts the alignment padding / the switch epilogues after the return)
    {0x001fa8f0, 0x0020d300, 0x464},  // screen_sprite
    {0x00259028, 0x002635a0, 0x190},  // doomlet_259028
    {0x00263618, 0x0026d590, 0x1dc},  // glow_draw
    {0x00263fd8, 0x0026de68, 0x348},
    {0x002655d8, 0x0026f4f8, 0xe0},   // music2_start
    {0x0028dfc8, 0x002962e0, 0x78},   // sound_emitters_update
    {0x002a4530, 0x002ac950, 0x320},  // region_height
    {0x002e8648, 0x003101c8, 0x28},   // cam0_set_flag116
    {0x002eaa30, 0x00313820, 0x60},
    {0x002eac18, 0x00313a68, 0x144},
    {0x0023e268, 0x002481c8, 0x8},    // cutscene_level_extras (empty: Veldin's port does nothing on level 8 either)
    // level 8's own code (transliterated at its addresses: batalia_hero_tr_*.cpp / batalia_classes_tr_*.cpp)
    {0x00205000, 0x00217b80, 0x90},   // sub_00205000
    {0x00205f60, 0x00218ab8, 0x48},   // hero_footstep
    {0x002070d0, 0x002194e0, 0x1f0},  // hero_frame_update
    {0x002076a8, 0x00219ab0, 0x170},  // sub_002076a8
    {0x0020b960, 0x0021d9c0, 0x360},  // hero_surface_update
    {0x0020bed0, 0x0021dd20, 0x7a8},  // hero_ledge_grab_check
    {0x0020c758, 0x0021e4c8, 0x568},  // hero_ledge_check_v
    {0x00210a08, 0x002225c8, 0x108},  // hero_mode_enter
    {0x00210b30, 0x002226d0, 0xc8},   // hero_mode_exit
    {0x00210ce8, 0x00222800, 0x450},  // hero_hurt
    {0x00211380, 0x00222d80, 0x238},  // hero_gait_update
    {0x00212318, 0x00223c60, 0x348},  // hero_wall_slide
    {0x002127b8, 0x002240a8, 0x7c8},  // hero_ground_update
    {0x00213e68, 0x002256e8, 0x298},  // sub_00213e68
    {0x00214658, 0x00225ed0, 0x728},  // hero_horizontal_velocity
    {0x00217970, 0x002284e0, 0x7600}, // hero_state_update
    {0x002215c8, 0x0022fd98, 0x510},  // hero_timers_update
    {0x002223f8, 0x00230b38, 0x40e0}, // hero_set_state
    {0x00227638, 0x00234c18, 0x58},   // hero_enter_idle
    {0x00227fa0, 0x00235550, 0xbc0},  // hero_item_use
    {0x002293a8, 0x00236940, 0x288},  // hero_land
    {0x00229b70, 0x002370d8, 0x5ee0}, // hero_input_update
    {0x00233210, 0x0023e4c0, 0x18},   // hero_mobys_set_flag1
    {0x00233248, 0x0023e4d8, 0x20},   // hero_mobys_clear_flag1
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l8(size_t* count) {
    *count = sizeof kBatalia / sizeof kBatalia[0];
    return kBatalia;
}

}  // namespace rac1
