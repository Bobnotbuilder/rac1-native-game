#ifndef RAC1_RUNTIME_KERNEL_VU0_H
#define RAC1_RUNTIME_KERNEL_VU0_H

// VU0/COP2 macro instruction library — C++ equivalents of the PS2 vector
// unit operations used by the game's physics and graphics pipelines.
//
// orig sin routine: vcallms 400 (approx 0x0028b440 call site)
// orig cos routine: vcallms 402
// orig angle wrap:  hero_motion_tick 0x0028b440
// orig capsule math: 0x001efa68

#include <cstddef>
#include <cstdint>

namespace rac1 {

// Q15 fixed-point quaternion structure matching the layout in skeleton update
struct Q15Quat {
    int16_t x, y, z, w;
};

// 16-byte aligned 4-float vector matching the PS2 VF register layout.
struct alignas(16) Vec4 {
    float x, y, z, w;
};

// ---------- Q15 Fixed-Point Quaternion Math ----------

// COP2/VU0 quaternion weighted linear interpolation (nlerp, no normalize)
// Weight: 0x0000 = 0.0, 0x8000 = 1.0
void vu0_nlerp_q15(const Q15Quat* src, const Q15Quat* dst, Q15Quat* out, uint16_t blend_weight);

// COP2/VU0 quaternion composition / multiplication (no normalize)
Q15Quat vu0_quat_mul(const Q15Quat& p, const Q15Quat& q);

// ---------- Trigonometry (simulate vcallms 400/402) ----------

// PS2 VU0 sine approximation matching vcallms 400 behaviour.
// Uses the PS2 PI constant 0x40490fdb.
float vu0_sin(float rad);

// PS2 VU0 cosine approximation matching vcallms 402 behaviour.
float vu0_cos(float rad);

// ---------- Vector arithmetic (VADD / VSUB / VMUL macros) ----------

// VADD: dest[i] = a[i] + b[i]  (xyz components; w left at 0)
void vu0_vec3_add(float dest[4], const float a[4], const float b[4]);

// VSUB: dest[i] = a[i] - b[i]
void vu0_vec3_sub(float dest[4], const float a[4], const float b[4]);

// VMUL scalar: dest[i] = src[i] * scale  (xyz only)
void vu0_vec3_scale(float dest[4], const float src[4], float scale);

// ---------- Angle wrapping ----------

// Wraps (angle + delta) into [-PI, +PI] using the PS2 float constant
// 0x40490fdb.  Mirrors the inline normalisation in hero_motion_tick.
float angle_wrap_normalize(float angle, float delta);

// ---------- Matrix × vector ----------

// 3x4 column-major matrix (rows = m[0..2], each 4 floats) times a vec3.
// dest[3] is always set to 1.0.
// ---------- Camera clip matrix computation ----------

// Builds or validates the world->clip-space projection matrix.
void camera_clip_matrix_vu0();

} // namespace rac1

#endif // RAC1_RUNTIME_KERNEL_VU0_H
