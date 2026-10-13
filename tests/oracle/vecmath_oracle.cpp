// Differential test: src/game/math/vecmath.cpp vs the original overlay code
// executed by the EE interpreter (ADR 0006). Random inputs, exact comparison
// of every written word and of the return value.

#include <runtime/ee/ps2_fpu.h>
#include "oracle_env.h"

#include <game/math/vecmath.h>

#include <cmath>
#include <cstdio>
#include <functional>
#include <random>

using namespace rac1;
using namespace rac1::oracle;

namespace {

constexpr uint32_t A = TEST_AREA + 0x000;   // input vector / matrix a
constexpr uint32_t B = TEST_AREA + 0x100;   // input vector / matrix b
constexpr uint32_t D = TEST_AREA + 0x200;   // output

std::mt19937 rng(12345);

float rnd(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); }

void fill_floats(uint32_t addr, int n, float lo, float hi) {
    for (int i = 0; i < n; ++i) ee_write<float>(addr + 4 * i, rnd(lo, hi));
}
void fill_random_words(uint32_t addr, int n) {
    for (int i = 0; i < n; ++i) ee_write<uint32_t>(addr + 4 * i, static_cast<uint32_t>(rng()));
}

int g_failures = 0;
void check(const char* name, uint32_t orig, int iters, Ret ret, const std::function<Args()>& gen,
           const std::function<uint64_t(const Args&)>& port, uint32_t ignore_word = 0) {
    if (!check_fn(name, orig, iters, ret, gen, port, Scope::TestArea, ignore_word)) ++g_failures;
}

Vec4& V(uint32_t a) { return ee_ref<Vec4>(a); }
Mat4& M(uint32_t a) { return ee_ref<Mat4>(a); }

Args vec_args(int n_vec_words, float lo, float hi, std::vector<uint32_t> ptrs, std::vector<float> fl = {}) {
    fill_floats(A, n_vec_words, lo, hi);
    fill_floats(B, n_vec_words, lo, hi);
    fill_random_words(D, 16);
    return Args{ptrs, fl};
}

}  // namespace

