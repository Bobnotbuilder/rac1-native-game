// The nanotech crate's orbs' draw callback: 0x2d9810, queued by the orbs' update
// 0x2d89f0 (nanotech.cpp) through 0x1f93e0, with the two helpers only it calls: the textured
// primitive packet builder 0x1fde98 and the VU0 remainder 0x200260.
#ifndef RAC1_GAME_OVL_NANOTECH_DRAW_H
#define RAC1_GAME_OVL_NANOTECH_DRAW_H

#include <cstdint>

namespace rac1 {

void nanotech_orbs_draw(uint32_t m);  // orig: 0x002d9810
// `n` vertices as one GIF packet (path 2: GIF tag, ST / RGBA / XYZ unpacks, MSCALF 0xc or 0xe) at the frame's
// packet cursor 0x1611c0: xyz 3 words each at `xyz`, rgba 1 word at `rgba`, st 2 words at `st`.
void prim_list_packet(int32_t n, uint32_t xyz, uint32_t rgba, uint32_t st, uint32_t clip);  // orig: 0x001fde98
float float_fmod_vu(float a, float b);  // orig: 0x00200260
void nanotech_draw_fns_register();       // orig: (none) — the callback for the indirect calls

}  // namespace rac1

#endif
