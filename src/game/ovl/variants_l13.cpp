// See variants.h / variants_l13.h. Every row was checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps, then the immediates, the cross-level
// compare of): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l13.h>

namespace rac1 {

namespace {
constexpr VariantRow kGemlik[] = {
    // Veldin function, level 13's address, bytes
    // the same code as Veldin's (the matcher could not place them: Veldin's size counts the alignment padding / the next function's first words after the return)
    {0x001fa8f0, 0x002062d8, 0x464},  // screen_sprite
    {0x00239df8, 0x002458e0, 0x418},  // hud_bolts_draw
    {0x00259028, 0x00264df0, 0x18c},  // doomlet_259028
    {0x00259bc8, 0x00265c30, 0x138},
    {0x00262e40, 0x0026eac8, 0x278},  // path_point
    {0x002655d8, 0x00270f90, 0xe0},   // music2_start
    {0x0028de68, 0x00298220, 0xc8},   // level_sound_start
    {0x002a3cd8, 0x002ae648, 0x1c8},  // region_find
    {0x002a4530, 0x002af230, 0x2f8},  // region_height
    {0x002ec678, 0x00316ae0, 0x30},   // cam7_step
    // level 13's cutscene_level_extras is empty (a `jr ra`): Veldin's port does nothing on level 13 either (its branches are keyed on levels 0 / 2 / 3 / 7 / 14 / 15)
    {0x0023e268, 0x00249858, 0x8},    // cutscene_level_extras
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l13(size_t* count) {
    *count = sizeof kGemlik / sizeof kGemlik[0];
    return kGemlik;
}

}  // namespace rac1
