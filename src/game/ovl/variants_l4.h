// Eudora (level 4) on its own overlay. Unlike Novalis and
// Aridia, Eudora's engine build is Veldin's with almost nothing compiled out: the hero controller and every
// engine function the matcher placed are the same code at other addresses (94.6 % of Veldin's functions found
// once; the oracle runs every batch on level 4 with the Veldin ports). What the matcher could not place are
// short or ambiguous functions that are the same code too (checked word by word, the relocations aside: the
// table in variants_l4.cpp) and a few Veldin-only functions (smoke burst, spawn_particle_45, ...) that Eudora
// does not have. So Eudora has no hero variant: its own code is its moby classes (eudora_*.cpp) and its sound
// emitter / camera type functions, ported at Eudora's own addresses (`// orig: L4:0x00xxxxxx`).
#ifndef RAC1_GAME_OVL_VARIANTS_L4_H
#define RAC1_GAME_OVL_VARIANTS_L4_H

#include <cstddef>
#include <cstdint>

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/variants.h>

namespace rac1 {

constexpr int EUDORA = 4;

// orig: (host) the current translation target is Eudora's overlay
inline bool on_eudora() { return on_level(EUDORA); }

// An Eudora address of the port's own (level-only data / code, not a Veldin equivalent): never translated.
#define L4_ADDR(a) (a)

}  // namespace rac1

#endif
