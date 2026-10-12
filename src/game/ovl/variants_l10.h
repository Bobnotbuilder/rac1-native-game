// Orxon (level 10) on its own overlay. Level 10's engine build is Veldin's almost
// everywhere (1897 of Veldin's 1974 functions found in its overlay, 95.8 %): the hero controller and the engine run the Veldin
// ports; what is level 10's own is its class table (62 update functions), a camera type (0x13), a sound emitter type (3), the five
// hooks of class 827's table and the camera mode hook, transliterated at its addresses (`// orig: L10:0x00xxxxxx`, orxon_classes_tr_*.cpp).
// The generic parts (the table rows, on_level) are in variants.h.
#ifndef RAC1_GAME_OVL_VARIANTS_L10_H
#define RAC1_GAME_OVL_VARIANTS_L10_H

#include <cstddef>
#include <cstdint>

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/variants.h>

namespace rac1 {

constexpr int ORXON = 10;

// orig: (host) the current translation target is Orxon overlay
inline bool on_orxon() { return on_level(ORXON); }

// A level-10 address of the port's own (level-only data / code, not a Veldin equivalent): never translated.
#define L10_ADDR(a) (a)

}  // namespace rac1

#endif
