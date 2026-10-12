#ifndef RAC1_GAME_LIB_LIBVU0_H
#define RAC1_GAME_LIB_LIBVU0_H

// Ports of the Sony libvu0 helpers in the boot ELF that game code calls (VU0 macro code,
// computed with the VU float rules).

#include <game/math/vecmath.h>
#include <runtime/kernel/vu0.h>

namespace rac1 {

// orig: 0x00125180 — sceVu0Normalize: d.xyz = s.xyz / |s.xyz|, d.w = 0.
void vu0_normalize(Vec4& d, const Vec4& s);

// orig: 0x00125218 — sceVu0UnitMatrix: m = identity.
void vu0_unit_matrix(Mat4& m);
// orig: 0x00125240 — the libvu0 sine kernel (VU0 macro, the odd polynomial of the table 0x132e00):
// s = sin(x) as the lane .y; .x = sqrt(1 - s^2), negated when `neg`. The rotations pass x =
// pi/2 -+ a, giving (sin a, cos a).
void vu0_sincos(float x, bool neg, float& out_x, float& out_y);
// orig: 0x001252b8 — sceVu0RotMatrixZ: d = the rotation by `a` about z applied to the rows of s.
void vu0_rot_matrix_z(Mat4& d, const Mat4& s, float a);
// orig: 0x00125360 — sceVu0RotMatrixX: about x.
void vu0_rot_matrix_x(Mat4& d, const Mat4& s, float a);
// orig: 0x00125408 — sceVu0RotMatrixY: about y.
void vu0_rot_matrix_y(Mat4& d, const Mat4& s, float a);

}  // namespace rac1

#endif
