#ifndef RAC1_GAME_OVL_GLOBALS_H
#define RAC1_GAME_OVL_GLOBALS_H

// EE RAM addresses of game globals used by the ported overlay code (Veldin,
// NTSC-U). Named as far as their role is known; the rest keep their address.

#include <cstdint>

#include <game/ovl/ovl_reloc.h>
namespace rac1::g {

constexpr uint32_t GP = 0x00166c00;            // $gp of the overlay

constexpr uint32_t PAD = 0x0013c940;             // pad state
constexpr uint32_t PLAYER = 0x0013f350;          // player / camera block ("lui 0x14; addiu -0xcb0")
constexpr uint32_t HERO_STATE = 0x001413d4;      // player state (PLAYER + 0x2084)
constexpr uint32_t HERO_MOBY = 0x001413d0;       // u32 pointer to the hero moby (PLAYER + 0x2080)
constexpr uint32_t HERO_SUBSTATE = 0x001413dc;   // PLAYER + 0x208c
constexpr uint32_t HERO_MOBY_2 = 0x00140970;     // u32 pointer, second hero-related moby (PLAYER + 0x1620)
constexpr OvConst<0x00167150u> MOBY_TABLE_167150{};  // 48 entries, stride 0xa0
constexpr OvConst<0x0017a680u> FX_PARAMS{};          // "lui 0x18; addiu -0x5980"

}  // namespace rac1::g

#endif
