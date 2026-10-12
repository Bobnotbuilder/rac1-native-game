// Pokitaru (level 11) on its own overlay. Level 11's engine build is Novalis's family (the hero controller
// has the shapes of Novalis's / Rilgar's / Batalia's, with its own sizes): 1835 of Veldin's 1974 functions are found in its overlay (82.8 %). The hero controller's
// variants, the class table, the camera types, the sound emitters and the class hooks are level 11's own, transliterated at its addresses
// (`// orig: L11:0x00xxxxxx`, pokitaru_hero_tr_*.cpp, pokitaru_classes_tr_*.cpp). The generic parts (the table rows, on_level) are in variants.h.
#ifndef RAC1_GAME_OVL_VARIANTS_L11_H
#define RAC1_GAME_OVL_VARIANTS_L11_H

#include <cstddef>
#include <cstdint>

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/variants.h>

namespace rac1 {

constexpr int POKITARU = 11;

// orig: (host) the current translation target is Pokitaru's overlay
inline bool on_pokitaru() { return on_level(POKITARU); }

// A level-11 address of the port's own (level-only data / code, not a Veldin equivalent): never translated.
#define L11_ADDR(a) (a)

}  // namespace rac1

#endif
