// See variants.h / variants_l17.h. Every row was checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps, then the immediates, the cross-level
// compare of): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l17.h>

namespace rac1 {

namespace {
constexpr VariantRow kVeldinOrbit[] = {
    // Veldin function, level 17's address, bytes
    // the same code as Veldin's (the matcher could not place them: Veldin's size counts the alignment padding after the return)
    {0x0024edc8, 0x002494c0, 0xf8},   // moby_anim_time
    {0x00259028, 0x002537e0, 0x190},  // doomlet_259028
    {0x00263fd8, 0x0025dcd8, 0x348},  // debris_spawn
    {0x002655d8, 0x0025f368, 0xe0},   // music2_start
    {0x00274de0, 0x0026e918, 0x60},   // particle_46_update
    {0x002ec678, 0x002ffed0, 0x30},   // cam7_step
    // level 17's cutscene_level_extras is empty (a `jr ra`): Veldin's port does nothing on level 17 either (its branches are keyed on levels 0 / 2 / 3)
    {0x0023e268, 0x00238cd0, 0x8},    // cutscene_level_extras
    // level 17's own hero controller build (transliterated at its addresses: veldin_orbit_hero_tr_*.cpp)
    {0x00205000, 0x002020a8, 0xb0},   // sub_00205000
    {0x002062b0, 0x00203290, 0x8d0},  // hero_move_mode (the update in a movement mode; Veldin's is 0x958 bytes)
    {0x00210a08, 0x0020d960, 0x108},  // hero_mode_enter
    {0x00210b30, 0x0020da68, 0x118},  // hero_mode_exit
    {0x00210ce8, 0x0020dbe8, 0x4a8},  // hero_hurt
    {0x00211380, 0x0020e1c0, 0x2c0},  // hero_gait_update
    {0x00212318, 0x0020f128, 0x380},  // hero_wall_slide
    {0x00217970, 0x00214760, 0x8d48}, // hero_state_update
    {0x002215c8, 0x0021d760, 0x540},  // hero_timers_update
    {0x002223f8, 0x0021e530, 0x4e50}, // hero_set_state
    {0x00227638, 0x00223380, 0x78},   // hero_enter_idle
    {0x00229b70, 0x002258a8, 0x7748}, // hero_input_update
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l17(size_t* count) {
    *count = sizeof kVeldinOrbit / sizeof kVeldinOrbit[0];
    return kVeldinOrbit;
}

}  // namespace rac1
