// See variants.h / variants_l4.h. Every row is the same code as the Veldin function (instruction streams
// diffed with the relocations / registers masked, then word by word with the immediates compared: the
// matcher (tools/overlay_match) placed 1887 of Veldin's functions in Eudora's overlay but left these out,
// mostly short functions whose first words trail the previous function's size). The Veldin port runs on
// Eudora; the table makes the per-level oracle run it against Eudora's own copy and the translation know
// the address.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l4.h>

namespace rac1 {

namespace {
constexpr VariantRow kEudora[] = {
    // Veldin function, Eudora's address, bytes
    {0x001fa8f0, 0x001ed258, 0x45c},  // screen_sprite
    {0x00200510, 0x001f2f78, 0xb8},   // (weapon effect helper)
    {0x00202d78, 0x001f6358, 0x5e0},  // texture loader
    {0x00203338, 0x001f6918, 0x20},   // tie render
    {0x00203730, 0x001f6d10, 0x14c},  // tie render
    {0x00203e00, 0x001f73e0, 0x8c4},  // shrub render
    {0x00204f60, 0x001f8540, 0x1c},   // sky render
    {0x002496f8, 0x0023c0e8, 0x88},
    {0x00249728, 0x0023c118, 0x58},
    {0x0024ac88, 0x0023d678, 0xd8},   // memory card
    {0x0024acb0, 0x0023d6a0, 0xb0},   // memory card
    {0x0024adb0, 0x0023d7a0, 0x64},   // memory card
    {0x0024af08, 0x0023d8f8, 0x60},   // memory card
    {0x0024dfe8, 0x002409d8, 0x28},   // pause menu goals
    {0x0024e1c0, 0x00240bb0, 0x28},   // pause menu goals
    {0x0024ec80, 0x00241670, 0x98},
    {0x0024edc8, 0x002417b8, 0xf4},   // moby_anim_time
    {0x00259740, 0x0024c590, 0xcc},   // point_in_polygon
    {0x0025fc08, 0x00252c38, 0xa8},   // vec_rotate_axis
    {0x00262840, 0x002550d0, 0x80},   // retire_children
    {0x00263fd8, 0x00256768, 0x348},  // debris_spawn
    {0x002655d8, 0x00257df8, 0xe0},   // music2_start
    {0x00266298, 0x00258b08, 0x4c},
    {0x0026888c, 0x0025b0fc, 0x70},   // particles draw
    {0x0026d700, 0x00260090, 0x39c},  // particles
    {0x0026e150, 0x00260ae0, 0x520},  // particles (one global in the bss: +0x180)
    {0x002779c8, 0x0026acf8, 0x8c},   // pause menu widgets
    {0x0027a6e0, 0x0026da10, 0x44},   // pause menu help
    {0x0027a710, 0x0026da40, 0x14},   // planet map
    {0x0027ba18, 0x0026ed48, 0x28},   // pause menu
    {0x0027f240, 0x00272570, 0xc8},   // pause menu help
    {0x002a13a0, 0x00292298, 0x34},   // packet_mark
    {0x002eaa30, 0x002ee738, 0x58},   // uid5_set_pvar_90
    {0x002ec678, 0x002f03e0, 0x2c},   // cam7_step
};
}  // namespace

// orig: (host) see variants.h
const VariantRow* ovl_variants_l4(size_t* count) {
    *count = sizeof kEudora / sizeof kEudora[0];
    return kEudora;
}

}  // namespace rac1
