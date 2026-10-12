// The ammo pickups: the update 0x2c53c0 of the item pickup classes 0xcc, 0xd5, 0xd6,
// 0xde, 0xdf, 0xe1, 0xe2, 0x3ee, ... (the pickup a crate or an enemy drops, pickup_spawn 0x2c52a8),
// and its bounding-sphere gap 0x250430. Proved against the original.
#ifndef RAC1_GAME_OVL_AMMO_PICKUP_H
#define RAC1_GAME_OVL_AMMO_PICKUP_H

#include <cstdint>

namespace rac1 {

float moby_sphere_gap(uint32_t a, uint32_t b);  // orig: 0x00250430
void ammo_pickup_update(uint32_t m);            // orig: 0x002c53c0
void ammo_pickup_fns_register();                // orig: (none)

}  // namespace rac1

#endif  // RAC1_GAME_OVL_AMMO_PICKUP_H
