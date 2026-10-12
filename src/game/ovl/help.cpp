// Ported overlay code: the help desk (the tutorial messages of the first levels). Its state at
// 0x179410: +0 the step (0 waiting, 1..7 a message shown: the icon grows (1), its box opens (3),
// the text (4..6), closes (7); 8 done), +4 the step's frames, +8 / +0xc the box's half size,
// +0x10 / +0x14 its centre, +0x18 / +0x1c drawn half size, +0x20 the message (table 0x15f660,
// 16 bytes: +0 the text, +8 the voice line), +0x24 the message to show next, +0x28 the stat
// counted, +0x30 the help enabled (after 2 s of the d-pad / a first press), +0x38 / +0x3a / +0x3c
// a pending repeat. The voice lines are the dialog streams' (0x1516d0).
#include <game/lib/libc.h>
#include <game/ovl/ovl_reloc.h>
#include <game/lib/snd989.h>
#include <game/math/vecmath.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/port_stubs.h>

namespace rac1 {

namespace {
constexpr OvConst<0x00179410u> H{};
constexpr uint32_t STREAM = 0x001516d0;  // the dialog stream state
// the message's voice line (-1: none)
uint32_t msg(int32_t i) { return lwu(OV(0x0015f660)) + static_cast<uint32_t>(i) * 16; }
// orig: (helper) repeated in 0x00203cc0 — the stat of the message (0x141968 + 8 * +0x28): its
// count, the time it was first seen (in 10 s), the levels it was seen on
void help_stat(bool count) {
    const uint32_t r = lwu(H + 0x28) * 8 + 0x00141968;
    if (count && !(0xfffe < lhu(r))) sh(r, lhu(r) + 1);
    const int32_t q = ee_div(frame_scale_i(lw(0x0015eea4)), 0x258);
    if (static_cast<int32_t>(lhu(lwu(H + 0x28) * 8 + 0x00141968 + 2)) < q)
        sh(lwu(H + 0x28) * 8 + 0x00141968 + 2, static_cast<uint32_t>(ee_div(frame_scale_i(lw(0x0015eea4)), 0x258)));
    const uint32_t w = lwu(H + 0x28) * 8 + 0x0014196c;
    sw(w, lwu(w) | (1u << (lwu(0x0015ed84) & 31)) | 0x80000000u);
}
}  // namespace

// The HUD message line 0x17e530: the text `s` (the default 0x1e84d8 first, under 0x50 long).
// orig: 0x00237718
void hud_message_text_set(uint32_t s) {
    if (static_cast<uint32_t>(libc_strlen(s)) < 0x50) {
        for (uint32_t i = 0; i < 0x13; ++i) sb(OV(0x0017e530) + i, lbu(OV(0x001e84d8) + i));
    }
    libc_strcpy(OV(0x0017e530), s);
}

// The HUD message request of `who` with the text id `id` (0: none): taken when free or already
// `who`'s (2 / 1), 0 when another holds it.
// orig: 0x002643c8
int32_t hud_message_request(int32_t who, int32_t id) {
    const int32_t cur = lw(OV(0x0015f594));
    if (cur == who) {
        if (id != 0) hud_message_text_set(sub_00203ad8(id));
        sw(OV(0x00161e08), static_cast<uint32_t>(id));
        sw(OV(0x0015f590), 2);
        return 2;
    }
    if (cur != 0) return 0;
    if (id != 0) hud_message_text_set(sub_00203ad8(id));
    sw(OV(0x0015f594), static_cast<uint32_t>(who));
    sw(OV(0x0015f590), 2);
    sw(OV(0x00161e08), static_cast<uint32_t>(id));
    return 1;
}

// The same, taken over from another holder (3) when refused.
// orig: 0x00264470
int32_t hud_message_force(int32_t who, int32_t id) {
    const int32_t r = hud_message_request(who, id);
    if (r != 0) return r;
    if (id != 0) hud_message_text_set(sub_00203ad8(id));
    sw(OV(0x0015f594), static_cast<uint32_t>(who));
    sw(OV(0x0015f590), 2);
    sw(OV(0x00161e08), static_cast<uint32_t>(id));
    return 3;
}

// The dialog stream playing (state 3) stopped: 1, else 0.
// orig: 0x002652e8
int32_t dialog_stream_stop() {
    if (lwu(STREAM + 0x50) == 0) return 0;
    if (lh(STREAM + 0x5a) != 3) return 0;
    snd_cmd_2e(lwu(STREAM + 0x50));
    sh(STREAM + 0x5a, 4);
    return 1;
}

// A message shown: step 1, its sound, its box measured (the text box at the screen's bottom,
// measure only) for the box's size and place.
// orig: 0x00203b88
void help_show() {
    sw(H, 1);
    sw(H + 4, 0);
    if (lbu(0x0015ee1d) != 0 || lbu(0x0015ee1c) != 0) sound_play_indexed(0, 1, 0);
    const uint32_t s = lwu(msg(lw(H + 0x20)));
    const uint32_t box = ee_stack_alloc(0x20);
    text_box_set(box, 0xf0, 0x1e0, 0x2c, 0x1d4, 0x100, 0x168, 0x10, 7);
    text_box_draw(box, 0x80ffa888u, s, -1);
    const int32_t sh_ = lw(0x0013e504);
    const int32_t hh = static_cast<int16_t>(lhu(box + 0xe)) >> 1;
    const int32_t y = sh_ - 0x3c;
    const int32_t yh = hh + 5;
    const int32_t hw = static_cast<int16_t>(lhu(box + 0xc)) >> 1;
    ee_stack_free(0x20);
    sw(H + 8, static_cast<uint32_t>(hw + 10));
    sw(H + 0x10, 0x100);
    sw(H + 0x1c, 8);
    sw(H + 0xc, static_cast<uint32_t>(yh));
    sw(H + 0x18, 8);
    sw(H + 0x14, static_cast<uint32_t>(y));
    if (sh_ - 0xc < y + yh) sw(H + 0x14, static_cast<uint32_t>(sh_ - (hh + 0x11)));
}

// The help desk's frame (see the file's head): enabled once the d-pad was used for 2 s; the
// steps; triangle (pad 0x10) skips a message (counted), the voice line paces the text.
// orig: 0x00203cc0
void help_update() {
    constexpr uint32_t PRESSED = 0x0013cae4;
    if (lw(H + 0x30) == 0) {
        if (lw(H + 0x34) == 0 && (lwu(0x0013cae0) & 0xf000) != 0) sw(H + 0x34, 1);
        if (lw(H + 0x34) != 0) {
            const int32_t t = lw(H + 0x34) + 1;
            sw(H + 0x34, static_cast<uint32_t>(t));
            if (!(t < frame_scale_i(0x78))) sw(H + 0x30, 1);
        }
    }
    if (lw(OV(0x0015f5c4)) != 0 || lw(H + 0x30) == 0) {
        sw(H + 0x24, 0xffffffffu);
        sw(H, 0);
        sw(H + 4, 0);
        return;
    }
    const uint32_t step = lwu(H);
    sw(H + 4, lwu(H + 4) + 1);
    if (step >= 9) return;
    switch (step) {
    case 8:
        if (lh(H + 0x38) != 0) return;
        if (!(lw(H + 0x20) < 0)) {
            if (lh(H + 0x3a) == 0) {
                if (timer_tick_i32(*static_cast<int32_t*>(ee_ram_ptr(H + 0x3c))) == 0) return;
                help_show();
                sh(H + 0x3a, lhu(H + 0x3a) + 1);
                return;
            }
            help_stat(true);
        } else {
            help_stat(false);
        }
        sw(H + 0x20, 0xffffffffu);
        sw(H, 0);
        sh(H + 0x3a, 0);
        return;
    case 0: {
        const int32_t next = lw(H + 0x24);
        if (next < 0) return;
        const int32_t m = sub_00203a68(next);
        sw(H + 0x20, static_cast<uint32_t>(m));
        sw(H + 0x24, 0xffffffffu);
        if (m < 0) return;
        help_show();
        return;
    }
    case 1: {
        hud_message_force(5, 0);
        const int32_t voice = lw(msg(lw(H + 0x20)) + 8);
        if (!(voice < 0) && lwu(STREAM + 0x50) == 0 && lw(STREAM + 0x1c) == -1) sw(STREAM + 0x1c, static_cast<uint32_t>(voice + 0x7530));
        if ((lwu(PRESSED) & 0x10) != 0) {
            help_stat(true);
            sw(H + 4, static_cast<uint32_t>(8 - lw(H + 4)));
            sw(H, 7);
            return;
        }
        if (lw(H + 4) < 6) return;
        sw(H + 4, 0);
        sw(H, 2);
        return;
    }
    case 2: {
        hud_message_force(5, 0);
        if ((lwu(PRESSED) & 0x10) != 0) {
            help_stat(true);
            sw(H, 7);
            sw(H + 4, 0);
            return;
        }
        if (lw(H + 4) < frame_scale_i(0x18)) return;
        if (lh(STREAM + 0x5a) != 3) {
            const int32_t voice = lw(msg(lw(H + 0x20)) + 8);
            if (voice != -1 && voice == lh(STREAM + 0x54) - 0x7530) return;
        }
        sw(H, 3);
        sw(H + 4, 0);
        return;
    }
    case 3:
        hud_message_force(5, 0);
        if ((lwu(PRESSED) & 0x10) != 0) {
            help_stat(true);
            sw(H, 7);
            sw(H + 4, 0);
            return;
        }
        if (lw(H + 4) < 8) return;
        sw(H + 4, 0);
        sw(H, 4);
        return;
    case 4:
        hud_message_force(5, 0);
        if ((lwu(PRESSED) & 0x10) != 0) {
            help_stat(true);
            sw(H, 6);
            sw(H + 4, static_cast<uint32_t>(4 - lw(H + 4)));
            return;
        }
        if (lw(H + 4) < 4) return;
        {
            const int32_t voice = lw(msg(lw(H + 0x20)) + 8);
            if (voice != -1 && voice == lh(STREAM + 0x54) - 0x7530 && lh(STREAM + 0x5a) == 3) dialog_stream_stop();
        }
        sw(H, 5);
        sw(H + 4, 0);
        return;
    case 5: {
        hud_message_force(5, 0);
        bool done = false;
        if (!(lw(H + 4) < frame_scale_i(0x1a4))) {
            const int32_t voice = lw(msg(lw(H + 0x20)) + 8);
            if (voice == -1) done = true;
            else if (voice != lh(STREAM + 0x54) - 0x7530) done = true;
            else if (lwu(STREAM + 0x50) == 0 && lw(STREAM + 0x1c) == -1) done = true;
        }
        if (!done && (lwu(PRESSED) & 0x10) == 0) return;
        help_stat(true);
        sw(H, 6);
        sw(H + 4, 0);
        return;
    }
    case 6:
        if (lbu(0x0015ee1d) != 0 && lw(H + 4) < 4 && (lwu(PRESSED) & 0x10) == 0) return;
        sw(H, 7);
        sw(H + 4, 0);
        return;
    case 7: {
        hud_message_force(5, 0);
        const int32_t voice = lw(msg(lw(H + 0x20)) + 8);
        if (voice != -1 && voice == lh(STREAM + 0x54) - 0x7530 && !((lhu(STREAM + 0x5a) - 6u) < 2u)) sh(STREAM + 0x5a, 5);
        if (lw(H + 4) < 8) return;
        if (lh(H + 0x38) != 0) {
            sw(H, 8);
        } else {
            sw(H, 0);
            sw(H + 0x20, 0xffffffffu);
        }
        sw(H + 4, 0);
        return;
    }
    default:
        return;
    }
}

// The help desk's icon (effect texture 4) on its box, fading in on step 1 and out on step 7.
// orig: 0x002046c8
void help_icon_draw() {
    sw(H + 0x1c, 0x20);
    sw(H + 0x18, 0x20);
    box_rounded(lw(H + 0x14) - 0x20, lw(H + 0x14) + 0x20, lw(H + 0x10) - 0x20, lw(H + 0x10) + 0x20, 0x60);
    int32_t a = 0x7e;
    const int32_t st = lw(H);
    if (st == 1 || st == 7) a = lw(H + 4) * 0x15;
    if (!(a < 0x81)) a = 0x80;
    const uint64_t tex0 = effect_tex0(4);
    const uint64_t col = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>((static_cast<uint32_t>(a) << 24) | 0x808080u)));
    glyph_sprite(lw(H + 0x10) - 0x20, lw(H + 0x14) - 0x20, 0x40, 0x40, 0, 0, 0x40, 0x40, col, tex0);
}

