// Draw callbacks of mobys that draw textured quads through the quad drawer (0x1fd228): queued
// by their update (sub_001f93e0 / draw_callback_add) and called with the moby.
#ifndef RAC1_GAME_OVL_EFFECT_QUADS_H
#define RAC1_GAME_OVL_EFFECT_QUADS_H

#include <cstdint>

namespace rac1 {

// orig: 0x002acbf0 — the bomb's two spinning discs (effect textures 0x11 / 0x12) about its
// flight direction (pvars +0x20), at its position (pvars +0x10); not in game mode 2
void bomb_discs_draw(uint32_t m);
// orig: 0x002a7780 — the wrench swing's trail: the swing mesh of its combo step (records of 0x60
// at 0x1d92e8: +0 the four quads lit, +0x8.. their colours, +0x48 the time step, +0x4c the
// vertices, +0x50 the quads (vertex, uv index pairs), +0x54 the uvs, +0x58 the quad count, +0x5c
// the fade) on the hero's matrix taken when it starts (pvars +0..+0x30), fading over its time
// (pvars +0x70)
void wrench_trail_draw(uint32_t m);
// orig: 0x002d19b0 — class 530's mirrored mesh: its 102 vertices (0x1de7b0, normals 0x1df9a0) on
// its matrix (pvars) into 0x1e04a0, sphere-mapped uvs (0x1e0b00) from the view reflected by the
// normals while the camera is within 16 (or it starts), else eased back to the uvs kept at
// 0x1e0e30 over 60 frames (timer pvars +0x40); its 74 quads (0x1dee10) drawn with the texture
// *(0x1618f0), the colour *(0x1618f4) and the blend of 0x1618dc..0x1618ec
void mirror_mesh_draw(uint32_t m);
// orig: 0x002d73e8 — the smoke vent's wisps (smoke_vent.cpp): each wisp of the vent (pvars +0 the
// first, +2 / +0x40 the counts) scrolls its height (0x1d31c0) and is drawn on a camera-facing
// basis (or the reversed one, 0x1d2860) placed by the vent's matrix (*(0x1600ec) + pvars +4 x
// 0x80): rising, two quads (0x1d2d10), else one quad fading with its timers (0x1d5740, 0x1d60a0,
// 0x1d6a00); then, unless pvars +0x42, the six quads of its base
void smoke_wisps_draw(uint32_t m);
// orig: (none) — registers these callbacks for the indirect calls (ovl_call_moby_fn)
void effect_quad_fns_register();

}  // namespace rac1

#endif
