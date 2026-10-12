#ifndef RAC1_GAME_MATH_VECMATH_H
#define RAC1_GAME_MATH_VECMATH_H

// Vector / matrix / scalar helpers of the Veldin overlay (0x001fef10..0x002002e0).
//
// Faithful ports: every function reproduces the original's arithmetic in the
// same order, including which lanes are written (e.g. vec3_add keeps a.w in
// the result, because the original loads `a` into the VU register and only
// writes .xyz). Verified against the original with the EE interpreter
// (ADR 0006).
//
// Arguments that were EE pointers in the original are host references here;
// pass ee_ref<Vec4>(addr) for data living in EE RAM.

#include <runtime/kernel/vu0.h>  // rac1::Vec4

#include <cstdint>

namespace rac1 {

struct alignas(16) Mat4 {
    Vec4 r[4];
};

// --- frame-rate scaling (g_frame_scale at 0x0015ed68, 1.0 at 60 Hz) --------
float frame_scale_f(float v);          // orig: 0x001fef10
int32_t frame_scale_i(int32_t frames); // orig: 0x001fef20

// --- countdown timers --------------------------------------------------------
int32_t timer_tick_i32(int32_t& t);    // orig: 0x001fef68  (1 idle, 0 running, 2 expired)
int32_t timer_tick_i16(int16_t& t);    // orig: 0x001fef98
int32_t highest_bit_index(int32_t v);  // orig: 0x001feff8

// --- memory -------------------------------------------------------------------
void spin_delay(int32_t n);                               // orig: 0x001ff018
void mem_fill32(uint32_t dst, uint32_t value, int32_t n); // orig: 0x001ff040 (EE addresses)
void mem_clear128(uint32_t dst, int32_t n);               // orig: 0x001ff068
void mem_copy(uint32_t dst, uint32_t src, int32_t n);     // orig: 0x001ff090
void mem_copy128(uint32_t dst, uint32_t src, int32_t n);  // orig: 0x001ff128
void mem_or128(uint32_t dst, uint32_t a, uint32_t b, int32_t n);  // orig: 0x001ff150

// --- scalars ------------------------------------------------------------------
float sqrt_f(float v);                 // orig: 0x001ff1e0
int32_t abs_i(int32_t v);              // orig: 0x001ff200
float abs_f(float v);                  // orig: 0x001ff218
float max_f(float a, float b);         // orig: 0x001ff220
float min_f(float a, float b);         // orig: 0x001ff228
float clampf(float v, float lo, float hi);  // orig: 0x001ff240
int32_t bytes_sum(uint32_t p, int32_t n);   // orig: 0x001ff7b8 (EE address)
float asin_f(float v);                 // orig: 0x001ff818
float atan2_f(float y, float x);       // orig: 0x001ff8b0
float angle_add(float a, float b);     // orig: 0x002000e8  (wrapped to (-pi, pi))
float angle_sub(float a, float b);     // orig: 0x00200130
float angle_dist(float a, float b);    // orig: 0x002001f0
float angle_wrap(float a);             // orig: 0x00200178  (into [-pi, pi))
float split_int_frac(float* int_part, float v);  // orig: 0x00200228
float int_to_float(int32_t v);         // orig: 0x00200240
int32_t float_to_int(float v);         // orig: 0x00200250

// --- vectors ------------------------------------------------------------------
void vec_clear(Vec4& d);                                  // orig: 0x001ff260
void vec_set_unit_z(Vec4& d);                             // orig: 0x001ff268
void vec3_add(Vec4& d, const Vec4& a, const Vec4& b);     // orig: 0x001ff278
void vec3_add_inplace(Vec4& a, const Vec4& b);            // orig: 0x001ff290
void vec3_sub(Vec4& d, const Vec4& a, const Vec4& b);     // orig: 0x001ff2a8
void vec3_lerp(Vec4& d, const Vec4& a, const Vec4& b, float t);  // orig: 0x001ff2d8
void vec3_scale(Vec4& d, const Vec4& a, float s);         // orig: 0x001ff300
void vec4_scale(Vec4& d, const Vec4& a, float s);         // orig: 0x001ff330
void vec3_mul(Vec4& d, const Vec4& a, const Vec4& b);     // orig: 0x001ff348
void vec4_mul(Vec4& d, const Vec4& a, const Vec4& b);     // orig: 0x001ff360
int32_t clip_outcode(const Vec4& p);                      // orig: 0x001ff1b0 (+x -x +y -y +z -z bits)
void vec3_div(Vec4& d, const Vec4& a, float s);           // orig: 0x001ff378
void vec3_reflect(Vec4& d, const Vec4& v, const Vec4& n);  // orig: 0x001ff660
float vec3_dot(const Vec4& a, const Vec4& b);             // orig: 0x001ff398
void vec3_cross_ba(Vec4& d, const Vec4& a, const Vec4& b);// orig: 0x001ff3c0  (= b x a)
float vec3_length(const Vec4& a);                         // orig: 0x001ff3d8
float vec2_length(const Vec4& a);                         // orig: 0x001ff408
float vec3_distance(const Vec4& a, const Vec4& b);        // orig: 0x001ff450
float vec2_distance(const Vec4& a, const Vec4& b);        // orig: 0x001ff488
void vec3_set_length(Vec4& d, const Vec4& a, float len);  // orig: 0x001ff500
void vec2_set_length(Vec4& d, const Vec4& a, float len);  // orig: 0x001ff550
int32_t vec2_clamp_length(Vec4& d, const Vec4& a, float max_len);  // orig: 0x001ff600
uint64_t vec4_to_s16x4(const Vec4& a, float scale);       // orig: 0x001ff768
void rgba_to_vec(Vec4& d, uint32_t c);                    // orig: 0x001ff728
uint32_t vec_to_rgba(const Vec4& a);                      // orig: 0x001ff740
uint32_t rgba_lerp(uint32_t a, uint32_t b, float t);      // orig: 0x00200298

// --- matrices (rows) ------------------------------------------------------------
void mat3_transform(Vec4& d, const Vec4& v, const Mat4& m);   // orig: 0x001ff6d0
void mat4_transform(Vec4& d, const Vec4& v, const Mat4& m);   // orig: 0x001ff6f8
void mat4_identity(Mat4& m);                                  // orig: 0x001ff9e8
void mat3_copy_affine(Mat4& d, const Mat4& s);                // orig: 0x001ffcb8 (row 3 = 0,0,0,1)
void mat3_copy(Mat4& d, const Mat4& s);                       // orig: 0x001ffcd8
void mat3_transpose(Mat4& d, const Mat4& s);                  // orig: 0x001ffcf8
void mat3_mul(Mat4& d, const Mat4& a, const Mat4& b);         // orig: 0x001ffd88
void mat4_mul(Mat4& d, const Mat4& a, const Mat4& b);         // orig: 0x001ffdd8
void quat_mul(Vec4& d, const Vec4& a, const Vec4& b);         // orig: 0x001ffe68

// --- trigonometry and rotations -----------------------------------------------
float cos_vu(float a);                                        // orig: 0x001ff7e8 (VU0 micro 0x190)
float sin_vu(float a);                                        // orig: 0x001ff800 (VU0 micro 0x192)
void euler_to_mat3_vu(Mat4& d, const Vec4& angles);           // orig: 0x001ffa50 (VU0 micro 0x1a3)
void euler_to_mat4_vu(Mat4& d, const Vec4& angles);           // orig: 0x001ffa70
void euler_to_mat4_fpu(Mat4& d, const Vec4& angles);          // orig: 0x001ffa90
void quat_nlerp(Vec4& d, const Vec4& a, const Vec4& b, float t);  // orig: 0x001ffea8
void quat_to_mat3(const Vec4& q, Mat4& m);                    // orig: 0x001fffe8
void quat_to_mat4(const Vec4& q, Mat4& m);                    // orig: 0x00200060
void quat_from_axis_angle(Vec4& d, int32_t axis, float angle);// orig: 0x001fff28 (axis 0=x 1=y 2=z)

}  // namespace rac1

#endif
