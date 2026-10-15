// Particle types 0x47's (Veldin's 0x274e48) and 0x4f's (0x275d68) updates (met on Drek's fleet, level 17), transliterated: the declarations
// of particle_types4_tr.cpp (regen group veldin_particles4), and their registration (particle_types4.cpp).
#ifndef RAC1_GAME_OVL_PARTICLE_TYPES4_TR_H
#define RAC1_GAME_OVL_PARTICLE_TYPES4_TR_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>

namespace rac1 {

void particle_47_update(uint32_t a0);  // 0x00274e48
void particle_4f_update(uint32_t a0);  // 0x00275d68

// orig: (none) -- registers them for the indirect calls (the particle table's slots 0x47 / 0x4f)
void particle_types4_register();

}  // namespace rac1

#endif