// The help desk's drawing (while enabled, with the help option 0x15ee1d / 0x15ee1c): by step,
// the box growing from the icon, the icon, the text in it (fading in / out on 4 / 6), the box
// closing.
// orig: 0x002047b0
void help_draw() {
    const int32_t st = lw(H);
    if (st == 0 || lw(H + 0x30) == 0) return;
    const uint32_t opt = lbu(0x0015ee1d);
    if (opt == 0 && lbu(0x0015ee1c) == 0) return;
    const uint32_t i = static_cast<uint32_t>(st - 1);
    if (i >= 7) return;
    auto div8 = [](int32_t v) { return (-1 < v ? v : v + 7) >> 3; };
    switch (i) {
    case 0: {
        if (opt == 0) return;
        const int32_t r = lw(H + 4) * 4 + 8;
        sw(H + 0x18, static_cast<uint32_t>(r));
        sw(H + 0x1c, static_cast<uint32_t>(r));
        box_rounded(lw(H + 0x14) - r, lw(H + 0x14) + r, lw(H + 0x10) - r, lw(H + 0x10) + r, 0x60);
        return;
    }
    case 1:
        if ((opt & 0xff) == 0) return;
        help_icon_draw();
        return;
    case 2: {
        if ((opt & 0xff) == 0) return;
        const int32_t t = lw(H + 4);
        const int32_t rx = div8((lw(H + 8) - 0x20) * t) + 0x20;
        const int32_t ry = div8((lw(H + 0xc) - 0x20) * t) + 0x20;
        box_rounded(lw(H + 0x14) - ry, lw(H + 0x14) + ry, lw(H + 0x10) - rx, lw(H + 0x10) + rx, 0x60);
        sw(H + 0x18, static_cast<uint32_t>(rx));
        sw(H + 0x1c, static_cast<uint32_t>(ry));
        int32_t a = (8 - lw(H + 4)) << 4;
        if (!(-1 < a)) a = 0;
        const uint64_t tex0 = effect_tex0(4);
        const uint64_t col = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>((static_cast<uint32_t>(a) << 24) | 0x808080u)));
        glyph_sprite(lw(H + 0x10) - 0x20, lw(H + 0x14) - 0x20, 0x40, 0x40, 0, 0, 0x40, 0x40, col, tex0);
        return;
    }
    case 3: case 4: case 5: {
        if ((opt & 0xff) == 0) return;
        int32_t col = static_cast<int32_t>(0x80ffa888u);
        const int32_t rx = lw(H + 8), ry = lw(H + 0xc);
        sw(H + 0x18, static_cast<uint32_t>(rx));
        sw(H + 0x1c, static_cast<uint32_t>(ry));
        box_rounded(lw(H + 0x14) - ry, lw(H + 0x14) + ry, lw(H + 0x10) - rx, lw(H + 0x10) + rx, 0x60);
        if (lw(H) == 4) col = static_cast<int32_t>((lwu(H + 4) << 29) | 0xffa888u);
        else if (lw(H) == 6) col = static_cast<int32_t>((static_cast<uint32_t>(4 - lw(H + 4)) << 29) | 0xffa888u);
        const uint32_t box = ee_stack_alloc(0x20);
        text_box_set(box, 0xf0, 0x1e0, 0x2c, 0x1d4, 0x100, lw(H + 0x14), 0x10, 3);
        text_box_draw(box, static_cast<uint64_t>(static_cast<int64_t>(col)), lwu(msg(lw(H + 0x20))), -1);
        ee_stack_free(0x20);
        return;
    }
    case 6: {
        if ((opt & 0xff) == 0) return;
        const int32_t t = lw(H + 4);
        const int32_t hy = lw(H + 0x1c), hx = lw(H + 0x18);
        const int32_t ry = hy - div8((hy - 8) * t);
        const int32_t rx = hx - div8((hx - 8) * t);
        box_rounded(lw(H + 0x14) - ry, lw(H + 0x14) + ry, lw(H + 0x10) - rx, lw(H + 0x10) + rx, static_cast<uint32_t>((8 - t) * 0xc));
        return;
    }
    default:
        return;
    }
}

}  // namespace rac1
