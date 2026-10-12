// Aridia's (level 2) own variants of the hero controller functions: see variants_l2.h.
// Where Aridia's build is the same code as Novalis's the L1 ports run (hero_l1.h; the ports' annotation
// lists both addresses); the functions here are the ones with code of their own, `// orig: L2:0x...`.
// Each Veldin function of the same name dispatches to them while the translation target is Aridia's
// overlay (`on_aridia()`).
#ifndef RAC1_GAME_OVL_HERO_L2_H
#define RAC1_GAME_OVL_HERO_L2_H

#include <cstdint>

#include <game/math/vecmath.h>
#include <game/ovl/hero_l1.h>
#include <game/ovl/variants_l2.h>

namespace rac1 {

void hero_surface_update_l2();  // L2:0x0021b698 (Veldin 0x20b960)

}  // namespace rac1

#endif
