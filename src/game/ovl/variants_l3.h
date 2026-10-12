// Kerwan (level 3) runs its own build of the hero controller and the engine's other variants: the
// same source as Veldin's with other features compiled in (
// section 10). Kerwan's hero module is Novalis's plus the swingshot states (0x24..0x26, 0x2c, 0x2d,
// the same code as Aridia's) and the grind rail 0x74: 21 of its functions are the same code as
// Novalis's (the L1 ports run there: `if (on_novalis() || on_aridia() || on_kerwan())`), two as
// Aridia's, the rest are ports of their own (`// orig: L3:0x00xxxxxx`). The generic parts (the table
// rows, on_level) are in variants.h.
#ifndef RAC1_GAME_OVL_VARIANTS_L3_H
#define RAC1_GAME_OVL_VARIANTS_L3_H

#include <cstddef>
#include <cstdint>

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/variants.h>

namespace rac1 {

constexpr int KERWAN = 3;

// orig: (host) the current translation target is Kerwan's overlay
inline bool on_kerwan() { return on_level(KERWAN); }

// orig: (host) Novalis's hero code is what runs here: the three builds that share the L1 ports
inline bool on_l1_family() { return on_level(1) || on_level(2) || on_level(3); }

// A Kerwan address of the port's own (not a Veldin equivalent: level-only data / code): the same
// on every level's overlay (no Veldin translation); on another region's disc, the region table's.
#define L3_ADDR(a) LVL_ADDR(3, a)
// the same address as a `case` label: its switch compares lvl_key(pointer)
#define L3_CASE(a) (a)
// a `lui` half kept apart from its low half (ovl_reloc.h lvl_page): never translated
#define L3_PAGE(a) LVL_PAGE(a)

}  // namespace rac1

#endif
