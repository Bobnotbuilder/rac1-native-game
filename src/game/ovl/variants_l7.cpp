// See variants.h / variants_l7.h. Every row was checked by diffing the two functions' instruction streams (relocations and
// registers masked, the jal targets compared through the levels' maps, then the immediates, the cross-level
// compare of): same source, other build.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l7.h>

namespace rac1 {

namespace {
constexpr VariantRow kUmbris[] = {
    // Veldin function, level 7's address, bytes
    // the same code as Veldin's (the matcher could not place them: Veldin's size counts the alignment padding / the switch epilogues after the return)
    {0x00239df8, 0x00261c60, 0x41c},
    {0x00259028, 0x00281a50, 0x190},  // doomlet_259028
    {0x002592b8, 0x00281d88, 0x174},
    {0x00261b48, 0x00289fb0, 0x22c},  // path_crossing
    {0x002655d8, 0x0028dcd0, 0xe0},   // music2_start
    {0x0028dfc8, 0x002b4c00, 0x78},   // sound_emitters_update
    {0x002a3cd8, 0x002cac60, 0x1c8},  // region_find
    {0x002a4530, 0x002cb848, 0x2f8},  // region_height
    {0x002e8648, 0x00323fa8, 0x28},   // cam0_set_flag116
    {0x002ec678, 0x00328028, 0x30},   // cam7_step
    // level 7's own code (transliterated at its addresses: umbris_classes_tr_*.cpp)
    {0x0023e268, 0x00265bd8, 0x60},   // cutscene_level_extras (Veldin's holds the branches of levels 0 / 2 / 3; level 7's has its own)
    {0x0028ddd8, 0x002b4a00, 0x8c},   // level_sound_play
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l7(size_t* count) {
    *count = sizeof kUmbris / sizeof kUmbris[0];
    return kUmbris;
}

}  // namespace rac1
