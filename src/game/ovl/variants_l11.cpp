// See variants.h / variants_l11.h. Every row was checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps, then the immediates, the cross-level
// compare of): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l11.h>

namespace rac1 {

namespace {
constexpr VariantRow kPokitaru[] = {
    // Veldin function, level 11's address, bytes
    // the same code as Veldin's (the matcher could not place them: Veldin's size counts the alignment padding / the next function's first words after the return)
    {0x001fa8f0, 0x0022c158, 0x464},  // screen_sprite
    {0x00259028, 0x0027eed0, 0x18c},  // doomlet_259028
    {0x00261b48, 0x00286f20, 0x22c},  // path_crossing
    {0x00263618, 0x00288cc0, 0x1dc},  // glow_draw
    {0x00263fd8, 0x002892c8, 0x348},  // debris_spawn
    {0x002655d8, 0x0028a958, 0xe0},   // music2_start
    {0x002a3cd8, 0x002c6eb0, 0x1c8},  // region_find
    {0x002a4530, 0x002c7a80, 0x2f8},  // region_height
    {0x002ec678, 0x00327a28, 0x30},   // cam7_step
    {0x0023e268, 0x00263988, 0x8},    // cutscene_level_extras (a bare `jr ra`; Veldin's holds the branches of levels 0 / 2 / 3)
    {0x0024dbc0, 0x00272f70, 0x38},   // (a DMA wait loop; Veldin's 0x24dbc0 sits after the padding the matcher counted)
    // level 11's own code (transliterated at its addresses: pokitaru_hero_tr_*.cpp / pokitaru_classes_tr_*.cpp)
    {0x00205000, 0x00236968, 0x90},   // sub_00205000
    {0x00205f60, 0x002378a0, 0x48},   // hero_footstep
    {0x002062b0, 0x00237a58, 0x3a8},  // hero_move_mode (the update in a movement mode; Veldin's is 0x958 bytes, the Novalis family's 0x3a8)
    {0x002070d0, 0x002382c8, 0x1e8},  // hero_frame_update
    {0x002076a8, 0x00238890, 0x170},  // sub_002076a8
    {0x0020b960, 0x0023c7a0, 0x2f8},  // hero_surface_update
    {0x0020bed0, 0x0023ca98, 0x7a8},  // hero_ledge_grab_check
    {0x0020c758, 0x0023d240, 0x568},  // hero_ledge_check_v
    {0x00210a08, 0x00240d50, 0x108},  // hero_mode_enter
    {0x00210b30, 0x00240e58, 0xc8},   // hero_mode_exit
    {0x00210ce8, 0x00240f88, 0x430},  // hero_hurt
    {0x00211380, 0x002414e8, 0x238},  // hero_gait_update
    {0x00211be8, 0x00241c98, 0xc8},   // hero_stick_speed
    {0x00212318, 0x00242380, 0x348},  // hero_wall_slide
    {0x002127b8, 0x002427c8, 0x7c8},  // hero_ground_update
    {0x002133a8, 0x00243348, 0x3c8},  // hero_collide_push
    {0x00213880, 0x002437e8, 0x510},  // hero_move_integrate
    {0x00213e68, 0x00243d78, 0x298},  // sub_00213e68
    {0x00214658, 0x00244560, 0x728},  // hero_horizontal_velocity
    {0x00217970, 0x00246b70, 0x5f88}, // hero_state_update
    {0x002215c8, 0x0024cdb0, 0x510},  // hero_timers_update
    {0x002223f8, 0x0024db50, 0x3a08}, // hero_set_state
    {0x00227638, 0x00251558, 0x58},   // hero_enter_idle
    {0x00227fa0, 0x00251e90, 0xbc0},  // hero_item_use
    {0x002293a8, 0x00253280, 0x288},  // hero_land
    {0x00229b70, 0x00253a18, 0x4d60}, // hero_input_update
    {0x00233210, 0x00259c80, 0x18},   // hero_mobys_set_flag1
    {0x00233248, 0x00259c98, 0x20},   // hero_mobys_clear_flag1
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l11(size_t* count) {
    *count = sizeof kPokitaru / sizeof kPokitaru[0];
    return kPokitaru;
}

}  // namespace rac1
