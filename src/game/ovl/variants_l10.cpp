// See variants.h / variants_l10.h. Every row was checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps, then the immediates, the cross-level
// compare of): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l10.h>

namespace rac1 {

namespace {
constexpr VariantRow kOrxon[] = {
    // Veldin function, level 10's address, bytes
    {0x00259bc8, 0x0024c6b8, 0x138},
    {0x00261b48, 0x00254658, 0x22c},  // path_crossing
    {0x00262840, 0x002559c0, 0x80},   // retire_children
    {0x00262e40, 0x002560c0, 0x278},  // path_point
    {0x00263fd8, 0x00257128, 0x348},  // debris_spawn
    {0x002655d8, 0x002587b8, 0xe0},   // music2_start
    {0x0028dfc8, 0x0027ea58, 0x78},   // sound_emitters_update
    {0x002ec678, 0x002f4ab0, 0x30},   // cam7_step
    // level 10's cutscene_level_extras is empty (a `jr ra`): Veldin's port does nothing on level 10 either (its branches are keyed on levels 0 / 2 / 3)
    {0x0023e268, 0x00231050, 0x8},    // cutscene_level_extras
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l10(size_t* count) {
    *count = sizeof kOrxon / sizeof kOrxon[0];
    return kOrxon;
}

}  // namespace rac1
