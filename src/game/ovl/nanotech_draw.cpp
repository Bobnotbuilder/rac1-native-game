// The nanotech crate's orbs' draw callback 0x2d9810 (queued by the orbs' update 0x2d89f0 through
// 0x1f93e0, called with the moby): a glow billboard at the crate's centre facing the camera and
// the cluster's mesh (a 290-point table through the billboard matrix, three strip lists drawn
// as GIF packets by 0x1fde98), then, while the crate is in state 1, four quads of the
// bounds' texture coordinates wrapped by 0x200260. Header nanotech_draw.h.
#include <game/ovl/nanotech_draw.h>

#include <game/math/vecmath.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/quad_draw.h>
#include <game/ovl/shadows.h>
#include <game/ovl/vu0_macro.h>
#include <game/render/mrp_fx.h>

namespace rac1 {

namespace {
// orig: (helper) a $gp-relative word of the level's globals (gp = 0x166c00)
template <uint32_t K>
inline uint32_t G() { return OV(0x00166c00u - K); }
// orig: (helper) the 0x10-aligned padding of 0x1fde98's lists: zero words up to the next quadword
inline uint32_t pad_zero(uint32_t a) {
    while ((a & 0xfu) != 0) {
        sw(a, 0);
        a += 4;
    }
    return a;
}
}  // namespace

// The remainder of a / b about whole multiples of b (VU0: Q = a / b, its fraction, times b): a - trunc(a / b) * b by
// the VU float rules. Only the x lanes of the registers it uses are modelled.
// orig: 0x00200260
float float_fmod_vu(float a, float b) {
    const float q = vu0m::div(a, b);
    const float x = vu0m::add(0.0f, q);                                                          // VADDq.x vf1, vf0, Q (vf0.x = 0)
    const float whole = static_cast<float>(static_cast<int32_t>(vu0m::ftoi(x, 1.0f)));            // VFTOI0.x, VITOF0.x
    return vu0m::mul(vu0m::sub(x, whole), b);                                                    // VSUB.x, VMUL.x
}

// `n` vertices as one packet at the cursor *(0x1611c0) (the cursor's value kept at its start for the tag's QWC): the
// five quadwords of the template 0x1c4020 (the GIF tag's NLOOP |= n), the ST unpack (0x160ec0, 8 bytes a vertex from
// `st`), the colour unpack (0x160ee0, 4 bytes from `rgba`), the position unpack (0x160f00, 12 bytes from `xyz`), each
// list padded to a quadword and its UNPACK's count |= n << 16, then MSCALF 0xc (| (clip & 1) << 1) and FLUSH.
// orig: 0x001fde98
void prim_list_packet(int32_t n, uint32_t xyz, uint32_t rgba, uint32_t st, uint32_t clip) {
    const MrpFxScope fx(kMrpFxPrimList, xyz, static_cast<uint32_t>(n));  // (host: drawn by the MRP, mrp_fx.h)
    const uint32_t t0 = ((clip & 1u) << 1) + 0xc;
    const uint32_t t2 = lwu(OV(0x001611c0));
    const uint32_t cnt = static_cast<uint32_t>(n);
    const uint32_t t7 = cnt << 16;
    uint32_t at = t2;
    for (uint32_t k = 0; k < 5; ++k) lq_sq(at + 0x10 * k, OV(0x001c4020) + 0x10 * k);
    at += 0x50;
    sw(at - 0x40, lwu(at - 0x40) | cnt);
    lq_sq(at, OV(0x00160ec0));
    at += 0x10;
    sw(at - 4, lwu(at - 4) | t7);
    for (int32_t v = n; v != 0; --v) {
        sd(at, ld(st));
        st += 8;
        at += 8;
    }
    at = pad_zero(at);
    lq_sq(at, OV(0x00160ee0));
    at += 0x10;
    sw(at - 4, lwu(at - 4) | t7);
    for (int32_t v = n; v != 0; --v) {
        sw(at, lwu(rgba));
        rgba += 4;
        at += 4;
    }
    at = pad_zero(at);
    lq_sq(at, OV(0x00160f00));
    at += 0x10;
    sw(at - 4, lwu(at - 4) | t7);
    for (int32_t v = n; v != 0; --v) {
        sw(at, lwu(xyz));
        sw(at + 4, lwu(xyz + 4));
        sw(at + 8, lwu(xyz + 8));
        xyz += 12;
        at += 12;
    }
    at = pad_zero(at);
    sw(at, 0x15000000u | t0);
    sw(at + 4, 0x11000000u);
    sd(at + 8, 0);
    at += 0x10;
    sw(t2, lwu(t2) | (((at - t2) >> 4) - 1));
    sw(OV(0x001611c0), at);
}

namespace {
// orig: 0x002d8128
bool moby_state_is_1(uint32_t m) { return lbu(m + 0x20) == 1; }
}  // namespace

// The callback of a nanotech crate's orbs `m` (pvars +0: the glow's height and size scale, +0xc the crate moby whose
// matrix 0xc0.. turns the glow): the quad's GS state (CLAMP / TEX0 effect texture *(gp-0x512c) / TEX1 / ALPHA from the
// words gp-0x513c.., colour gp-0x5128), the billboard matrix (rows: towards the camera, its cross with the crate's z
// row (normalised), their cross; the point: the crate's centre raised by 0.5 + the height), the 290 table points
// 0x1e14c0 through it into 0x1e3da0, three lists drawn through 0x1fde98 (148, 109 and 36 vertices), the effect
// texture 0xb's quad with a colour from the scale (*64 + 128) and 0x161b10, 32 quads of the mesh table (0x1e3a80
// indices into 0x1e3480, the two last vertices scaled by gp-0x5114), each through the matrix; then in state 1
// (0x2d8128) the matrix copied from the crate and 4 quads of 0x1e3b80 vertices (0x1e3c80 indices) whose texture
// coordinates are the view-side direction x 0.5 + 0.5 wrapped to 8 (0x200260), colour 0x107f7f7f, drawn flat.
// orig: 0x002d9810
void nanotech_orbs_draw(uint32_t m) {
    const uint32_t pv = lwu(m + 0x78);
    const float scale = fpu_div(lwc1(pv), lwc1(G<0x51f0>()));  // f22
    const uint32_t f = ee_stack_alloc(0x110);                   // the quad +0x00..+0x90, the matrix +0x90, temporaries
    const uint32_t mat = f + 0x90;
    const uint64_t tex = effect_tex0(lw(G<0x512c>()));
    uint64_t alpha = static_cast<uint64_t>(static_cast<int64_t>(lw(G<0x5140>())));
    alpha |= static_cast<uint64_t>(static_cast<int64_t>(lw(G<0x513c>())) << 2);
    alpha |= static_cast<uint64_t>(static_cast<int64_t>(lw(G<0x5138>())) << 4);
    alpha |= static_cast<uint64_t>(static_cast<int64_t>(lw(G<0x5134>())) << 6);
    alpha |= static_cast<uint64_t>(static_cast<int64_t>(lw(G<0x5130>())) << 32);
    sd(f + 0x78, tex);
    sd(f + 0x88, alpha);
    sd(f + 0x80, 0x0000ff9000000260ull);
    sd(f + 0x70, 0);
    sub_002a0ba8(0x47, 0x53001);
    const float cz = lwc1(m + 0x18) + 0.5f;
    swc1(f + 0xc0, lwc1(m + 0x10));
    swc1(f + 0xc4, lwc1(m + 0x14));
    swc1(f + 0xc8, cz);
    swc1(f + 0xcc, 1.0f);
    swc1(f + 0xc8, cz + lwc1(pv));
    vec3_add(V(f + 0xc0), V(f + 0xc0), V(OV(0x00161af0)));
    vec3_sub(V(f + 0xe0), V(OV(0x00166dc0)), V(f + 0xc0));
    vec3_set_length(V(mat), V(f + 0xe0), 1.0f);
    vec3_cross_ba(V(f + 0xa0), V(mat), V(lwu(pv + 0xc) + 0xe0));
    vec3_set_length(V(f + 0xa0), V(f + 0xa0), 1.0f);
    vec3_cross_ba(V(f + 0xb0), V(f + 0xa0), V(mat));
    const uint32_t col = lwu(G<0x5128>());
    sw(f + 0x40, col);
    sw(f + 0x4c, col);
    sw(f + 0x48, col);
    sw(f + 0x44, col);
    sub_002a0ba8(6, effect_tex0(lw(G<0x512c>())));
    sub_002a0ba8(0x42, 0x8000000044ull);
    sub_002a0ba8(8, 0);
    sub_002a0ba8(0x14, 0x0000ff9000000260ull);
    sub_002a0ba8(0x4a, 0);
    shadows_vu1_setup();
    for (uint32_t i = 0; i < 0x122; ++i) {
        mat4_transform(V(f + 0xf0), V(OV(0x001e14c0) + i * 0x10), M(mat));
        const uint32_t o = OV(0x001e3da0) + i * 0xc;
        swc1(o, lwc1(f + 0xf0));
        swc1(o + 4, lwc1(f + 0xf4));
        swc1(o + 8, lwc1(f + 0xf8));
    }
    prim_list_packet(0x94, OV(0x001e3da0), OV(0x001e2ff0), OV(0x001e26e0), 1);
    prim_list_packet(0x6d, OV(0x001e3da0) + 0x6d8, OV(0x001e2ff0) + 0x248, OV(0x001e26e0) + 0x490, 1);
    prim_list_packet(0x24, OV(0x001e3da0) + 0xbe8, OV(0x001e2ff0) + 0x3f8, OV(0x001e26e0) + 0x7f0, 1);
    const uint64_t tex2 = effect_tex0(0xb);
    sd(f + 0x88, 0x8000000048ull);
    sd(f + 0x78, tex2);
    const int32_t c = float_to_int(scale * 64.0f + 128.0f);
    const uint32_t v0 = (static_cast<uint32_t>(c) + lwu(OV(0x00161b10))) << 24;
    sw(f + 0x4c, 0);
    sw(f + 0x48, 0);
    sw(f + 0x40, v0 | lwu(G<0x5120>()));
    sw(f + 0x44, v0 | lwu(G<0x5120>()));
    for (uint32_t i = 0; i < 0x20; ++i) {
        const uint32_t idx = OV(0x001e3a80) + i * 8;
        for (uint32_t j = 0; j < 4; ++j) {
            const uint32_t v = OV(0x001e3480) + (static_cast<uint32_t>(static_cast<int32_t>(lh(idx + j * 2))) << 4);
            if (j < 2) vec3_scale(V(f + j * 0x10), V(v), lwc1(G<0x5124>()));
            else vec3_scale(V(f + j * 0x10), V(v), lwc1(G<0x5124>()) * lwc1(G<0x5114>()));
        }
        swc1(f + 0x50, 0.5f);
        swc1(f + 0x54, 0.5f);
        swc1(f + 0x58, 0.5f);
        swc1(f + 0x5c, 0.5f);
        swc1(f + 0x60, 0.5f);
        swc1(f + 0x64, 1.0f);
        swc1(f + 0x68, 0.5f);
        swc1(f + 0x6c, 1.0f);
        quad_draw(f, mat);
    }
    if (moby_state_is_1(m)) {
        const uint64_t tex3 = effect_tex0(lw(G<0x511c>()));
        sd(f + 0x78, tex3);
        sw(f + 0x40, 0x107f7f7fu);
        sw(f + 0x4c, 0x107f7f7fu);
        sw(f + 0x48, 0x107f7f7fu);
        sw(f + 0x44, 0x107f7f7fu);
        sd(f + 0x88, 0x8000000044ull);
        const uint32_t cr = lwu(pv + 0xc);
        lq_sq(mat, cr + 0xc0);
        lq_sq(mat + 0x10, cr + 0xd0);
        lq_sq(mat + 0x20, cr + 0xe0);
        swc1(f + 0xc8, lwc1(m + 0x18));
        for (uint32_t i = 0; i < 4; ++i) {
            const uint32_t idx = OV(0x001e3c80) + i * 8;
            uint32_t s1 = f + 0x50, s2 = f + 0x54;
            for (uint32_t k = 0; k < 4; ++k) {
                const uint32_t v = OV(0x001e3b80) + (static_cast<uint32_t>(static_cast<int32_t>(lh(idx + k * 2))) << 4);
                mat4_transform(V(f + k * 0x10), V(v), M(mat));
                vec3_sub(V(f + 0xd0), V(f + k * 0x10), V(OV(0x00166dc0)));
                swc1(f + 0xd4, lwc1(f + 0xd8));
                vec2_set_length(V(f + 0xd0), V(f + 0xd0), lwc1(G<0x5118>()));
                float s = lwc1(f + 0xd0) * 0.5f + 0.5f;
                swc1(s1, s);
                swc1(s1, float_fmod_vu(s, 8.0f));
                s1 += 8;
                s = lwc1(f + 0xd4) * 0.5f + 0.5f;
                swc1(s2, s);
                swc1(s2, float_fmod_vu(s, 8.0f));
                s2 += 8;
            }
            quad_draw(f, 0);
        }
    }
    sub_002a0ba8(0x47, 0x5360b);
    ee_stack_free(0x110);
}

// orig: (none) — the callback for the indirect calls (the draw lists, 0x1f93e0)
void nanotech_draw_fns_register() { ovl_register_moby_fn(0x002d9810, nanotech_orbs_draw); }

}  // namespace rac1
