// MIT. Narrow, independently implemented ARM64 adapter for the intrinsics used by vu0_regs.h.
// Only add ios/simd to APPLE ARM64 include paths. Desktop builds keep their system SSE header.
// This is native NEON, not a MIPS interpreter, and does not change PS2 arithmetic rules.
// Missing intrinsics deliberately fail compilation; do not add successful-looking stubs.
#pragma once
#if !defined(__aarch64__)
#error The iOS NEON adapter must never override the desktop SSE implementation
#endif
#include <arm_neon.h>
#include <cstdint>

using __m128i = uint32x4_t;
using __m128 = float32x4_t;
inline __m128i _mm_load_si128(const __m128i* p) { return vld1q_u32(reinterpret_cast<const uint32_t*>(p)); }
inline void _mm_store_si128(__m128i* p, __m128i v) { vst1q_u32(reinterpret_cast<uint32_t*>(p), v); }
inline void _mm_store_ps(float* p, __m128 v) { vst1q_f32(p, v); }
inline __m128i _mm_setzero_si128() { return vdupq_n_u32(0); }
inline __m128i _mm_set1_epi32(int n) { return vdupq_n_u32(static_cast<uint32_t>(n)); }
inline __m128i _mm_set_epi32(int w, int z, int y, int x) {
    return __m128i{static_cast<uint32_t>(x), static_cast<uint32_t>(y), static_cast<uint32_t>(z), static_cast<uint32_t>(w)};
}
inline __m128 _mm_set_ps(float w, float z, float y, float x) { return __m128{x, y, z, w}; }
inline __m128i _mm_and_si128(__m128i a, __m128i b) { return vandq_u32(a, b); }
inline __m128i _mm_andnot_si128(__m128i a, __m128i b) { return vbicq_u32(b, a); }
inline __m128i _mm_or_si128(__m128i a, __m128i b) { return vorrq_u32(a, b); }
inline __m128i _mm_cmpeq_epi32(__m128i a, __m128i b) { return vceqq_u32(a, b); }
inline __m128 _mm_castsi128_ps(__m128i a) { return vreinterpretq_f32_u32(a); }
inline __m128i _mm_castps_si128(__m128 a) { return vreinterpretq_u32_f32(a); }
inline __m128 _mm_add_ps(__m128 a, __m128 b) { return vaddq_f32(a, b); }
inline __m128 _mm_sub_ps(__m128 a, __m128 b) { return vsubq_f32(a, b); }
inline __m128 _mm_mul_ps(__m128 a, __m128 b) { return vmulq_f32(a, b); }
inline int _mm_movemask_ps(__m128 a) {
    const uint32x4_t signs = vshrq_n_u32(vreinterpretq_u32_f32(a), 31);
    return static_cast<int>(vgetq_lane_u32(signs, 0) | (vgetq_lane_u32(signs, 1) << 1)
                          | (vgetq_lane_u32(signs, 2) << 2) | (vgetq_lane_u32(signs, 3) << 3));
}
#define _MM_SHUFFLE(w, z, y, x) (((w) << 6) | ((z) << 4) | ((y) << 2) | (x))
template <int Mask> inline __m128i rac1_neon_shuffle_epi32(__m128i v) {
    return __m128i{vgetq_lane_u32(v, Mask & 3), vgetq_lane_u32(v, (Mask >> 2) & 3),
                   vgetq_lane_u32(v, (Mask >> 4) & 3), vgetq_lane_u32(v, (Mask >> 6) & 3)};
}
#define _mm_shuffle_epi32(v, mask) rac1_neon_shuffle_epi32<(mask)>((v))
