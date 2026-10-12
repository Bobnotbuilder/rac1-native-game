// Ported overlay code: the oxygen indicator's HUD element (controller slot 4, key 0x753f, requested
// by hero_water_air 0x00227778 while the hero is under water). Its start (+0x10), update (+0x14)
// and draw (+0x18) functions. The element keeps the shown air in +0x74 (hud_plain_update eases it
// toward *(+0xc) = P+0x22a0, of +8 = 10000), the bubbles in a table of 100 records of 16 bytes at
// 0x17e660: x, y, and a velocity x, y. The globals 0x15f750.. are the HUD data's layout numbers
// (three bubble tiers, a count, the frame's size and two ratios) read from the level's data.
#include <game/math/vecmath.h>
#include <game/ovl/hud_o2.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/pause_menu.h>
#include <game/ovl/suck_cannon.h>
#include <game/port_stubs.h>

namespace rac1 {

namespace {
constexpr OvConst<0x0017e660u> BUBBLES{};  // 100 x (x, y, vx, vy)
// the layout numbers (the first three floats are the tiers' bounds, then a count and a size)
constexpr OvConst<0x0015f750u> TIER_A{};
constexpr OvConst<0x0015f754u> TIER_B{};
constexpr OvConst<0x0015f758u> TIER_C{};
constexpr OvConst<0x0015f75cu> COUNT{};
constexpr OvConst<0x0015f760u> WIDTH{};
constexpr OvConst<0x0015f764u> HEIGHT{};
constexpr OvConst<0x0015f768u> RATIO_X{};
constexpr OvConst<0x0015f76cu> RATIO_Y{};
constexpr OvConst<0x0015f3f8u> FRAME{};  // the frame counter

// a signed remainder; the original's `div` is followed by a `break 7` for a zero divisor
// orig: (helper) repeated in 0x00236268 / 0x002365a8
int32_t rem_s(int32_t a, int32_t b) { return b != 0 ? a % b : 0; }

// the tier bound of bubble `i` (start: 10 / 20; the draw and the update: 10 / 30)
// orig: (helper) repeated in 0x00236268 / 0x00236458
float tier_start(int32_t i) { return i < 10 ? lwc1(TIER_A) : (i < 20 ? lwc1(TIER_B) : lwc1(TIER_C)); }
}  // namespace

// The 100 bubbles' table: each one's x at random inside its tier's width, y inside a slice of the
// frame by its index, and a velocity as the sum of three random numbers per axis.
// orig: 0x00236268
void hud_o2_start(uint32_t e) {
    hud_plain_start(e);
    for (int32_t i = 0; i < 100;) {
        const float f2 = tier_start(i);
        const int32_t cnt = lw(COUNT);
        const float f0 = f2 + f2;
        const float f13 = u2f(0x41880000u) - f2;
        float f20 = u2f(0x43680000u) - f0;
        const float f12 = f2 - u2f(0x41880000u);
        float f21 = f2 - u2f(0x42e80000u);
        const int32_t hi = rem_s(i, cnt);
        const float f1 = int_to_float(cnt);
        const uint32_t rec = BUBBLES + static_cast<uint32_t>(i) * 16u;
        f20 = fpu_div(f20, f1);
        ++i;
        f21 = f21 + int_to_float(hi) * f20;
        f20 = f21 + f20;
        swc1(rec, rand_range_f(f12, f13));
        swc1(rec + 4, rand_range_f(f21, f20));
        const float r3 = rand_range_f(u2f(0xbecccccdu), u2f(0x3ecccccdu));
        const float r4 = rand_range_f(u2f(0xbf19999au), u2f(0x3f19999au));
        swc1(rec + 8, r3);
        swc1(rec + 0xc, r4);
        const float r5 = rand_range_f(u2f(0xbecccccdu), u2f(0x3ecccccdu));
        const float r6 = rand_range_f(u2f(0xbf19999au), u2f(0x3f19999au));
        swc1(rec + 8, lwc1(rec + 8) + r5);
        const float r7 = rand_range_f(u2f(0xbecccccdu), u2f(0x3ecccccdu));
        swc1(rec + 0xc, lwc1(rec + 0xc) + r6);
        const float r8 = rand_range_f(u2f(0xbf19999au), u2f(0x3f19999au));
        swc1(rec + 8, lwc1(rec + 8) + r7);
        swc1(rec + 0xc, lwc1(rec + 0xc) + r8);
    }
}

// Every other bubble each frame (by the frame counter's parity) moves by its velocity and bounces
// off the frame's edges (the tier's bound).
// orig: 0x00236458
void hud_o2_update(uint32_t e) {
    hud_plain_update(e);
    int32_t a1 = lw(FRAME) % 2;
    uint32_t rec = BUBBLES + (static_cast<uint32_t>(a1) << 4);
    if (!(a1 < 100)) return;
    const float f10 = lwc1(TIER_A), f9 = lwc1(TIER_B), f8 = lwc1(TIER_C);
    do {
        float tier = f10;
        if (!(a1 < 10)) tier = a1 < 30 ? f9 : f8;
        float x = lwc1(rec);
        const float f3 = tier - u2f(0x41880000u);
        const float vx = lwc1(rec + 8);
        const float f4 = tier - u2f(0x42e80000u);
        float y = lwc1(rec + 4);
        x = x + vx;
        const float vy0 = lwc1(rec + 0xc);
        const float f6 = -f3;
        y = y + vy0;
        swc1(rec, x);
        const float f7 = -f4;
        swc1(rec + 4, y);
        if (x < f3) {
            swc1(rec, f3);
            swc1(rec + 8, -vx);
        }
        if (f6 < lwc1(rec)) {
            const float v = lwc1(rec + 8);
            swc1(rec, f6);
            swc1(rec + 8, -v);
        }
        if (lwc1(rec + 4) < f4) {
            const float v = lwc1(rec + 0xc);
            swc1(rec + 4, f4);
            swc1(rec + 0xc, -v);
        }
        if (f7 < lwc1(rec + 4)) {
            const float v = lwc1(rec + 0xc);
            swc1(rec + 4, f7);
            swc1(rec + 0xc, -v);
        }
        a1 += 2;
        rec += 0x20;
    } while (a1 < 100);
}

// The indicator: a flat box, the frame's sprites (blinking in the last third of the air, with a
// sound), and the bubbles below the water line that sinks as the air runs out.
// orig: 0x002365a8
int32_t hud_o2_draw(uint32_t e) {
    const uint32_t xy = ee_stack_alloc(0x10);  // the place goes by address
    sw(xy, lwu(e + 0x50));
    sw(xy + 4, lwu(e + 0x54));
    sw(e + 0x58, lwu(WIDTH));
    sw(e + 0x5c, lwu(HEIGHT));
    hud_element_place(e, xy, xy + 4);
    hud_element_offset(e, xy, xy + 4, lw(e + 0x6c), 0);
    const int32_t w = lw(WIDTH), h = lw(HEIGHT);
    // the box over the frame (1/16 pixels), the sizes rounded toward zero
    const int32_t wx = w * 0xd0, hy = h * 0xa0, wb = w * 0x260, hb = h * 0xea0;
    int32_t x0 = (lw(xy) + lh(e + 0x48)) << 4;
    int32_t y0 = (lw(xy + 4) + lh(e + 0x4a)) << 4;
    x0 += (-1 < wx ? wx : wx + 0x3f) >> 6;
    y0 += (-1 < hy ? hy : hy + 0xff) >> 8;
    const int32_t x1 = x0 + ((-1 < wb ? wb : wb + 0x3f) >> 6);
    const int32_t y1 = y0 + ((-1 < hb ? hb : hb + 0xff) >> 8);
    gs_flat_rect(x0, y0, x1, y1, 0x80684c2fu, 1);
    // the blink: in the last third of the air two square waves of the frame counter
    int32_t s1 = 0, s0 = 0;
    if (lw(e + 0x74) * 3 < 0x2711) {
        const int32_t sec = frame_scale_i(1);
        const float t = frame_scale_f(u2f(0x3f400000u));
        const int32_t thr = float_to_int(t * u2f(0x42700000u));
        const int32_t per = float_to_int(int_to_float(sec) * u2f(0x42700000u));
        s1 = rem_s(lw(FRAME) - 1, per) < thr ? 1 : 0;
    }
    if (lw(e + 0x74) * 3 < 0x2711) {
        const int32_t sec = frame_scale_i(1);
        const float t = frame_scale_f(u2f(0x3f400000u));
        const int32_t thr = float_to_int(t * u2f(0x42700000u));
        const int32_t per = float_to_int(int_to_float(sec) * u2f(0x42700000u));
        s0 = rem_s(lw(FRAME), per) < thr ? 1 : 0;
    }
    if (s1 == 0 && s0 == 1) hero_sound_start(0xb, 0);
    const float k16 = u2f(0x41800000u), k32 = u2f(0x42000000u);
    int32_t frame = hud_frame(lw(e), s0);
    suck_camera_23bac0(static_cast<uint32_t>(frame), static_cast<uint32_t>(lw(xy) + lh(e + 0x48)),
                       static_cast<uint32_t>(lw(xy + 4) + lh(e + 0x4a)), lwu(WIDTH), lwu(HEIGHT), 0x80);
    // the bubbles' origin, scale and the water level (the layout ratios of the frame)
    const float fH = int_to_float(lw(HEIGHT)), fW = int_to_float(lw(WIDTH));
    const float g1 = lwc1(RATIO_X), g2 = lwc1(RATIO_Y);
    float f3 = fH * g2;
    const int32_t used = 10000 - lw(e + 0x74);
    const float half = u2f(0x3f000000u);
    float f6 = u2f(0x3f800000u) - g1;
    const float f7 = int_to_float(used);
    float f1 = fW * g1;
    float f4 = u2f(0x3f800000u) - g2;
    f3 = f3 * half;
    float f21 = fpu_div(f7, u2f(0x461c4000u));
    f1 = f1 * half;
    f4 = f4 * half;
    const float px = int_to_float(lw(xy) + lh(e + 0x48));
    f6 = f6 * half;
    const float py = int_to_float(lw(xy + 4) + lh(e + 0x4a));
    float f5 = -f3;
    float f0 = f1 + px;
    f3 = f3 + py;
    f6 = f6 * fW;
    f4 = f4 * fH;
    float f1d = fpu_div(f1, u2f(0x41880000u));
    f5 = fpu_div(f5, u2f(0x42e80000u));
    f0 = f0 + f6;
    f3 = f3 + f4;
    float f2 = f21 + f21;
    f0 = f0 * k16;
    f3 = f3 * k16;
    f1d = f1d * k16;
    f2 = f2 * u2f(0x42e80000u);
    f5 = f5 * k16;
    f21 = u2f(0x42e80000u) - f2;
    Vec4 scale = {f1d, f5, u2f(0x3f800000u), u2f(0x3f800000u)};  // sp+0
    Vec4 origin = {f0, f3, 0.0f, 0.0f};                          // sp+0x10
    frame = hud_frame(lw(e), 2);
    Vec4 tier_a, tier_b, tier_c, pos;  // sp+0x20 / 0x30 / 0x40 / 0x50
    vec_clear(tier_a);
    vec_clear(tier_b);
    vec_clear(tier_c);
    const float a = lwc1(TIER_A), b = lwc1(TIER_B), c = lwc1(TIER_C);
    const float d1 = a * k16, d2 = (b - a) * k16, d3 = (c - b) * k16;
    tier_a.y = d1;
    tier_b.y = d2;
    tier_c.y = d3;
    tier_a.x = d1;
    tier_b.x = d2;
    tier_c.x = d3;
    int32_t size = 0;
    for (int32_t i = 0; i < 100; ++i) {
        if (i == 0) {
            vec3_sub(origin, origin, tier_a);
            const float v = lwc1(TIER_A);
            size = float_to_int(v * k32);
            f21 = f21 + v;
        } else if (i == 10) {
            vec3_sub(origin, origin, tier_b);
            const float v = lwc1(TIER_B);
            size = float_to_int(v * k32);
            f21 = f21 + (v - lwc1(TIER_A));
        } else if (i == 30) {
            vec3_sub(origin, origin, tier_c);
            const float v = lwc1(TIER_C);
            size = float_to_int(v * k32);
            f21 = f21 + (v - lwc1(TIER_B));
        }
        const uint32_t rec = BUBBLES + static_cast<uint32_t>(i) * 16u;
        if (f21 < lwc1(rec + 4)) continue;
        vec3_mul(pos, V(rec), scale);
        vec3_add(pos, pos, origin);
        hud_sprite_xyz(frame, float_to_int(pos.x), float_to_int(pos.y), size, size, 0x80);
    }
    ee_stack_free(0x10);
    return lw(e + 0x58);
}

namespace {
// orig: (none) -- the draw as an indirect-call target (its result unused there)
void hud_o2_draw_fn(uint32_t e) { (void)hud_o2_draw(e); }
}  // namespace

// orig: (none) -- the element functions for the slot's indirect calls.
void hud_o2_fns_register() {
    ovl_register_moby_fn(0x00236268, hud_o2_start);
    ovl_register_moby_fn(0x00236458, hud_o2_update);
    ovl_register_moby_fn(0x002365a8, hud_o2_draw_fn);
}

}  // namespace rac1
