// The spawner of particle type 0x16 (sparks: the Birdbot's and Novalis's rocks' sparks, other levels' scene effects; the
// type's update is particle_16_update 0x00267b10).
#ifndef RAC1_GAME_OVL_PARTICLE_16_SPAWN_H
#define RAC1_GAME_OVL_PARTICLE_16_SPAWN_H

#include <cstdint>

namespace rac1 {

void particle_16_spawn(uint32_t pos, uint32_t vel, uint32_t c_start, uint32_t c_end, int32_t frames, float size);  // orig: 0x0026ced0
void particle_20_update(uint32_t p);  // orig: 0x00271fc0  (particle type 0x20)

}  // namespace rac1
#endif
