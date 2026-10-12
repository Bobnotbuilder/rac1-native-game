// Ported overlay code: the Gadgebot command ring, the HUD element (slot 3) the hero's input
// 0x229b70 requests instead of the quick select while the hero is Clank (hero form 0x1413f4 == 1) and
// triangle is pressed. It is the quick select's sibling: a ring of the slots of the page 0x15f740 (4
// slots of 0x1c bytes: +0 / +4 the icon's animation and frame, +8 / +0xc / +0x10 / +0x14 the d-pad
// neighbours, +0x18 the command), chosen with the left stick or the d-pad while triangle is held; when
// triangle is let go the chosen slot's command goes to 0x141610 (0 for none), which the Gadgebots'
// update reads (Blarg's class 857 L6:0x2f0040: 1 follow -> state 3, 2 -> 4, 3 -> 5, 4 -> 7).
// The element's slot shown +0x74, the stick's hold timer +0x78, its fade counters +0x70 / +0x71.
#include <game/lib/libc.h>
#include <game/ovl/ovl_reloc.h>
#include <game/math/vecmath.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/port_stubs.h>

namespace rac1 {

namespace {
constexpr uint32_t PAD = 0x0013c940;
// orig: (helper) repeated in 0x00238b80 / 0x00238f88 — the current page's slot `i`
uint32_t gc_slot(int32_t i) {
    return static_cast<uint32_t>(i * 0x1c) + lwu(lwu(OV(0x0015fac0)) + lwu(OV(0x0015f72c)) * 4);
}
// orig: (helper) repeated in 0x00238f88 — a ratio clamped to 0..1 (the compares keep NaN)
float gc_clamp01(float v) {
    float r = 1.0f;
    if (!(1.0f < v)) {
        r = v;
        if (v < 0.0f) r = 0.0f;
    }
    return r;
}
}  // namespace

// The command ring's start: 4 slots, the page 0x15f740, its size 0x15f718 x 0x15f71c, none chosen
// (-2), the stick timer 1/2 s, page 0.
// orig: 0x00238b18
void gadgebot_cmd_start(uint32_t e) {
    sw(OV(0x0015fa90), 4);
    sw(OV(0x0015fac0), OV(0x0015f740));
    sh(e + 0x48, 0);
    sh(e + 0x4a, 0);
    sw(e + 0x58, lwu(OV(0x0015f718)));
    sw(e + 0x74, 0xfffffffeu);
    sw(e + 0x5c, lwu(OV(0x0015f71c)));
    sw(e + 0x78, static_cast<uint32_t>(frame_scale_i(0x1e)));
    sw(OV(0x0015f72c), 0);
}

// The command ring's frame: kept up (the pad's +0x1cc set to 2); the stick (beyond 0.9) points at a
// slot, then the d-pad walks the slots' neighbours (the defaults 0x17dc38.. with none chosen); the
// stick's timer only runs while 0x17e504 is set. While triangle is held the ring fades in; once let
// go the chosen slot's command is written to 0x141610 (0 with none), the ring fades out and is cleared.
// orig: 0x00238b80
void gadgebot_cmd_update(uint32_t e) {
    const uint32_t b = e + 0x70;
    sw(e + 0x7c, static_cast<uint32_t>(frame_scale_i(0xb4)));
    sw(e + 0x6c, 0x18);
    sw(PAD + 0x1cc, 2);
    float sx = lwc1(PAD + 0x148), sy = lwc1(PAD + 0x14c);
    const float len = sqrt_f(sx * sx + sy * sy);
    if (!(len == 0.0f)) {
        sx = fpu_div(sx, len);
        sy = fpu_div(sy, len);
    }
    const float ang = atan2_f(sx, sy);
    {
        const int32_t t = lw(e + 0x78);
        if ((t >> 24) == 0 && lw(OV(0x0017e504)) != 0) {
            bool reset = (lwu(PAD + 0x1c4) & 0xf000) != 0 || len < 0.5f;
            if (!reset) {
                sw(e + 0x78, static_cast<uint32_t>(t - 1));
                reset = t - 1 == -1;
            }
            if (reset) {
                sw(e + 0x74, 0xffffffffu);
                sw(e + 0x78, 0x010000ff);
            }
        }
    }
    const int32_t old = lw(e + 0x74);
    if (static_cast<int8_t>(lbu(e + 0x7b)) == 1) {
        if (u2f(0x3f666666u) < len) {
            const float PI = u2f(0x40490fdbu), TWO_PI = u2f(0x40c90fdbu), HALF_PI = u2f(0x3fc90fdbu);
            const int32_t n = lw(OV(0x0015fa90));
            const float fn = static_cast<float>(n);
            const float a = ang + PI + PI + HALF_PI + fpu_div(PI, fn);
            const float pos = a * fpu_div(fn, TWO_PI);
            sw(e + 0x74, static_cast<uint32_t>(ee_rem(float_to_int(pos), n)));
        }
        const uint32_t h = lwu(PAD + 0x1c4);
        if ((h & 0xf000) != 0) {
            int32_t s = lw(e + 0x74);
            if ((h & 0x1000) != 0) s = s == -1 ? lw(OV(0x0017dc40)) : lw(gc_slot(s) + 0x10);
            if ((lwu(PAD + 0x1c4) & 0x4000) != 0) s = s == -1 ? lw(OV(0x0017dc44)) : lw(gc_slot(s) + 0x14);
            if ((lwu(PAD + 0x1c4) & 0x8000) != 0) s = s == -1 ? lw(OV(0x0017dc38)) : lw(gc_slot(s) + 8);
            if ((lwu(PAD + 0x1c4) & 0x2000) != 0) s = s == -1 ? lw(OV(0x0017dc3c)) : lw(gc_slot(s) + 0xc);
            sw(e + 0x74, static_cast<uint32_t>(s));
        }
    }
    if (lw(e + 0x74) == old) {
        if (static_cast<int32_t>(lbu(b + 1)) < lw(OV(0x0015f780))) sb(b + 1, lbu(b + 1) + 1);
    } else {
        sb(b + 1, 0);
    }
    if ((lwu(PAD + 0x1c0) & 0x10) != 0) {
        if (static_cast<int32_t>(lbu(b)) < lw(OV(0x0015f780))) sb(b, lbu(b) + 1);
        if (lbu(b) != 0) return;
    } else {
        const int32_t s = lw(e + 0x74);
        sw(0x00141610, s == -1 ? 0u : lwu(gc_slot(s) + 0x18));
        if (lbu(b) != 0) {
            sb(b, lbu(b) - 1);
            if (lbu(b) != 0) return;
        }
    }
    sw(OV(0x0015fa98), lwu(e + 0x78));
    sw(OV(0x0015fa9c), lwu(e + 0x74));
    hud_request_clear(lw(e + 0x64));
    sw(e + 0x6c, 0xfffffffau);
    sw(OV(0x0015fa94), lwu(OV(0x0015f3f8)));
}

// The command ring: its frame (four mirrored quarters of 0xe934, frame 1), the slots' icons round the
// ring (the chosen one ringed by 0xe99c; the others dimmer; with 0x17e504 clear every icon at full
// alpha and the frame after its own +4, the greyed one), and the chosen command's name (the texts
// table 0x15f7e0, in the colours 0x15f7d4 / 0x15f7d8 over a black shadow).
// orig: 0x00238f88
int32_t gadgebot_cmd_draw(uint32_t e) {
    const float fbox = gc_clamp01(fpu_div(int_to_float(lbu(e + 0x70)), int_to_float(lw(OV(0x0015f780)))));
    const float fin = gc_clamp01(fpu_div(int_to_float(lbu(e + 0x71)), int_to_float(lw(OV(0x0015f780)))));
    const int32_t ph = ee_rem(lw(OV(0x0015f5cc)), lw(OV(0x0015f7b0)));
    float t = fpu_div(int_to_float(ph), int_to_float(lw(OV(0x0015f7b0))));
    t = t * u2f(0x40c90fd0u);
    const float sn = sin_vu(t - u2f(0x40490fd0u));
    const float a128 = fbox * 128.0f;
    const int32_t alpha = float_to_int(a128);
    const float pulse = sn * 0.125f + 0.875f;
    const int32_t ring_a = float_to_int(a128 * pulse);
    const uint32_t frame = ee_stack_alloc(0x20);  // (the original's sp+0..0x1f: the texts table copy, the place)
    const uint32_t pos = frame + 0x10;
    sw(pos, lwu(e + 0x50));
    sw(pos + 4, lwu(e + 0x54));
    hud_element_place(e, pos, pos + 4);
    const int32_t w = lw(OV(0x0015f718)), h = lw(OV(0x0015f71c));
    const int32_t hw = w / 2, hh = h / 2;
    const uint64_t a64 = static_cast<uint64_t>(static_cast<int64_t>(alpha));
    hud_sprite(hud_frame(0xe934, 1), lw(pos), lw(pos + 4), hw, hh, a64);
    hud_sprite(hud_frame(0xe934, 1), lw(pos) + lw(OV(0x0015f718)), lw(pos + 4) + lw(OV(0x0015f71c)), -hw, -hh, a64);
    hud_sprite(hud_frame(0xe934, 1), lw(pos) + lw(OV(0x0015f718)), lw(pos + 4), -hw, hh, a64);
    hud_sprite(hud_frame(0xe934, 1), lw(pos), lw(pos + 4) + lw(OV(0x0015f71c)), hw, -hh, a64);
    const float PI = u2f(0x40490fdbu);
    for (int32_t i = 0; i < lw(OV(0x0015fa90)); ++i) {
        const float fi = static_cast<float>(i), fn = static_cast<float>(lw(OV(0x0015fa90)));
        const float a = angle_add(fpu_div((fi + fi) * PI, fn) - PI, u2f(0x3fc90fdbu));
        const int32_t sx = lw(pos) + lw(OV(0x0015f720)) + float_to_int(lwc1(OV(0x0015f78c)) * cos_vu(a) * lwc1(OV(0x0015f7c4)));
        const int32_t sy = lw(pos + 4) + lw(OV(0x0015f724)) + float_to_int(lwc1(OV(0x0015f78c)) * sin_vu(a));
        if (lw(e + 0x74) == i && lw(gc_slot(i)) != 0)
            hud_sprite(hud_frame(0xe99c, 0), sx - 0x13, sy - 0x13, 0x26, 0x26, static_cast<uint64_t>(static_cast<int64_t>(ring_a)));
        if (lw(gc_slot(i)) != 0) {
            const int32_t ia = i == lw(e + 0x74) ? alpha : float_to_int(static_cast<float>(alpha) * lwc1(OV(0x0015f7c0)));
            if (lw(OV(0x0017e504)) != 0) {
                const int32_t f = hud_frame(lwu(gc_slot(i)), lw(gc_slot(i) + 4));
                hud_sprite_at(e, f, sx, sy, 1, static_cast<uint64_t>(static_cast<int64_t>(ia)));
            } else {
                const int32_t f = hud_frame(lwu(gc_slot(i)), lw(gc_slot(i) + 4) + 4);
                hud_sprite_at(e, f, sx, sy, 1, a64);
            }
        }
    }
    const int32_t sel = lw(e + 0x74);
    if (sel < 0 || lw(gc_slot(sel)) == 0) {
        ee_stack_free(0x20);
        return lw(e + 0x58);
    }
    for (uint32_t k = 0; k < 0x10; k += 4) sw(frame + k, lwu(OV(0x0015f7e0) + k));
    if (lw(gc_slot(sel)) == 0) {
        ee_stack_free(0x20);
        return lw(e + 0x58);
    }
    const uint32_t name = sub_00203ad8(lw(frame + static_cast<uint32_t>(sel) * 4));
    if (name == 0 || libc_strlen(name) == 0) {
        ee_stack_free(0x20);
        return lw(e + 0x58);
    }
    const float k = fin * fbox;
    const uint64_t col = rgba_lerp(lwu(OV(0x0015f7d4)), lwu(OV(0x0015f7d8)), k);
    const uint64_t shadow = rgba_lerp(0, 0x80000000u, k);
    const int32_t ty = lw(pos + 4) + lw(OV(0x0015f7dc)) + lw(OV(0x0015f724));
    text_centered_small(lw(pos) + lw(OV(0x0015f720)) - 1, ty - 1, shadow, name, -1);
    text_centered_small(lw(pos) + lw(OV(0x0015f720)), ty, col, name, -1);
    ee_stack_free(0x20);
    return lw(e + 0x58);
}

namespace {
// orig: (none) — the draw function as an indirect-call target
void gadgebot_cmd_draw_fn(uint32_t e) { (void)gadgebot_cmd_draw(e); }
}  // namespace

// orig: (none) — the command ring's functions for the HUD slots' indirect calls.
void gadgebot_cmd_fns_register() {
    ovl_register_moby_fn(0x00238b18, gadgebot_cmd_start);
    ovl_register_moby_fn(0x00238b80, gadgebot_cmd_update);
    ovl_register_moby_fn(0x00238f88, gadgebot_cmd_draw_fn);
}

}  // namespace rac1
