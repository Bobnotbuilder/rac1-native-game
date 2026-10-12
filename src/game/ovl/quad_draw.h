// The overlay's textured quad drawer (hand-written EE code with VU0 macro ops): a quad of four
// world points (optionally through a matrix) projected, trivially rejected when all four are out
// of one side of the guard band, else written as a GIF packet (path 2) into the frame's packet
// buffer: fully on screen directly, partly off screen through the triangle clipper 0x1fd89c. Used by the
// ground decals 0x1f9660 and the mobys' blob shadows.
//
// The descriptor (EE address): +0x00..+0x30 the four points, +0x40 their four RGBA words,
// +0x50 / +0x60 their (s, t) pairs, +0x70 CLAMP, +0x78 TEX0, +0x80 TEX1, +0x88 ALPHA.
#ifndef RAC1_GAME_OVL_QUAD_DRAW_H
#define RAC1_GAME_OVL_QUAD_DRAW_H

#include <cstdint>

namespace rac1 {

// orig: 0x001fd228
void quad_draw(uint32_t desc, uint32_t matrix);
// The ground decals (*(0x15f434) entries of 0x20 bytes at 0x16e080: +0 the centre, +0xc the
// size, +0x10 the surface normal): the template quad 0x16c7e0 (x, y offsets, s, t per corner)
// laid on each one's plane, drawn with the effect texture 0, colour 0x40808080, blended.
// orig: 0x001f9660
void decals_draw();
// orig: 0x001fd85c — a CNT + DIRECT tag of `qwc` quadwords at the packet buffer; its data address
uint32_t packet_direct_begin(uint32_t qwc);
// orig: 0x001fd7dc — the clipped quad's GS state (CLAMP, TEX0, TEX1, ALPHA of the descriptor)
void quad_gs_state(uint32_t desc);

}  // namespace rac1

#endif
