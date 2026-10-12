// See variants.h / variants_l9.h. Every row was checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps, then the immediates, the cross-level
// compare of): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l9.h>

namespace rac1 {

namespace {
constexpr VariantRow kGaspar[] = {
    // Veldin function, level 9's address, bytes
    // the same code as Veldin's (the matcher could not place them: Veldin's size counts the alignment padding / the switch epilogues after the return)
    {0x001fead8, 0x0021e1a0, 0x3c0},  // sparkles_draw
    {0x00259028, 0x00278af0, 0x188},  // doomlet_259028
    {0x00263fd8, 0x00282e18, 0x348},
    {0x0028dfc8, 0x002aa8a8, 0x78},   // sound_emitters_update
    {0x002a4530, 0x002c0e80, 0x320},  // region_height
    {0x002eac18, 0x00312748, 0x140},
    {0x002ec678, 0x003141b8, 0x30},   // cam7_step
    // level 9's cutscene_level_extras is empty (a `jr ra`): Veldin's port does nothing on level 9 either (its branches are keyed on levels 0 / 2 / 3)
    {0x0023e268, 0x0025d6c8, 0x8},    // cutscene_level_extras
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l9(size_t* count) {
    *count = sizeof kGaspar / sizeof kGaspar[0];
    return kGaspar;
}

}  // namespace rac1
