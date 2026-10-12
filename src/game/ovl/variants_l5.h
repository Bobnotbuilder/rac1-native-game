// Rilgar (level 5) on its own overlay. Rilgar's engine build
// is between Aridia's and Eudora's: 1846 of Veldin's 1974 functions are found in its overlay (83.5 %); the hero module
// is partly the same code as Novalis's (the L1 ports run there: `if (on_novalis() || on_rilgar())`), partly Rilgar's
// own (transliterated at Rilgar's addresses, `// orig: L5:0x00xxxxxx`, rilgar_hero_tr_*.cpp), and the rest of the
// engine is Veldin's. The generic parts (the table rows, on_level) are in variants.h.
#ifndef RAC1_GAME_OVL_VARIANTS_L5_H
#define RAC1_GAME_OVL_VARIANTS_L5_H

#include <cstddef>
#include <cstdint>

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/variants.h>

namespace rac1 {

constexpr int RILGAR = 5;

// orig: (host) the current translation target is Rilgar's overlay
inline bool on_rilgar() { return on_level(RILGAR); }

// A Rilgar address of the port's own (level-only data / code, not a Veldin equivalent): never translated.
#define L5_ADDR(a) (a)

}  // namespace rac1

#endif
