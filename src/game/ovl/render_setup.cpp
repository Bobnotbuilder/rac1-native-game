// Ported overlay code: the projection / clip constants of the renderer.
#include <game/debug_hooks.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/ovl_common.h>
#include <game/math/vecmath.h>
#include <game/render/mrp_lod.h>

namespace rac1 {

float (*g_debug_fov_y_base)(float x_tangent) = nullptr;

namespace {
constexpr OvConst<0x0016ca40u> S{};  // the projection block
// orig: (helper) lq / sq
void q(uint32_t dst, uint32_t src) { lq_sq(dst, src); }
}  // namespace

// The screen size from the display (0x1518d0 / 0x1518d2): half sizes and the 4096-centred
// offsets at 0x13e500..; near 32 (0x16cae0), far 745472 (0x16cae4), fov tangent 0.63
// (0x16caf0); the screen centre 0x16cc40 / 0x16cc44 and x 4 at 0x16cc48 / 0x16cc4c; the fog
// range 0x16cc58 / 0x16cc5c / 0x16cc68 / 0x16cc6c.
// orig: 0x001f7848
void screen_setup() {
    const int32_t h = static_cast<int32_t>(static_cast<uint32_t>(lhu(0x001518d2)) << 16);
    const int32_t w = static_cast<int32_t>(static_cast<uint32_t>(lhu(0x001518d0)) << 16);
    const int32_t h2 = h >> 17, w2 = w >> 17;
    constexpr uint32_t B = 0x0013e500;
    sw(B + 0x1c, static_cast<uint32_t>((h2 + 0x800) * 16));
    sw(B, static_cast<uint32_t>(w >> 16));
    sw(B + 4, static_cast<uint32_t>(h >> 16));
    sw(B + 0x10, static_cast<uint32_t>((0x800 - w2) * 16));
    sw(B + 0x14, static_cast<uint32_t>((0x800 - h2) * 16));
    sw(B + 0x18, static_cast<uint32_t>((w2 + 0x800) * 16));
    swc1(S + 0xa0, 32.0f);
    swc1(S + 0xb0, u2f(0x3f2147aeu));
    swc1(S + 0xa4, u2f(0x49360000u));
    sw(B + 8, static_cast<uint32_t>(w2));
    sw(B + 0xc, static_cast<uint32_t>(h2));
    swc1(S + 0x200, int_to_float(w >> 16) * 0.5f);
    const float hy = int_to_float(lh(0x001518d2)) * 0.5f;
    const float cx = lwc1(S + 0x200);
    swc1(S + 0x21c, u2f(0x49000000u));
    swc1(S + 0x228, u2f(0x437f0000u));
    swc1(S + 0x20c, hy * 4.0f);
    swc1(S + 0x208, cx * 4.0f);
    sw(S + 0x22c, 0);
    swc1(S + 0x204, hy);
    sw(S + 0x218, 0);
}

// The projection: aspect (0x16cae8 / 0x16caec), the fov tangents x / y (y = x * 0.775 NTSC /
// 0.756 PAL), the guard-band factors, the fog line (0x16cc50 / 0x16cc54 / 0x16cc60 / 0x16cc64
// and its copies in the render packets 0x1c28xx / 0x1c2axx / 0x1c2bxx), the LOD bands, the clip
// matrices 0x16cb00 / 0x16cb40 / 0x16cb80 and the VU1 constant block 0x16ca40..0x16ca9c.
// orig: 0x001f7980
void projection_setup() {
    const MrpLodSetupScope shadow_patch(false);  // (host: inside a widened shadow run, the game's own projection, mrp_lod.h)
    const float BIG = u2f(0xcafffbe0u);  // -8388080
    const float tx = g_debug_fov_y_base != nullptr ? g_debug_fov_y_base(lwc1(S + 0xb0)) : lwc1(S + 0xb0);  // host widescreen (not in the original)
    swc1(S + 0xb4, tx * (lw(0x0015ed80) != 0 ? u2f(0x3f418937u) : u2f(0x3f466666u)));
    swc1(S + 0xa8, fpu_div(lwc1(S + 0x208), lwc1(S + 0x200)));
    swc1(S + 0xac, fpu_div(lwc1(S + 0x20c), lwc1(S + 0x204)));
    swc1(S + 0xb8, fpu_div(1.0f, cos_vu(atan2_f(1.0f, lwc1(S + 0xb0)))));
    swc1(S + 0xbc, fpu_div(1.0f, cos_vu(atan2_f(1.0f, lwc1(S + 0xb4)))));
    float f20 = fpu_div(1.0f, cos_vu(atan2_f(1.0f, lwc1(S + 0xb0) * lwc1(S + 0xa8))));
    float f0 = fpu_div(1.0f, cos_vu(atan2_f(1.0f, lwc1(S + 0xb4) * lwc1(S + 0xac))));
    const float b8 = lwc1(S + 0xb8), bc = lwc1(S + 0xbc), a8 = lwc1(S + 0xa8), ac = lwc1(S + 0xac);
    f20 = fpu_div(f20, b8);
    const float r8 = fpu_div(1.0f, a8), rc = fpu_div(1.0f, ac);
    f0 = fpu_div(f0, bc);
    swc1(S + 0x1e0, b8);
    swc1(S + 0x1e4, bc);
    swc1(S + 0x1f0, lwc1(S + 0xa4));
    swc1(S + 0x1d0, a8);
    swc1(S + 0x1d8, r8);
    swc1(S + 0x1dc, rc);
    swc1(S + 0x1d4, ac);
    swc1(S + 0x1f4, lwc1(S + 0xa0));
    swc1(S + 0x1e8, fpu_mul(fpu_add(f20, f0), 0.5f));
    q(OV(0x0016cd00), S + 0xb0);
    q(OV(0x0016cd10), S + 0x1d0);
    q(OV(0x0016cd20), S + 0x1e0);
    q(OV(0x0016cd30), S + 0x1f0);
    // the fog line
    const float f2 = lwc1(S + 0x218), f0b = lwc1(S + 0x21c), f6 = lwc1(S + 0x228), f1 = lwc1(S + 0x22c);
    const float k = u2f(0x3a800000u);  // 1/1024
    const float f4 = f0b - f2;
    const float f5a = f1 - f6;
    const uint32_t t1 = lwu(S + 0x22c), t0 = lwu(S + 0x228);
    const float f7 = f4 * k;
    const float f3a = f5a * lwc1(S + 0xa0);
    const float f5 = fpu_div(f5a, f7);
    sw(OV(0x001c2b2c), 0x437effffu);
    const float c0 = f6 * f0b - f1 * f2;
    const float f3 = fpu_div(f3a, f4);
    sw(OV(0x001c2858), t0);
    sw(OV(0x001c2b30), t1);
    const float f0c = fpu_div(c0, f4);
    const float f6b = f6 - (f2 * k) * f5;
    swc1(S + 0x210, f3);
    swc1(OV(0x001cad10), f3);
    swc1(S + 0x214, f0c);
    const uint32_t v0 = lwu(S + 0x210);
    swc1(S + 0x224, f6b);
    swc1(OV(0x001c4050), f3);
    sw(OV(0x001c2b34), t0);
    sw(OV(0x001c2b28), v0);
    sw(OV(0x001c2adc), 0x437effffu);
    swc1(S + 0x220, f5);
    swc1(OV(0x00160bf0), f3);
    sw(OV(0x001c2854), t1);
    sw(OV(0x001c2850), v0);
    sw(OV(0x001c2ae0), t1);
    sw(OV(0x001c2ae4), t0);
    sw(OV(0x001c2ad8), v0);
    lod_bands_set();
    // the clip matrices
    {
        const float a0 = lwc1(S + 0xa0), a4 = lwc1(S + 0xa4), b0 = lwc1(S + 0xb0), b4 = lwc1(S + 0xb4);
        const float f11a = fpu_div(1.0f, a0);
        const float d = a4 - a0;
        const float f1b = (a0 * -2.0f) * a4;
        const float f8a = a4 + a0;
        const float f3b = a0 * d;
        const float f7b = b0 * 210000.0f;
        const float c200 = lwc1(S + 0x200), c204 = lwc1(S + 0x204), c210 = lwc1(S + 0x210);
        const float f6c = b4 * a0;
        const float f5c = b0 * a0;
        const float f1c = fpu_div(f1b, f3b);
        swc1(OV(0x001607e0), u2f(0x361fc873u));
        const float f8b = fpu_div(f8a, f3b);
        swc1(OV(0x001607e4), u2f(0xb61fc873u));
        const float f5d = fpu_div(c200, f5c);
        swc1(OV(0x001607e8), 0.5f);
        const float f10 = fpu_div(c204, f6c);
        sw(S + 0xc4, 0);
        const float f9 = fpu_div(c200, f7b);
        sw(S + 0xc8, 0);
        const float f11 = f11a * c210;
        sw(S + 0xcc, 0);
        swc1(S + 0xc0, f5d);
        swc1(S + 0xd4, f10);
        swc1(OV(0x001607ec), f9);
        swc1(S + 0xe8, fpu_mul(f8b, BIG));
        swc1(S + 0xec, f11);
        swc1(S + 0xf8, fpu_mul(f1c, BIG));
        for (uint32_t o : {0xd0u, 0xd8u, 0xdcu, 0xe0u, 0xe4u, 0xf0u, 0xf4u, 0xfcu}) sw(S + o, 0);
        mem_copy(S + 0x100, S + 0xc0, 0x40);
        swc1(S + 0x12c, fpu_div(1.0f, lwc1(S + 0xa0)));
        mem_copy(S + 0x140, S + 0x100, 0x40);
    }
    {
        const float f10 = lwc1(S + 0x208), f9 = lwc1(S + 0x20c), f11 = lwc1(S + 0x210);
        const float f17 = fpu_div(1.0f, BIG);
        const float f16 = fpu_div(1.0f, f10);
        const float f0d = fpu_div(lwc1(S + 0x140), f10);
        const float f14 = fpu_div(1.0f, f9);
        const float f15 = fpu_div(1.0f, f11);
        const float f1d = fpu_div(lwc1(S + 0x154), f9);
        const float f2d = fpu_div(lwc1(S + 0x168), BIG);
        const float f3d = fpu_div(lwc1(S + 0x178), BIG);
        const float f6d = fpu_div(f10, lwc1(S + 0xc0));
        const float f5d = fpu_div(f9, lwc1(S + 0xd4));
        const float f4d = fpu_div(BIG, lwc1(S + 0xe8));
        const float f7d = fpu_div(f10, lwc1(S + 0x200));
        swc1(S + 0x140, f0d);
        const float f8d = fpu_div(f9, lwc1(S + 0x204));
        const float f13 = lwc1(S + 0x214);
        swc1(S + 0x154, f1d);
        swc1(S + 0x168, f2d);
        swc1(S + 0x178, f3d);
        swc1(S + 0x180, f16);
        swc1(S + 0x184, f14);
        swc1(S + 0x188, f17);
        swc1(S + 0x18c, f15);
        swc1(S + 0x1b0, f6d);
        swc1(S + 0x1b4, f5d);
        swc1(S + 0x1b8, f4d);
        swc1(S + 0x1bc, f11);
        swc1(S + 0x1a4, 2048.0f);
        sw(S + 0x1a8, 0x4afffc20u);
        swc1(S + 0x1ac, f13);
        swc1(S + 0x1c0, f7d);
        swc1(S + 0x190, f10);
        swc1(S + 0x194, f9);
        swc1(S + 0x198, BIG);
        swc1(S + 0x19c, f11);
        swc1(S + 0x1a0, 2048.0f);
        swc1(S + 0x1c4, f8d);
    }
    // the VU1 constant block
    sw(S, lwu(S + 0x22c));
    swc1(S + 0x1cc, 1.0f);
    sw(S + 4, 0x103e4000u);
    sw(S + 8, 0xe);
    swc1(S + 0x1c8, 1.0f);
    sw(S + 0xc, 0);
    sd(S + 0x10, 0);
    sd(S + 0x18, 0);
    const uint32_t t1b = lwu(S + 0x210);
    sw(S + 0x64, 0x303e4000u);
    sw(S + 0x6c, 0x303ec000u);
    sw(S + 0x74, 0x302e4000u);
    sw(S + 0x78, 0x412);
    sw(S + 0x7c, 0x302ec000u);
    sw(S + 0x84, 0x20364000u);
    sw(S + 0x90, t1b);
    sw(S + 0x94, 0x20264000u);
    sw(S + 0x98, 0x41);
    sw(S + 0x60, t1b);
    sw(S + 0x68, 0x412);
    sw(S + 0x70, t1b);
    sw(S + 0x80, t1b);
    sw(S + 0x88, 0x41);
    sw(S + 0x8c, 0);
    sw(S + 0x9c, 0);
    q(S + 0x10, S + 0x60);
    q(S + 0x20, S + 0x1c0);
    q(S + 0x30, S + 0x180);
    q(S + 0x40, S + 0x190);
    q(S + 0x50, S + 0x1a0);
    q(OV(0x001c2810), S + 0x1a0);
    q(OV(0x001c2820), S + 0x190);
    q(OV(0x001c2830), S + 0x1c0);
    q(OV(0x001c28a0), S + 0x1a0);
    q(OV(0x001c28b0), S + 0x180);
    q(OV(0x001c28c0), S + 0x190);
    sw(OV(0x001c28e4), lwu(S + 0x1c4));
    sw(OV(0x001c28e0), lwu(S + 0x1c0));
    q(OV(0x001c2b40), S + 0x190);
    q(OV(0x001c2b50), S + 0x1a0);
}

}  // namespace rac1
