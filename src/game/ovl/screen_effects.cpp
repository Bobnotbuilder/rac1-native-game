// Ported overlay code: the screen-space effects (see screen_effects.h).
#include <game/ovl/screen_effects.h>

#include <game/ovl/ovl_reloc.h>
#include <game/math/vecmath.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>

namespace rac1 {

namespace {
// orig: (helper) a 32-bit result sign-extended into a 64-bit register
uint64_t sx64(int32_t v) { return static_cast<uint64_t>(static_cast<int64_t>(v)); }
}  // namespace

// orig: 0x001f6c38
void screen_project(uint32_t out, uint32_t in) {
    Mat4 t;
    mat4_identity(t);
    t.r[3].x = fpu_mul(-lwc1(OV(0x00166dc0)), 1024.0f);
    t.r[3].y = fpu_mul(-lwc1(OV(0x00166dc4)), 1024.0f);
    t.r[3].z = fpu_mul(-lwc1(OV(0x00166dc8)), 1024.0f);
    Mat4 m;
    mat4_mul(m, M(OV(0x00166cc0)), t);
    Vec4 p;
    vec3_scale(p, V(in), 1024.0f);
    p.w = 1.0f;
    Vec4 q;
    mat4_transform(q, p, m);
    const float k = fpu_div(lwc1(OV(0x0016cc50)), q.w);
    swc1(out + 8, fpu_mul(q.z, u2f(0x3a800000u)));
    const float y = fpu_add(fpu_mul(q.y, k), 2048.0f), x = fpu_add(fpu_mul(q.x, k), 2048.0f);
    swc1(out + 4, fpu_mul(y, 16.0f));
    swc1(out, fpu_mul(x, 16.0f));
}

// orig: 0x001fa8f0
void screen_sprite(int32_t u, int32_t v, uint64_t tex0, uint32_t z, uint32_t rgba, uint32_t flip_u, uint32_t flip_v, float x, float y, float sx,
                   float sy, float angle, float px, float py) {
    const uint32_t u0 = (flip_u & 0xff) == 0 ? 0x10u : static_cast<uint32_t>(u) << 4;
    const uint32_t u1 = (flip_u & 0xff) == 0 ? static_cast<uint32_t>(u) << 4 : 0x10u;
    const uint32_t v0 = (flip_v & 0xff) == 0 ? 0x100000u : static_cast<uint32_t>(v) << 20;
    const uint32_t v1 = (flip_v & 0xff) == 0 ? static_cast<uint32_t>(v) << 20 : 0x100000u;
    const uint64_t zz = static_cast<uint64_t>(z) << 32;
    const Vec4 pos = Vec4{x, y, 0.0f, 0.0f};
    const float sn = sin_vu(angle), cs = cos_vu(angle);
    const Vec4 ay = Vec4{fpu_mul(sy, sn), fpu_mul(sy, cs), 0.0f, 0.0f};
    const Vec4 ax = Vec4{fpu_mul(sx, cs), fpu_mul(-sx, sin_vu(angle)), 0.0f, 0.0f};
    const float qy = fpu_sub(1.0f, py), qx = fpu_sub(1.0f, px);
    Vec4 t, c[4];
    vec3_scale(t, ay, qy);
    vec3_add(c[0], pos, t);
    vec3_scale(t, ax, qx);
    vec3_sub(c[0], c[0], t);
    vec3_scale(t, ay, qy);
    vec3_add(c[1], pos, t);
    vec3_scale(t, ax, px);
    vec3_add(c[1], c[1], t);
    vec3_scale(t, ay, py);
    vec3_sub(c[2], pos, t);
    vec3_scale(t, ax, qx);
    vec3_sub(c[2], c[2], t);
    vec3_scale(t, ay, py);
    vec3_sub(c[3], pos, t);
    vec3_scale(t, ax, px);
    vec3_add(c[3], c[3], t);
    const uint32_t r = lwu(OV(0x001611c0));
    sw(r, 0x10000007);
    sw(r + 4, 0);
    sw(r + 8, 0);
    sw(r + 0xc, 0x50000007);
    const uint32_t d = r + 0x10;
    sw(OV(0x001611c0), d);
    sd(d, 0xb400000000008001ull);
    sd(d + 8, (0xa6a6a6a6ull << 11) | 0x106);
    sd(d + 0x18, 0x154);
    sd(d + 0x10, tex0);
    sd(d + 0x28, u0 | v0);
    sd(d + 0x20, sx64(static_cast<int32_t>(rgba)));
    const uint32_t uv[3] = {u1 | v0, u0 | v1, u1 | v1};
    for (uint32_t k = 0; k < 4; ++k) {
        const int32_t px16 = float_to_int(fpu_mul(c[k].x, 16.0f));
        const int32_t py16 = float_to_int(fpu_mul(c[k].y, 16.0f));
        const int32_t yy = py16 + lw(0x0013e514) - 8, xx = px16 + lw(0x0013e510) - 8;
        if (k < 3) sd(d + 0x38 + 0x10 * k, uv[k]);
        else sd(d + 0x68, 0);
        sd(d + 0x30 + 0x10 * k, sx64(xx) | (sx64(yy) << 16) | zz);
    }
    sw(OV(0x001611c0), lwu(OV(0x001611c0)) + 0x70);
}

// orig: 0x001edc38
void screen_effect_draw(uint32_t e, float x, float y) {
    const uint64_t tex0 = effect_tex0(lw(e + 0x18));
    float ang = lwc1(e + 0x1c);
    const int32_t mode = lw(e + 0x2c);
    const float k40 = 40.0f;
    const uint32_t z = 0x00fffff3u;
    auto size = [e, k40] { return fpu_mul(k40, lwc1(e + 0x10)); };
    if (mode == 0) {  // a series of sprites, turned by +0x28 each
        for (int32_t i = 0; i < lh(e + 0x26); ++i) {
            const float s = size();
            screen_sprite(0x3f, 0x3f, tex0, z, lwu(e + 0x14), 0, 0, x, y, s, s, ang, 0.0f, 0.0f);
            ang = angle_add(ang, lwc1(e + 0x28));
        }
    } else if (mode == 1) {  // four quadrants, mirrored
        const float a0 = fpu_mul(fpu_mul(sin_vu(ang), k40), lwc1(e + 0x10));
        const float a1 = fpu_mul(fpu_mul(cos_vu(ang), k40), lwc1(e + 0x10));
        const float b0 = fpu_mul(fpu_mul(cos_vu(ang), k40), lwc1(e + 0x10));
        const float sn = sin_vu(ang);
        const float s0 = lwc1(e + 0x10);
        const float b1 = fpu_mul(fpu_mul(sn, -k40), s0);
        float s = fpu_mul(s0, k40);
        screen_sprite(0x3f, 0x3f, tex0, z, lwu(e + 0x14), 0, 0, x, y, s, s, ang, 0.0f, 0.0f);
        s = fpu_mul(lwc1(e + 0x10), k40);
        screen_sprite(0x3f, 0x3f, tex0, z, lwu(e + 0x14), 1, 0, fpu_add(x, b0), fpu_add(y, b1), s, s, ang, 0.0f, 0.0f);
        s = fpu_mul(lwc1(e + 0x10), k40);
        screen_sprite(0x3f, 0x3f, tex0, z, lwu(e + 0x14), 0, 1, fpu_sub(x, a0), fpu_sub(y, a1), s, s, ang, 0.0f, 0.0f);
        s = fpu_mul(lwc1(e + 0x10), k40);
        screen_sprite(0x3f, 0x3f, tex0, z, lwu(e + 0x14), 1, 1, fpu_sub(fpu_add(x, b0), a0), fpu_sub(fpu_add(y, b1), a1), s, s, ang, 0.0f, 0.0f);
    } else if (mode == 2) {  // one, centred
        const float s = size();
        screen_sprite(0x3f, 0x3f, tex0, z, lwu(e + 0x14), 0, 0, x, y, s, s, ang, 0.5f, 0.5f);
    }
}

// orig: 0x001ee040
void screen_effects_draw() {
    constexpr OvConst<0x00169040u> L{};
    if (lw(0x001413d4) == 0x72) sw(L + 0xc0, 0);
    if (lw(L + 0xc0) == 0) return;
    const float cx = int_to_float(lw(0x0013e508)), cy = int_to_float(lw(0x0013e50c));
    const uint32_t sp = ee_stack_alloc(0x20);
    for (int32_t i = 0; i < lw(L + 0xc0); ++i) {
        const uint32_t e = L + static_cast<uint32_t>(i) * 0x30;
        const uint32_t m = lwu(e + 0x20);
        if (m == 0) continue;
        const uint32_t st = lbu(m + 0x20);
        if (st == 0xfe || st == 0xfd) continue;
        float x = cx, y = cy;
        if (lh(e + 0x24) != 0) {
            screen_project(sp, e);
            x = fpu_mul(fpu_sub(lwc1(sp), int_to_float(lw(0x0013e510))), 0.0625f);
            y = fpu_mul(fpu_sub(lwc1(sp + 4), int_to_float(lw(0x0013e514))), 0.0625f);
        }
        screen_effect_draw(e, x, y);
    }
    ee_stack_free(0x20);
    sw(L + 0xc0, 0);
}

// orig: 0x001ed880
void lens_flare_draw() {
    constexpr OvConst<0x00167040u> S{};
    if (lw(S + 4) != 1 || (lbu(lwu(S) + 0x20) & 0x80) != 0) {
        sw(S + 4, 0);
        return;
    }
    if (lwc1(S + 8) <= 0.0f) return;
    const uint32_t mob = lwu(S);
    const float dist = vec3_distance(V(OV(0x00166dc0)), V(mob + 0x10));
    const float cx = int_to_float(lw(0x0013e508)), cy = int_to_float(lw(0x0013e50c));
    const uint32_t sp = ee_stack_alloc(0x20);
    screen_project(sp, mob + 0x10);
    const float fx = fpu_mul(fpu_sub(lwc1(sp), int_to_float(lw(0x0013e510))), 0.0625f);
    const float fy = fpu_mul(fpu_sub(lwc1(sp + 4), int_to_float(lw(0x0013e514))), 0.0625f);
    ee_stack_free(0x20);
    const float dx = fpu_sub(cx, fx), dy = fpu_sub(cy, fy);
    uint32_t cofs = 0;
    for (uint32_t i = 0; i < 16; ++i) {
        const uint64_t tex0 = effect_tex0(lh(S + 0x30 + 2 * i) + 6);
        const float k = lwc1(S + 0x90 + 4 * i);
        const int32_t tw = i != 0 ? 0x20 : 0x40;
        const float ox = fpu_mul(dx, k), oy = fpu_mul(dy, k);
        float far = fpu_sub(dist, 5.0f);
        const float ex = fpu_add(cx, ox), ey = fpu_add(cy, oy);
        if (1.0f < far) far = 1.0f;
        else if (far < 0.0f) far = 0.0f;
        float fh = fpu_add(fpu_mul(fpu_div(abs_f(dx), cx), -5.0f), 5.5f);
        if (1.0f < fh) fh = 1.0f;
        else if (fh < 0.0f) fh = 0.0f;
        float fv = fpu_add(fpu_mul(fpu_div(abs_f(dy), cy), -5.0f), 5.5f);
        if (1.0f < fv) fv = 1.0f;
        else if (fv < 0.0f) fv = 0.0f;
        const float a = fpu_mul(far, fpu_mul(fh, fv));
        const int32_t alpha = float_to_int(fpu_mul(a, static_cast<float>(lh(S + 0x10 + 2 * i))));
        const uint32_t col = (static_cast<uint32_t>(alpha) << 24) | (lwu(S + cofs + 0x50) & 0xffffffu);
        cofs += 4;
        const int32_t size = float_to_int(fpu_mul(static_cast<float>(tw), lwc1(S + 0xd0 + 4 * i)));
        const float half = static_cast<float>(size >> 1);
        const int32_t x = float_to_int(fpu_sub(ex, half)), y = float_to_int(fpu_sub(ey, half));
        glyph_sprite(x, y, size, size, 0, 0, tw, tw, sx64(static_cast<int32_t>(col)), tex0);
    }
    sw(S + 8, 0);
}

}  // namespace rac1
