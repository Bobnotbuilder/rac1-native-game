// See variants.h / variants_l6.h. Every row was checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps, then the immediates, the cross-level
// compare of): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l6.h>

namespace rac1 {

namespace {
constexpr VariantRow kBlarg[] = {
    // Veldin function, level 6's address, bytes
    // the same code as Veldin's / another level's (the matcher could not place them: the Veldin ports, or the L1 ports, run)
    {0x001fa8f0, 0x002124d8, 0x460},  // screen_sprite
    {0x00205f60, 0x0021dfe8, 0x48},   // hero_footstep (the same code as Novalis's: the L1 port runs)
    {0x00213e68, 0x0022ac50, 0x298},  // sub_00213e68 (the same code as Novalis's: the L1 port runs)
    {0x00239df8, 0x00249320, 0x3fc},
    {0x0023e268, 0x0024d298, 0x8},    // cutscene_level_extras (empty: Veldin's port does nothing on level 6 either)
    {0x00261b48, 0x002706d8, 0x224},  // path_crossing
    {0x00262840, 0x00271a40, 0x80},   // retire_children
    {0x00263618, 0x00272498, 0x1d0},  // glow_draw
    {0x002655d8, 0x00274370, 0xe0},   // music2_start
    {0x0028dd40, 0x0029ae30, 0x90},
    {0x002e8648, 0x00310668, 0x28},   // cam0_set_flag116
    {0x002ec678, 0x00314748, 0x2c},   // cam7_step
    // level 6's own code (transliterated at its addresses: blarg_hero_tr_*.cpp / blarg_classes_tr_*.cpp)
    {0x001fead8, 0x002167c0, 0x3c8},  // sparkles_draw
    {0x00205000, 0x0021d0a0, 0xa0},   // sub_00205000
    {0x002070d0, 0x0021ef38, 0x1f0},  // hero_frame_update
    {0x0020b960, 0x00222fb8, 0x268},  // hero_surface_update
    {0x00210a08, 0x00227bf0, 0x108},  // hero_mode_enter
    {0x00210b30, 0x00227cf8, 0x118},  // hero_mode_exit
    {0x00210ce8, 0x00227e78, 0x3b8},  // hero_hurt
    {0x00211380, 0x00228360, 0x268},  // hero_gait_update
    {0x00211be8, 0x00228b40, 0x70},   // hero_stick_speed
    {0x00212318, 0x002291d0, 0x380},  // hero_wall_slide
    {0x002127b8, 0x00229650, 0x7c8},  // hero_ground_update
    {0x002133a8, 0x0022a1d0, 0x3c0},  // hero_collide_push
    {0x00214658, 0x0022b438, 0x7b0},  // hero_horizontal_velocity
    {0x00217970, 0x0022e020, 0x65f8},  // hero_state_update
    {0x002215c8, 0x002348d0, 0x540},  // hero_timers_update
    {0x002223f8, 0x002356a0, 0x3e88},  // hero_set_state
    {0x00227638, 0x00239528, 0x78},   // hero_enter_idle
    {0x00227fa0, 0x002398b8, 0xbc0},  // hero_item_use
    {0x002293a8, 0x0023aca8, 0x288},  // hero_land
    {0x00229b70, 0x0023b440, 0x6688},  // hero_input_update
    {0x00259028, 0x00267d60, 0x188},  // doomlet_259028
    {0x0028ddd8, 0x0029aec0, 0xa0},   // level_sound_play
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l6(size_t* count) {
    *count = sizeof kBlarg / sizeof kBlarg[0];
    return kBlarg;
}

}  // namespace rac1
