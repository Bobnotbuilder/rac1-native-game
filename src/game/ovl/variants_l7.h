// Umbris (level 7) on its own overlay. Level 7's engine build is Veldin's almost
// everywhere (1887 of Veldin's 1974 functions found in its overlay, 95.5 %): the hero controller and the engine run the Veldin ports; what is
// level 7's own is its class table (52 update functions), camera type 0x12, two sound emitter types, the class hooks +0xc / +0x10, the camera mode
// hook, `level_sound_play` and `cutscene_level_extras`, transliterated at its addresses (`// orig: L7:0x00xxxxxx`, umbris_classes_tr_*.cpp).
// The generic parts (the table rows, on_level) are in variants.h.
#ifndef RAC1_GAME_OVL_VARIANTS_L7_H
#define RAC1_GAME_OVL_VARIANTS_L7_H

#include <cstddef>
#include <cstdint>

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/variants.h>

namespace rac1 {

constexpr int UMBRIS = 7;

// orig: (host) the current translation target is Umbris overlay
inline bool on_umbris() { return on_level(UMBRIS); }

// A level-7 address of the port's own (level-only data / code, not a Veldin equivalent): never translated.
#define L7_ADDR(a) (a)

}  // namespace rac1

#endif