int main() {
    rac1::ps2fpu::enter_ee_rounding();  // the EE's float rounding (toward zero), as the runtime runs
    if (!setup_veldin()) return SKIP_CODE;
    const int N = 300;

    check("frame_scale_f", 0x001fef10, N, Ret::Float, [] { return Args{{}, {rnd(-1000, 1000)}}; },
          [](const Args& a) { return fbits(frame_scale_f(a.f[0])); });
    check("frame_scale_i", 0x001fef20, N, Ret::Int32, [] { return Args{{static_cast<uint32_t>(static_cast<int32_t>(rng() % 20000) - 10000)}, {}}; },
          [](const Args& a) { return (uint64_t)(uint32_t)frame_scale_i((int32_t)a.i[0]); });
    check("timer_tick_i32", 0x001fef68, N, Ret::Int32, [] { ee_write<int32_t>(A, (int32_t)(rng() % 7) - 2); return Args{{A}, {}}; },
          [](const Args& a) { return (uint64_t)(uint32_t)timer_tick_i32(ee_ref<int32_t>(a.i[0])); });
    check("timer_tick_i16", 0x001fef98, N, Ret::Int32, [] { ee_write<int16_t>(A, (int16_t)((int32_t)(rng() % 7) - 2)); return Args{{A}, {}}; },
          [](const Args& a) { return (uint64_t)(uint32_t)timer_tick_i16(ee_ref<int16_t>(a.i[0])); });
    check("highest_bit_index", 0x001feff8, N, Ret::Int32, [] { return Args{{static_cast<uint32_t>(rng() >> (rng() % 32))}, {}}; },
          [](const Args& a) { return (uint64_t)(uint32_t)highest_bit_index((int32_t)a.i[0]); });
    check("mem_fill32", 0x001ff040, N, Ret::None, [] { fill_random_words(A, 64); return Args{{A + 4 * (rng() % 4), (uint32_t)rng(), 4 * (1 + rng() % 40)}, {}}; },
          [](const Args& a) { mem_fill32(a.i[0], a.i[1], (int32_t)a.i[2]); return 0; });
    check("mem_clear128", 0x001ff068, N, Ret::None, [] { fill_random_words(A, 192); return Args{{A + 16 * (rng() % 4), 16 * (1 + rng() % 30)}, {}}; },
          [](const Args& a) { mem_clear128(a.i[0], (int32_t)a.i[1]); return 0; });
    check("mem_copy", 0x001ff090, N, Ret::None, [] {
              fill_random_words(A, 192);
              const uint32_t al = (rng() % 3 == 0) ? 1 : (rng() % 2 ? 4 : 16);
              return Args{{D + al * (rng() % 4), A + al * (rng() % 4), al * (1 + rng() % 16)}, {}}; },
          [](const Args& a) { mem_copy(a.i[0], a.i[1], (int32_t)a.i[2]); return 0; });
    check("mem_copy128", 0x001ff128, N, Ret::None, [] { fill_random_words(A, 192); return Args{{D, A, 16 * (1 + rng() % 16)}, {}}; },
          [](const Args& a) { mem_copy128(a.i[0], a.i[1], (int32_t)a.i[2]); return 0; });
    check("sqrt_f", 0x001ff1e0, N, Ret::Float, [] { return Args{{}, {rnd(-1e4f, 1e4f)}}; },
          [](const Args& a) { return fbits(sqrt_f(a.f[0])); });
    check("abs_i", 0x001ff200, N, Ret::Int32, [] { return Args{{(uint32_t)rng()}, {}}; },
          [](const Args& a) { return (uint64_t)(uint32_t)abs_i((int32_t)a.i[0]); });
    check("abs_f", 0x001ff218, N, Ret::Float, [] { return Args{{}, {rnd(-1e4f, 1e4f)}}; },
          [](const Args& a) { return fbits(abs_f(a.f[0])); });
    check("max_f", 0x001ff220, N, Ret::Float, [] { return Args{{}, {rnd(-10, 10), rnd(-10, 10)}}; },
          [](const Args& a) { return fbits(max_f(a.f[0], a.f[1])); });
    check("min_f", 0x001ff228, N, Ret::Float, [] { return Args{{}, {rnd(-10, 10), rnd(-10, 10)}}; },
          [](const Args& a) { return fbits(min_f(a.f[0], a.f[1])); });
    check("asin_f", 0x001ff818, N, Ret::Float, [] { return Args{{}, {rnd(-1, 1)}}; },
          [](const Args& a) { return fbits(asin_f(a.f[0])); });
    check("atan2_f", 0x001ff8b0, N, Ret::Float, [] { return Args{{}, {rnd(-10, 10), rnd(-10, 10)}}; },
          [](const Args& a) { return fbits(atan2_f(a.f[0], a.f[1])); });
    check("angle_add", 0x002000e8, N, Ret::Float, [] { return Args{{}, {rnd(-4, 4), rnd(-4, 4)}}; },
          [](const Args& a) { return fbits(angle_add(a.f[0], a.f[1])); });
    check("angle_sub", 0x00200130, N, Ret::Float, [] { return Args{{}, {rnd(-4, 4), rnd(-4, 4)}}; },
          [](const Args& a) { return fbits(angle_sub(a.f[0], a.f[1])); });
    check("angle_dist", 0x002001f0, N, Ret::Float, [] { return Args{{}, {rnd(-4, 4), rnd(-4, 4)}}; },
          [](const Args& a) { return fbits(angle_dist(a.f[0], a.f[1])); });
    check("split_int_frac", 0x00200228, N, Ret::Float, [] { return Args{{D}, {rnd(-1000, 1000)}}; },
          [](const Args& a) { return fbits(split_int_frac(&ee_ref<float>(a.i[0]), a.f[0])); });
    check("int_to_float", 0x00200240, N, Ret::Float, [] { return Args{{(uint32_t)rng()}, {}}; },
          [](const Args& a) { return fbits(int_to_float((int32_t)a.i[0])); });
    check("float_to_int", 0x00200250, N, Ret::Int32, [] { return Args{{}, {rnd(-3e9f, 3e9f)}}; },
          [](const Args& a) { return (uint64_t)(uint32_t)float_to_int(a.f[0]); });

    check("vec_clear", 0x001ff260, N, Ret::None, [] { return vec_args(4, -10, 10, {D}); },
          [](const Args& a) { vec_clear(V(a.i[0])); return 0; });
    check("vec_set_unit_z", 0x001ff268, N, Ret::None, [] { return vec_args(4, -10, 10, {D}); },
          [](const Args& a) { vec_set_unit_z(V(a.i[0])); return 0; });
    check("vec3_add", 0x001ff278, N, Ret::None, [] { return vec_args(4, -100, 100, {D, A, B}); },
          [](const Args& a) { vec3_add(V(a.i[0]), V(a.i[1]), V(a.i[2])); return 0; });
    check("vec3_add_inplace", 0x001ff290, N, Ret::None, [] { return vec_args(4, -100, 100, {D, A, B}); },
          [](const Args& a) { vec3_add_inplace(V(a.i[1]), V(a.i[2])); return 0; });
    check("vec3_sub", 0x001ff2a8, N, Ret::None, [] { return vec_args(4, -100, 100, {D, A, B}); },
          [](const Args& a) { vec3_sub(V(a.i[0]), V(a.i[1]), V(a.i[2])); return 0; });
    check("vec3_scale", 0x001ff300, N, Ret::None, [] { return vec_args(4, -100, 100, {D, A}, {rnd(-5, 5)}); },
          [](const Args& a) { vec3_scale(V(a.i[0]), V(a.i[1]), a.f[0]); return 0; });
    check("vec3_div", 0x001ff378, N, Ret::None, [] { return vec_args(4, -100, 100, {D, A}, {rnd(-5, 5)}); },
          [](const Args& a) { vec3_div(V(a.i[0]), V(a.i[1]), a.f[0]); return 0; });
    check("vec3_reflect", 0x001ff660, N, Ret::None, [] { return vec_args(4, -100, 100, {D, A, B}); },
          [](const Args& a) { vec3_reflect(V(a.i[0]), V(a.i[1]), V(a.i[2])); return 0; });
    check("vec3_dot", 0x001ff398, N, Ret::Float, [] { return vec_args(4, -100, 100, {A, B}); },
          [](const Args& a) { return fbits(vec3_dot(V(a.i[0]), V(a.i[1]))); });
    check("vec3_cross_ba", 0x001ff3c0, N, Ret::None, [] { return vec_args(4, -100, 100, {D, A, B}); },
          [](const Args& a) { vec3_cross_ba(V(a.i[0]), V(a.i[1]), V(a.i[2])); return 0; }, D + 12);
    check("vec3_length", 0x001ff3d8, N, Ret::Float, [] { return vec_args(4, -100, 100, {A}); },
          [](const Args& a) { return fbits(vec3_length(V(a.i[0]))); });
    check("vec2_length", 0x001ff408, N, Ret::Float, [] { return vec_args(4, -100, 100, {A}); },
          [](const Args& a) { return fbits(vec2_length(V(a.i[0]))); });
    check("vec3_distance", 0x001ff450, N, Ret::Float, [] { return vec_args(4, -100, 100, {A, B}); },
          [](const Args& a) { return fbits(vec3_distance(V(a.i[0]), V(a.i[1]))); });
    check("vec2_distance", 0x001ff488, N, Ret::Float, [] { return vec_args(4, -100, 100, {A, B}); },
          [](const Args& a) { return fbits(vec2_distance(V(a.i[0]), V(a.i[1]))); });
    check("vec3_set_length", 0x001ff500, N, Ret::None, [] { auto r = vec_args(4, -100, 100, {D, A}, {rnd(-5, 5)}); if (rng() % 10 == 0) V(A) = Vec4{0, 0, 0, 7}; return r; },
          [](const Args& a) { vec3_set_length(V(a.i[0]), V(a.i[1]), a.f[0]); return 0; });
    check("vec2_set_length", 0x001ff550, N, Ret::None, [] { auto r = vec_args(4, -100, 100, {D, A}, {rnd(-5, 5)}); if (rng() % 10 == 0) V(A) = Vec4{0, 0, 3, 7}; return r; },
          [](const Args& a) { vec2_set_length(V(a.i[0]), V(a.i[1]), a.f[0]); return 0; });
    check("vec2_clamp_length", 0x001ff600, N, Ret::Int32, [] { auto r = vec_args(4, -10, 10, {D, A}, {rnd(0, 12)}); if (rng() % 10 == 0) V(A) = Vec4{0, 0, 3, 7}; return r; },
          [](const Args& a) { return (uint64_t)(uint32_t)vec2_clamp_length(V(a.i[0]), V(a.i[1]), a.f[0]); });
    check("vec4_to_s16x4", 0x001ff768, N, Ret::Int64, [] { return vec_args(4, -2, 2, {A}, {rnd(0, 40000)}); },
          [](const Args& a) { return vec4_to_s16x4(V(a.i[0]), a.f[0]); });
    check("rgba_lerp", 0x00200298, N, Ret::Int64, [] { return Args{{(uint32_t)rng(), (uint32_t)rng()}, {rnd(0, 1)}}; },
          [](const Args& a) { return (uint64_t)rgba_lerp(a.i[0], a.i[1], a.f[0]); });

    check("mat3_transform", 0x001ff6d0, N, Ret::None, [] { return vec_args(16, -10, 10, {D, B, A}); },
          [](const Args& a) { mat3_transform(V(a.i[0]), V(a.i[1]), M(a.i[2])); return 0; });
    check("mat4_transform", 0x001ff6f8, N, Ret::None, [] { return vec_args(16, -10, 10, {D, B, A}); },
          [](const Args& a) { mat4_transform(V(a.i[0]), V(a.i[1]), M(a.i[2])); return 0; });
    check("mat4_identity", 0x001ff9e8, N, Ret::None, [] { return vec_args(16, -10, 10, {D}); },
          [](const Args& a) { mat4_identity(M(a.i[0])); return 0; });
    check("mat3_copy_affine", 0x001ffcb8, N, Ret::None, [] { return vec_args(16, -10, 10, {D, A}); },
          [](const Args& a) { mat3_copy_affine(M(a.i[0]), M(a.i[1])); return 0; });
    check("mat3_copy", 0x001ffcd8, N, Ret::None, [] { return vec_args(16, -10, 10, {D, A}); },
          [](const Args& a) { mat3_copy(M(a.i[0]), M(a.i[1])); return 0; });
    check("mat3_transpose", 0x001ffcf8, N, Ret::None, [] { return vec_args(16, -10, 10, {D, A}); },
          [](const Args& a) { mat3_transpose(M(a.i[0]), M(a.i[1])); return 0; });
    check("mat3_mul", 0x001ffd88, N, Ret::None, [] { return vec_args(16, -10, 10, {D, A, B}); },
          [](const Args& a) { mat3_mul(M(a.i[0]), M(a.i[1]), M(a.i[2])); return 0; });
    check("mat4_mul", 0x001ffdd8, N, Ret::None, [] { return vec_args(16, -10, 10, {D, A, B}); },
          [](const Args& a) { mat4_mul(M(a.i[0]), M(a.i[1]), M(a.i[2])); return 0; });
    check("quat_mul", 0x001ffe68, N, Ret::None, [] { return vec_args(4, -1, 1, {D, A, B}); },
          [](const Args& a) { quat_mul(V(a.i[0]), V(a.i[1]), V(a.i[2])); return 0; });

    check("cos_vu", 0x001ff7e8, N, Ret::Float, [] { return Args{{}, {rnd(-4.7f, 4.7f)}}; },
          [](const Args& a) { return fbits(cos_vu(a.f[0])); });
    check("sin_vu", 0x001ff800, N, Ret::Float, [] { return Args{{}, {rnd(-4.7f, 4.7f)}}; },
          [](const Args& a) { return fbits(sin_vu(a.f[0])); });
    auto euler_gen = [] {
        auto r = vec_args(16, -3.1f, 3.1f, {D, A});
        for (int k = 0; k < 3; ++k) if (rng() % 4 == 0) ee_write<float>(A + 4 * k, (rng() % 2) ? 0.0f : -0.0f);
        return r;
    };
    check("euler_to_mat3_vu", 0x001ffa50, N, Ret::None, euler_gen,
          [](const Args& a) { euler_to_mat3_vu(M(a.i[0]), V(a.i[1])); return 0; });
    check("euler_to_mat4_vu", 0x001ffa70, N, Ret::None, euler_gen,
          [](const Args& a) { euler_to_mat4_vu(M(a.i[0]), V(a.i[1])); return 0; });
    check("euler_to_mat4_fpu", 0x001ffa90, N, Ret::None, euler_gen,
          [](const Args& a) { euler_to_mat4_fpu(M(a.i[0]), V(a.i[1])); return 0; });
    check("quat_from_axis_angle", 0x001fff28, N, Ret::None, [] { return vec_args(4, -1, 1, {D, static_cast<uint32_t>(rng() % 3)}, {rnd(-3.1f, 3.1f)}); },
          [](const Args& a) { quat_from_axis_angle(V(a.i[0]), static_cast<int32_t>(a.i[1]), a.f[0]); return 0; });

    std::printf("%s: %d function(s) differ from the original\n", g_failures ? "FAIL" : "PASS", g_failures);
    return g_failures ? 1 : 0;
}
