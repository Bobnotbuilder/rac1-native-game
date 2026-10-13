// Novalis (level 1) runs its own build of the hero controller: the same source as Veldin's with
// other features compiled in. A Veldin
// function that is different code on Novalis (a "variant": no `fn` line in the generated map) has
// its own port at Novalis's address (// orig: L1:0x00xxxxxx); the Veldin function dispatches to it
// while the level's overlay is the translation target. The generic parts (the table rows, on_level)
// are in variants.h.
#ifndef RAC1_GAME_OVL_VARIANTS_L1_H
#define RAC1_GAME_OVL_VARIANTS_L1_H

#include <cstddef>
#include <cstdint>

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/variants.h>

namespace rac1 {

constexpr int NOVALIS = 1;

// orig: (host) the current translation target is Novalis's overlay
inline bool on_novalis() { return on_level(NOVALIS); }

// A Novalis address of the port's own (not a Veldin equivalent: level-only data / code): the same
// on every level's overlay (no Veldin translation); on another region's disc, the region table's.
#define L1_ADDR(a) LVL_ADDR(1, a)
// the same address as a `case` label: its switch compares lvl_key(pointer)
#define L1_CASE(a) (a)
// a `lui` half kept apart from its low half (ovl_reloc.h lvl_page): never translated
#define L1_PAGE(a) LVL_PAGE(a)

}  // namespace rac1

#endif
