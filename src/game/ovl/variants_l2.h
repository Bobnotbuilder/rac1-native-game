// Aridia (level 2) runs its own build of the hero controller and the engine's other variants: the
// same source as Veldin's with other features compiled in (
// section 9). Aridia's hero module is close to Novalis's (16 of the 27 functions are the same code:
// the Novalis ports run there, `if (on_novalis() || on_aridia())`), the rest are ports of their own
// (`// orig: L2:0x00xxxxxx`). The generic parts (the table rows, on_level) are in variants.h.
#ifndef RAC1_GAME_OVL_VARIANTS_L2_H
#define RAC1_GAME_OVL_VARIANTS_L2_H

#include <cstddef>
#include <cstdint>

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/variants.h>

namespace rac1 {

constexpr int ARIDIA = 2;

// orig: (host) the current translation target is Aridia's overlay
inline bool on_aridia() { return on_level(ARIDIA); }

// An Aridia address of the port's own (not a Veldin equivalent: level-only data / code): the same
// on every level's overlay (no Veldin translation); on another region's disc, the region table's.
#define L2_ADDR(a) LVL_ADDR(2, a)
// the same address as a `case` label: its switch compares lvl_key(pointer)
#define L2_CASE(a) (a)
// a `lui` half kept apart from its low half (ovl_reloc.h lvl_page): never translated
#define L2_PAGE(a) LVL_PAGE(a)

}  // namespace rac1

#endif
