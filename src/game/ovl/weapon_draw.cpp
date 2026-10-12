// Ported overlay code: draw callbacks of the weapons' mobys (queued by their updates with
// draw_callback_add / sub_001f93e0, called with the moby), drawing textured quads through the quad
// drawer 0x1fd228: the Taunter's rings and the Mine Glove's aim marker.
#include <game/math/vecmath.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/quad_draw.h>
#include <game/ovl/weapons.h>
#include <game/port_stubs.h>

namespace rac1 {

// The Taunter's rings (0x2b7398 keeps them): each live record of 0x1db440 (+0 the size along the
// hand's +0xd0 direction, +4 the life left) draws a quad of the effect texture 8 laid on the basis
// (the direction, its cross with the camera's row 0x13f5e0, ...) at the hand's chain point 0x1614e0
// plus the direction times the size; the corner offsets 0x1db400 scaled by a size lerped from
// 0x1614c8 to 0x1614c4 by the ring's age (life left x 0x1614dc, at most 1), its alpha 0x1614bc
// times the same, over the colour 0x1614c0.
// orig: 0x002b7568
void taunter_rings_draw(uint32_t m) {
    const uint32_t sp = ee_stack_alloc(0x1c0);
    const uint64_t tex0 = effect_tex0(8);
    sd(sp + 0x78, tex0);
    sd(sp + 0x88, 0x8000000048ull);
    sd(sp + 0x80, 0x0000ff9000000260ull);
    sd(sp + 0x70, 0);
    swc1(sp + 0x50, 0.0f);
    swc1(sp + 0x54, 0.0f);
    swc1(sp + 0x58, 0.0f);
    swc1(sp + 0x5c, 1.0f);
    swc1(sp + 0x60, 1.0f);
    swc1(sp + 0x64, 0.0f);
    swc1(sp + 0x68, 1.0f);
    swc1(sp + 0x6c, 1.0f);
    vec3_set_length(V(sp + 0xe0), V(m + 0xd0), -1.0f);
    const uint32_t ring = OV(0x001db440);
    for (uint32_t i = 0; i < 10; ++i) {
        const uint32_t s0 = ring + i * 0xc;
        if (lh(s0 + 8) == 0) continue;
        vec3_scale(V(sp + 0xf0), V(sp + 0xe0), lwc1(s0));
        vec3_add(V(sp + 0xc0), V(OV(0x001614e0)), V(sp + 0xf0));
        V(sp + 0x90) = V(sp + 0xe0);
        vec3_scale(V(sp + 0xb0), V(0x0013f5e0), -1.0f);
        vec3_cross_ba(V(sp + 0xa0), V(sp + 0x90), V(sp + 0xb0));
        vec3_set_length(V(sp + 0xa0), V(sp + 0xa0), 1.0f);
        vec3_cross_ba(V(sp + 0xb0), V(sp + 0xa0), V(sp + 0x90));
        float f20 = int_to_float(lh(s0 + 4)) * lwc1(OV(0x001614dc));
        if (1.0f < f20) f20 = 1.0f;
        const float f1 = lwc1(OV(0x001614c8));
        const float f0 = lwc1(OV(0x001614c4));
        const float f21 = f1 + (f0 - f1) * f20;
        const int32_t alpha = float_to_int(((int_to_float(lw(OV(0x001614bc))) - 0.0f) * f20) + 0.0f);
        const uint32_t col = (lwu(OV(0x001614c0)) & 0x00ffffffu) | (static_cast<uint32_t>(alpha) << 24);
        sw(sp + 0x40, col);
        sw(sp + 0x4c, col);
        sw(sp + 0x48, col);
        sw(sp + 0x44, col);
        for (uint32_t k = 0; k < 4; ++k) {
            vec3_scale(V(sp + 0xd0), V(OV(0x001db400) + k * 0x10), f21);
            mat4_transform(V(sp + k * 0x10), V(sp + 0xd0), M(sp + 0x90));
        }
        quad_draw(sp, 0);
    }
    ee_stack_free(0x1c0);
}

// The Mine Glove's aim marker (registered by 0x2aa2a8 for the mine's predicted impact, pvars +0x10
// its point, +0x20 its normal): two crossed discs (effect textures 0x11 / 0x12) at the point, each
// on the surface's frame (pitch from the normal, yaw by atan2(z, x)) turned about it by the
// frame counter's phase over 180 frames (the second one the other way), 0x80808080; not in game
// mode 2.
// orig: 0x002a9c50
void mine_marker_draw(uint32_t m) {
    const uint32_t sp = ee_stack_alloc(0x1e0);
    if (lw(OV(0x0015f5c4)) != 2) {
        const uint32_t pv = lwu(m + 0x78);
        for (uint32_t s4 = 0; s4 < 2; ++s4) {
            const bool odd = (s4 & 1) != 0;
            const float a = atan2_f(sqrt_f(lwc1(pv + 0x20) * lwc1(pv + 0x20) + lwc1(pv + 0x28) * lwc1(pv + 0x28)), lwc1(pv + 0x24));
            swc1(sp + 0x110, angle_add(-a, u2f(0x3fc90fdbu)));
            swc1(sp + 0x114, atan2_f(lwc1(pv + 0x28), lwc1(pv + 0x20)));
            sw(sp + 0x118, 0);
            sw(sp + 0x11c, 0);
            sw(OV(0x001613e0), 0);
            const int32_t period = frame_scale_i(0xb4);
            const int32_t rem = ee_rem(lw(OV(0x0015f5cc)), period);
            float f20 = int_to_float(rem);
            const float f0 = frame_scale_f(u2f(0x43340000u));
            f20 = f20 * u2f(0x40c90fdbu);
            float ang = angle_wrap(fpu_div(f20, f0));
            if (odd) ang = -ang;
            swc1(OV(0x001613e4), ang);
            sw(OV(0x001613e8), 0);
            sw(OV(0x001613ec), 0);
            euler_to_mat4_vu(M(sp + 0xd0), V(OV(0x001613e0)));
            euler_to_mat4_vu(M(sp + 0x90), V(sp + 0x110));
            const Mat4 spin = M(sp + 0xd0);
            const Mat4 frame = M(sp + 0x90);
            mat4_mul(M(sp + 0x90), frame, spin);
            V(sp + 0xc0) = V(pv + 0x10);
            const int32_t tex = odd ? 0x12 : 0x11;
            sw(sp + 0x40, 0x80808080u);
            sw(sp + 0x44, 0x80808080u);
            sw(sp + 0x48, 0x80808080u);
            sw(sp + 0x4c, 0x80808080u);
            const uint64_t tex0 = effect_tex0(tex);
            sd(sp + 0x88, 0x8000000048ull);
            sd(sp + 0x78, tex0);
            sd(sp + 0x80, 0x0000ff9000000260ull);
            sd(sp + 0x70, 0);
            swc1(sp, -1.0f);
            sw(sp + 4, 0);
            swc1(sp + 8, 1.0f);
            swc1(sp + 0xc, 1.0f);
            swc1(sp + 0x10, 1.0f);
            sw(sp + 0x14, 0);
            swc1(sp + 0x18, 1.0f);
            swc1(sp + 0x1c, 1.0f);
            swc1(sp + 0x20, -1.0f);
            sw(sp + 0x24, 0);
            swc1(sp + 0x28, -1.0f);
            swc1(sp + 0x2c, 1.0f);
            swc1(sp + 0x30, 1.0f);
            sw(sp + 0x34, 0);
            swc1(sp + 0x38, -1.0f);
            swc1(sp + 0x3c, 1.0f);
            sw(sp + 0x50, 0);
            swc1(sp + 0x54, 1.0f);
            swc1(sp + 0x58, 1.0f);
            swc1(sp + 0x5c, 1.0f);
            sw(sp + 0x60, 0);
            sw(sp + 0x64, 0);
            swc1(sp + 0x68, 1.0f);
            sw(sp + 0x6c, 0);
            quad_draw(sp, sp + 0x90);
        }
    }
    ee_stack_free(0x1e0);
}


// One ribbon of the Walloper's trail (0x2bbbc8): the five points at `seg` (the base, then four) drawn as
// quads of width 2 * `w1` on the side of each point (the cross of its view vector and the segment,
// set to length `w1`), the effect texture *0x1610b0 over the colour 0x161080 with the alpha `a1`
// (plain at the first and last corners, so it fades in and out), then a second, wider and brighter
// pass (texture *0x1610b4, colour 0x161084, alpha `a2`): the camera-relative view vectors of each
// pair of points brought to the same length, pushed `w3` along the segment, quads of width 2 * `w2`.
// orig: 0x002bbbc8
void walloper_ribbon(uint32_t seg, int32_t a1, int32_t a2, int32_t n, float w1, float w2, float w3) {
    const uint32_t sp = ee_stack_alloc(0x2b0);
    sw(sp + 0x1c0, static_cast<uint32_t>(n));
    const uint32_t cam = OV(0x00166dc0);
    const uint64_t texa = effect_tex0(lw(OV(0x001616b0)));
    const uint32_t colA = lwu(OV(0x00161680)) | (static_cast<uint32_t>(a1) << 24);
    const uint64_t texb = effect_tex0(lw(OV(0x001616b4)));
    sd(sp + 0x88, 0x8000000048ull);
    sd(sp + 0x80, 0x0000ff9000000260ull);
    sd(sp + 0x78, texa);
    sd(sp + 0x70, 0);
    sw(sp + 0x4c, colA);
    sw(sp + 0x48, colA);
    sw(sp + 0x44, colA);
    sw(sp + 0x40, colA);
    const uint32_t colB = lwu(OV(0x00161684)) | (static_cast<uint32_t>(a2) << 24);
    const uint64_t tex1 = ld(sp + 0x88);
    sd(sp + 0x108, texb);
    sw(sp + 0x1c4, colB);
    const uint64_t clamp = ld(sp + 0x80);
    const uint64_t zero70 = ld(sp + 0x70);
    sd(sp + 0x118, tex1);
    sd(sp + 0x110, clamp);
    sd(sp + 0x100, zero70);
    sw(sp + 0xdc, lwu(sp + 0x1c4));
    sw(sp + 0xd8, lwu(sp + 0x1c4));
    sw(sp + 0xd4, lwu(sp + 0x1c4));
    sw(sp + 0xd0, lwu(sp + 0x1c4));
    swc1(sp + 0xe0, 0.0f);
    swc1(sp + 0xe4, 0.0f);
    swc1(sp + 0xe8, 0.0f);
    swc1(sp + 0xec, 1.0f);
    swc1(sp + 0xf0, 1.0f);
    swc1(sp + 0xf4, 0.0f);
    swc1(sp + 0xf8, 1.0f);
    swc1(sp + 0xfc, 1.0f);
    swc1(sp + 0x50, 0.0f);
    swc1(sp + 0x54, 0.0f);
    swc1(sp + 0x58, 0.0f);
    swc1(sp + 0x5c, 1.0f);
    swc1(sp + 0x60, 1.0f);
    swc1(sp + 0x64, 0.0f);
    swc1(sp + 0x68, 1.0f);
    swc1(sp + 0x6c, 1.0f);
    vec3_sub(V(sp + 0x140), V(seg), V(cam));
    vec3_sub(V(sp + 0x150), V(seg + 0x10), V(seg));
    vec3_cross_ba(V(sp + 0x120), V(sp + 0x150), V(sp + 0x140));
    float f0 = vec3_length(V(sp + 0x120));
    if (f0 != 0.0f) f0 = fpu_div(1.0f, f0);
    vec3_scale(V(sp + 0x120), V(sp + 0x120), w1 * f0);
    int32_t s5 = 1;
    vec3_add(V(sp), V(seg), V(sp + 0x120));
    vec3_sub(V(sp + 0x10), V(seg), V(sp + 0x120));
    const int32_t last = n - 1;
    sw(sp + 0x1d8, static_cast<uint32_t>(last));
    if (s5 < last) {
        uint32_t s0 = seg + 0x10, s1 = seg + 0x20;
        do {
            vec3_sub(V(sp + 0x140), V(s0), V(cam));
            vec3_sub(V(sp + 0x150), V(s1), V(s0));
            vec3_cross_ba(V(sp + 0x120), V(sp + 0x150), V(sp + 0x140));
            float f1 = vec3_length(V(sp + 0x120));
            float f12;
            if (f1 == 0.0f) {
                f12 = w1 * f1;
            } else {
                f1 = fpu_div(1.0f, f1);
                f12 = w1 * f1;
            }
            vec3_scale(V(sp + 0x120), V(sp + 0x120), f12);
            vec3_add(V(sp + 0x20), V(s0), V(sp + 0x120));
            vec3_sub(V(sp + 0x30), V(s0), V(sp + 0x120));
            if (s5 == 1) {
                sw(sp + 0x40, lwu(OV(0x00161680)));
                sw(sp + 0x44, lwu(OV(0x00161680)));
            } else if (s5 == n - 2) {
                sw(sp + 0x48, lwu(OV(0x00161680)));
                sw(sp + 0x4c, lwu(OV(0x00161680)));
            } else if (s5 == 2) {
                sw(sp + 0x44, colA);
                sw(sp + 0x40, colA);
            }
            quad_draw(sp, 0);
            V(sp) = V(sp + 0x20);
            V(sp + 0x10) = V(sp + 0x30);
            ++s5;
            s1 += 0x10;
            s0 += 0x10;
        } while (s5 < last);
    }
    if (last > 0) {
        const float fz = 0.0f;
        uint32_t pk = seg;
        uint32_t pk1 = seg + 0x10;
        s5 = 0;
        do {
            vec3_sub(V(sp + 0x180), V(pk), V(cam));
            vec3_sub(V(sp + 0x190), V(pk1), V(cam));
            const float f20 = vec3_length(V(sp + 0x180));
            const float fl = vec3_length(V(sp + 0x190));
            if (fl < f20) {
                if (fl == fz) V(sp + 0x190) = V(sp + 0x180);
                else vec3_scale(V(sp + 0x190), V(sp + 0x190), fpu_div(f20, fl));
            } else {
                if (f20 == fz) V(sp + 0x180) = V(sp + 0x190);
                else vec3_scale(V(sp + 0x180), V(sp + 0x180), fpu_div(fl, f20));
            }
            vec3_add(V(sp + 0x160), V(cam), V(sp + 0x180));
            vec3_add(V(sp + 0x170), V(cam), V(sp + 0x190));
            vec3_sub(V(sp + 0x150), V(sp + 0x170), V(sp + 0x160));
            vec3_scale(V(sp + 0x1b0), V(sp + 0x150), w3);
            vec3_sub(V(sp + 0x160), V(sp + 0x160), V(sp + 0x1b0));
            vec3_add(V(sp + 0x170), V(sp + 0x170), V(sp + 0x1b0));
            vec3_sub(V(sp + 0x1a0), V(cam), V(sp + 0x160));
            vec3_cross_ba(V(sp + 0x120), V(sp + 0x1a0), V(sp + 0x150));
            float f1 = vec3_length(V(sp + 0x120));
            float f12;
            if (f1 == fz) {
                f12 = w2 * f1;
            } else {
                f1 = fpu_div(1.0f, f1);
                f12 = w2 * f1;
            }
            vec3_scale(V(sp + 0x130), V(sp + 0x120), f12);
            vec3_add(V(sp + 0x90), V(sp + 0x160), V(sp + 0x130));
            vec3_sub(V(sp + 0xa0), V(sp + 0x160), V(sp + 0x130));
            vec3_add(V(sp + 0xb0), V(sp + 0x170), V(sp + 0x130));
            vec3_sub(V(sp + 0xc0), V(sp + 0x170), V(sp + 0x130));
            if (s5 == 0) {
                sw(sp + 0xd0, lwu(OV(0x00161684)));
                sw(sp + 0xd4, lwu(OV(0x00161684)));
            } else if (s5 == n - 2) {
                sw(sp + 0xd8, lwu(OV(0x00161684)));
                sw(sp + 0xdc, lwu(OV(0x00161684)));
            } else if (s5 == 1) {
                sw(sp + 0xd4, lwu(sp + 0x1c4));
                sw(sp + 0xd0, lwu(sp + 0x1c4));
            }
            quad_draw(sp + 0x90, 0);
            ++s5;
            pk += 0x10;
            pk1 += 0x10;
        } while (s5 < last);
    }
    sub_002a0ba8(0x47, 0x5360b);
    ee_stack_free(0x2b0);
}

// The glow at the Walloper's hand (0x2bc1a0): a quad of the effect texture *0x1610c8 and size
// 0x1610b8 at the hand + its +0xd0 vector times 0x1610bc, pushed 0.2 toward the camera, facing it
// (the basis: the view direction, its cross with the camera's row 0x13f5e0 reversed, ...), colour
// 0x1610c0 (0x1610c4 one frame in four) with its alpha times the trail's fade 0x1616d4.
// orig: 0x002bc1a0
void walloper_glow(uint32_t m) {
    const uint32_t sp = ee_stack_alloc(0x170);
    vec3_scale(V(sp + 0xe0), V(m + 0xd0), lwc1(OV(0x001616bc)));
    vec3_add(V(sp + 0xc0), V(sp + 0xe0), V(m + 0x10));
    const uint64_t tex = effect_tex0(lw(OV(0x001616c8)));
    sd(sp + 0x78, tex);
    sd(sp + 0x88, 0x8000000048ull);
    sd(sp + 0x80, 0x0000ff9000000260ull);
    swc1(sp + 0x6c, 1.0f);
    sd(sp + 0x70, 0);
    sw(sp + 0x50, 0);
    sw(sp + 0x54, 0);
    sw(sp + 0x58, 0);
    swc1(sp + 0x5c, 1.0f);
    swc1(sp + 0x60, 1.0f);
    sw(sp + 0x64, 0);
    swc1(sp + 0x68, 1.0f);
    uint32_t s0 = rand_mod(4) != 0 ? lwu(OV(0x001616c0)) : lwu(OV(0x001616c4));
    const float f0 = int_to_float(static_cast<int32_t>(s0) >> 24);
    const int32_t alpha = float_to_int(f0 * lwc1(OV(0x001616d4)));
    s0 = (s0 & 0x00ffffffu) | (static_cast<uint32_t>(alpha) << 24);
    sw(sp + 0x40, s0);
    sw(sp + 0x4c, s0);
    sw(sp + 0x48, s0);
    sw(sp + 0x44, s0);
    vec3_sub(V(sp + 0xf0), V(OV(0x00166dc0)), V(sp + 0xc0));
    vec3_set_length(V(sp + 0x90), V(sp + 0xf0), 1.0f);
    vec3_cross_ba(V(sp + 0xa0), V(sp + 0x90), V(0x0013f5e0));
    vec3_set_length(V(sp + 0xa0), V(sp + 0xa0), -1.0f);
    vec3_cross_ba(V(sp + 0xb0), V(sp + 0xa0), V(sp + 0x90));
    vec3_scale(V(sp + 0xf0), V(sp + 0x90), u2f(0x3e4ccccdu));
    vec3_add(V(sp + 0xc0), V(sp + 0xc0), V(sp + 0xf0));
    for (uint32_t k = 0; k < 4; ++k) {
        vec3_scale(V(sp + 0xd0), V(OV(0x001dbb60) + k * 0x10), lwc1(OV(0x001616b8)));
        mat4_transform(V(sp + k * 0x10), V(sp + 0xd0), M(sp + 0x90));
    }
    quad_draw(sp, 0);
    ee_stack_free(0x170);
}

// The Walloper's trail (the hand's draw callback): every live segment of 0x1dbba0 (lives 0x1dbec0)
// as a ribbon of five points with the alpha 0x1dbed8 times the fade 0x1616d4 (widths 0x161090 /
// 0x161094 / 0.2), then the glow at the hand.
// orig: 0x002bbb00
void walloper_trail_draw(uint32_t m) {
    uint32_t seg = OV(0x001dbba0);
    for (uint32_t i = 0; i < 10; ++i, seg += 0x50) {
        if (lh(OV(0x001dbec0) + i * 2) == 0) continue;
        const float f0 = int_to_float(lh(OV(0x001dbed8) + i * 2));
        const int32_t a = float_to_int(f0 * lwc1(OV(0x001616d4)));
        walloper_ribbon(seg, a, a, 5, lwc1(OV(0x00161690)), lwc1(OV(0x00161694)), u2f(0x3e4ccccdu));
    }
    walloper_glow(m);
}

// orig: (none) — the weapons' draw callbacks for the indirect calls (ovl_call_moby_fn).
void weapon_draw_fns_register() {
    ovl_register_moby_fn(0x002b7568, taunter_rings_draw);
    ovl_register_moby_fn(0x002a9c50, mine_marker_draw);
    ovl_register_moby_fn(0x002bbb00, walloper_trail_draw);
}

}  // namespace rac1
