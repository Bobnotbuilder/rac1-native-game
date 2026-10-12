// Ported overlay code: the 2D drawing of the menus and messages, GS packets in the frame packet
// (cursor 0x1611c0): the scissor, flat quads and the rounded box they make, scaled text and its
// glyph sprites, and the text box (0x1fc520: +0 / +2 top / bottom, +4 / +6 left / right, +8 / +0xa
// the text's x / y, +0xc / +0xe the size laid out, +0x10 the line height, +0x12 flags (1 centred,
// 2 vertically centred, 4 measure only, 8 scaled), +0x14 / +0x16 the scaled offsets) whose text
// is wrapped into balanced lines.
#include <game/math/vecmath.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/port_stubs.h>

namespace rac1 {

namespace {
constexpr OvConst<0x001611c0u> CUR{};
// orig: (helper) a sign-extended word shifted left (the 64-bit dsll of the originals)
uint64_t sx_shl(int32_t v, int n) { return static_cast<uint64_t>(static_cast<int64_t>(v)) << n; }
}  // namespace

// The scissor (x0..x1, y0..y1, clipped to the screen 0x13e500 / 0x13e504) as a GIF packet (CNT 2).
// orig: 0x002a0c68
void packet_scissor(int32_t x0, int32_t x1, int32_t y0, int32_t y1) {
    const int32_t w = lw(0x0013e500) - 1, h = lw(0x0013e504) - 1;
    const uint32_t c = lwu(CUR);
    sw(c, 0x10000002);
    sw(c + 4, 0);
    sw(c + 8, 0);
    sw(c + 0xc, 0x50000002);
    const int32_t X1 = w < x1 ? w : x1, Y0 = -1 < y0 ? y0 : 0, X0 = -1 < x0 ? x0 : 0, Y1 = h < y1 ? h : y1;
    sw(c + 0x10, 0x8001);
    sw(c + 0x14, 0x10000000);
    sw(c + 0x18, 0xe);
    sw(c + 0x1c, 0);
    sd(c + 0x20, sx_shl(X0, 0) | sx_shl(X1, 16) | sx_shl(Y0, 32) | sx_shl(Y1, 48));
    sw(c + 0x28, 0x40);
    sw(c + 0x2c, 0);
    sw(CUR, c + 0x30);
}

// A flat quad x0..x1, y0..y1 (pixels; the corners' XYZ2 from the screen origin) in `colour`.
// orig: 0x001fa0e0
void quad_fill(int32_t y0, int32_t y1, int32_t x0, int32_t x1, uint64_t colour) {
    const uint32_t c = lwu(CUR);
    sw(c, 0x10000005);
    sw(c + 4, 0);
    sw(c + 8, 0);
    sw(c + 0xc, 0x50000005);
    lq_sq(c + 0x10, OV(0x001608e0));
    sh(c + 0x10, 0x8001);
    sd(c + 0x20, 0x144);
    sd(c + 0x28, colour);
    lq_sq(c + 0x30, OV(0x001608f0));
    sh(c + 0x30, 0x8004);
    const uint64_t Z = 0x00fffff000000000ull;
    const int32_t ox = lw(0x0013e510) - 8, oy = lw(0x0013e514) - 8;
    sd(c + 0x40, sx_shl(x0 * 16 + ox, 0) | sx_shl(y0 * 16 + oy, 16) | Z);
    sd(c + 0x48, sx_shl(x1 * 16 + ox, 0) | sx_shl(y0 * 16 + oy, 16) | Z);
    sd(c + 0x50, sx_shl(x0 * 16 + ox, 0) | sx_shl(y1 * 16 + oy, 16) | Z);
    sd(c + 0x58, sx_shl(x1 * 16 + ox, 0) | sx_shl(y1 * 16 + oy, 16) | Z);
    sw(CUR, c + 0x60);
}

// A grey box with rounded ends (7 quads), alpha `a`.
// orig: 0x001fad58
void box_rounded(int32_t y0, int32_t y1, int32_t x0, int32_t x1, uint32_t a) {
    const uint64_t col = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>((a << 24) | 0x40404)));
    quad_fill(y0, y1, x0, x1, col);
    quad_fill(y0 + 1, y1 - 1, x0 - 2, x0, col);
    quad_fill(y0 + 2, y1 - 2, x0 - 3, x0 - 2, col);
    quad_fill(y0 + 4, y1 - 4, x0 - 4, x0 - 3, col);
    quad_fill(y0 + 1, y1 - 1, x1, x1 + 2, col);
    quad_fill(y0 + 2, y1 - 2, x1 + 2, x1 + 3, col);
    quad_fill(y0 + 4, y1 - 4, x1 + 3, x1 + 4, col);
}

// A glyph sprite at (x, y) w x h pixels (floats) from texels (u, v) tw x th (the texture region
// clamped to them), colour, TEX0; none when entirely off the drawing area.
// orig: 0x001fa648
void glyph_sprite_f(int32_t u, int32_t v, int32_t tw, int32_t th, uint64_t colour, uint64_t tex0, float x, float y, float w, float h) {
    const int32_t ox = lw(0x0013e510), oy = lw(0x0013e514);
    const int32_t x0 = float_to_int(x * 16.0f) + ox - 8;
    const int32_t x1 = float_to_int((x + w) * 16.0f) + lw(0x0013e510) - 8;
    const int32_t y0 = float_to_int(y * 16.0f) + oy - 8;
    const int32_t y1 = float_to_int((y + h) * 16.0f) + lw(0x0013e514) - 8;
    if (0x9000 < x0 || x1 < 0x7000 || 0x9000 < y0 || y1 < 0x7000) return;
    const uint32_t t4 = lwu(CUR);
    sw(t4, 0x10000008);
    sw(t4 + 4, 0);
    sw(t4 + 8, 0);
    sw(t4 + 0xc, 0x50000008);
    lq_sq(t4 + 0x10, OV(0x00160920));
    sw(CUR, t4 + 0x20);
    const int32_t u1 = u + tw, v1 = v + th;
    const uint64_t Z = 0x00fffff000000000ull;
    sd(t4 + 0x20, tex0);
    sd(t4 + 0x30, sx_shl(u, 4) | sx_shl(u1, 14) | 0xa | sx_shl(v, 24) | sx_shl(v1, 34));
    sd(t4 + 0x28, 0x154);
    sd(t4 + 0x38, colour);
    auto uv = [](int32_t a, int32_t b) { return static_cast<uint64_t>(static_cast<int64_t>((b << 20) + (a << 4))); };
    sd(t4 + 0x40, uv(u, v));
    sd(t4 + 0x48, sx_shl(x0, 0) | sx_shl(y0, 16) | Z);
    sd(t4 + 0x50, uv(u1, v));
    sd(t4 + 0x58, sx_shl(x1, 0) | sx_shl(y0, 16) | Z);
    sd(t4 + 0x60, uv(u, v1));
    sd(t4 + 0x68, sx_shl(x0, 0) | sx_shl(y1, 16) | Z);
    sd(t4 + 0x70, uv(u1, v1));
    sd(t4 + 0x78, sx_shl(x1, 0) | sx_shl(y1, 16) | Z);
    sd(t4 + 0x80, 5);
    sd(t4 + 0x88, 0);
    sw(CUR, lwu(CUR) + 0x70);
}

// Text `s` (up to n characters) scaled by `scale` at (x, y) in the font `t` (as text_draw, with
// float positions: colour codes 8..15, accents, the wide grey glyphs below 0x20).
// orig: 0x001fb4f0
void text_draw_f(uint64_t colour, uint32_t s, int32_t n, uint64_t tex0, uint32_t t, float x, float y, float scale) {
    if (lw(OV(0x0015f460)) == 0) sw(OV(0x0016c838), static_cast<uint32_t>(colour));
    const float g16 = scale * 16.0f;
    if (n == 0 || lbu(s) == 0) return;
    int32_t k = 0;
    while (true) {
        const uint32_t c = lbu(s);
        if (c - 8 < 8) {
            if (lw(OV(0x0015f45c)) != 0) colour = (colour & 0xff000000ull) | (lwu(OV(0x0016c838) + (lbu(s) - 8) * 4) & 0xffffff);
        } else if (static_cast<int8_t>(lbu(t + c * 4 + 3)) != 0) {
            const float dy = int_to_float(static_cast<int8_t>(lbu(t + c * 4 + 2))) * scale;
            if (((lbu(s) + 0x80) & 0xff) < 0x28) {
                const uint32_t g = (lbu(s) + 0x40) * 4 + t;
                const float ax = int_to_float(static_cast<int8_t>(lbu(g + 3))) * scale;
                const float ay = int_to_float(static_cast<int8_t>(lbu(g + 2))) * scale;
                glyph_sprite_f(lbu(g), lbu(g + 1), 0x10, 0x10, colour, tex0, x + ax, y + ay, g16, g16);
            }
            const uint32_t c2 = lbu(s);
            if (c2 < 0x20) {
                const int64_t sum = static_cast<int64_t>(colour & 0xff) + static_cast<int64_t>((colour >> 8) & 0xff) + static_cast<int64_t>((colour >> 16) & 0xff);
                const int32_t m = ee_div(static_cast<int32_t>(sum), 3);
                const int32_t hi = static_cast<int32_t>(static_cast<uint32_t>(colour & 0xff000000ull));
                const int32_t col = m + hi + (m << 16) + (m << 8);
                const uint32_t gg = lbu(s) * 4 + t;
                glyph_sprite_f(lbu(gg), lbu(gg + 1), 0x18, 0x10, static_cast<uint64_t>(static_cast<int64_t>(col)), tex0, x, y + dy, scale * 24.0f, scale * 16.0f);
            } else if (0x20 < c2) {
                const uint32_t gg = lbu(s) * 4 + t;
                glyph_sprite_f(lbu(gg), lbu(gg + 1), 0x10, 0x10, colour, tex0, x, y + dy, g16, g16);
            }
            x = x + int_to_float(static_cast<int8_t>(lbu(lbu(s) * 4 + t + 3))) * scale;
        }
        ++k;
        if (k == n) return;
        ++s;
        if (lbu(s) == 0) return;
    }
}

// A text box's fields (see the file's head).
// orig: 0x001fc520
void text_box_set(uint32_t b, int32_t top, int32_t bottom, int32_t left, int32_t right, int32_t x, int32_t y, int32_t line, uint32_t flags) {
    sh(b, static_cast<uint32_t>(top));
    sh(b + 2, static_cast<uint32_t>(bottom));
    sh(b + 4, static_cast<uint32_t>(left));
    sh(b + 6, static_cast<uint32_t>(right));
    sh(b + 8, static_cast<uint32_t>(x));
    sh(b + 0xa, static_cast<uint32_t>(y));
    sh(b + 0x10, static_cast<uint32_t>(line));
    sh(b + 0x12, flags);
    sh(b + 0x16, 0);
    sh(b + 0xc, 0);
    sh(b + 0xe, 0);
    sh(b + 0x14, 0);
}

// Text `s` (up to n characters) in box `b` in the font `t`, TEX0 `tex0`: wrapped at spaces /
// control codes into lines (balanced: a narrower width is tried while the last line is short,
// back to the first when that adds lines), each line in its colour code; clipped to the box.
// orig: 0x001fbf48
void text_box(uint32_t b, uint64_t colour, uint32_t s, int32_t n, uint64_t tex0, uint32_t t) {
    // the original's frame: the lines' starts +0, ends +0x40, colour codes +0x80 (halfwords), the
    // colour +0xc0, TEX0 +0xc8, the full width +0xd0
    const uint32_t f = ee_stack_alloc(0xe0);
    auto START = [f](int32_t l) { return f + static_cast<uint32_t>(l) * 2; };
    auto END = [f](int32_t l) { return f + 0x40 + static_cast<uint32_t>(l) * 2; };
    auto CCOL = [f](int32_t l) { return f + 0x80 + static_cast<uint32_t>(l) * 2; };
    sd(f + 0xc0, colour);
    sd(f + 0xc8, tex0);
    packet_scissor(lh(b + 4), lh(b + 6) - 1, lh(b + 0), lh(b + 2) - 1);
    sw(OV(0x0015f460), 1);
    int32_t width;
    if (((lhu(b + 0x12) ^ 1) & 1) != 0) {
        width = lh(b + 6) - lh(b + 8);
    } else {
        const int32_t r = lh(b + 6) - lh(b + 8), l = lh(b + 8) - lh(b + 4);
        width = (r < l ? r : l) << 1;
    }
    sw(f + 0xd0, static_cast<uint32_t>(width));
    int32_t cc = 0, retry = 0, first_lines = 0, last_w = 0, lines = 0;
    for (;;) {
        lines = 0;
        int32_t i = 0;
        if (n != 0 && lbu(s) != 0) {
            for (;;) {
                const int32_t next = lines + 1;
                sh(START(lines), static_cast<uint32_t>(i));
                sh(CCOL(lines), static_cast<uint32_t>(cc));
                int32_t brk = i, w = 0;
                if (0 < width) {
                    const int32_t codes = lw(OV(0x0015f45c));
                    uint32_t p = s + static_cast<uint32_t>(i);
                    do {
                        const uint32_t ch = lbu(p);
                        if (ch == 0x20 || ch < 0x10) brk = i;
                        if (codes != 0 && static_cast<uint32_t>(lbu(p)) - 8u < 8u) cc = static_cast<int32_t>(lbu(p)) - 8;
                        if (lbu(p) < 2) break;
                        ++i;
                        const int32_t a = static_cast<int8_t>(lbu(t + lbu(p) * 4 + 3));
                        if (a != 0) w += a;
                        ++p;
                    } while (w < width);
                }
                sh(END(lines), static_cast<uint32_t>(brk));
                if (static_cast<int16_t>(brk) == lh(START(lines))) sh(END(lines), static_cast<uint32_t>(i));
                const int32_t e = lh(END(lines));
                const uint32_t ec = lbu(s + static_cast<uint32_t>(e));
                if (ec == 0x20 || ec < 0x10) sh(END(lines), lhu(END(lines)) - 1);
                lines = next;
                if (lbu(s + static_cast<uint32_t>(e)) == 0) {
                    last_w = w;
                    break;
                }
                i = e + 1;
                if (i == n) break;
                if (lbu(s + static_cast<uint32_t>(i)) == 0) break;
            }
        }
        if (retry != 0) break;
        if (first_lines == 0) first_lines = lines;
        if (lines < 2) break;
        if (first_lines < lines) {
            width = lw(f + 0xd0);
            retry = 1;
            continue;
        }
        const int32_t third = ee_div(width, 3);
        width -= 0x10;
        if (!(last_w < third)) break;
    }
    const int32_t lh_ = lh(b + 0x10);
    const uint32_t flags = lhu(b + 0x12);
    sh(b + 0xc, 0);
    sh(b + 0xe, static_cast<uint32_t>(lines * lh_));
    int32_t y = lh(b + 0xa);
    if ((flags & 2) != 0) y -= (lines * lh_) >> 1;
    for (int32_t l = 0; l < lines; ++l) {
        if (!(y + lh(b + 0x10) < lh(b + 0)) && !(lh(b + 2) < y)) {
            const int32_t st = lh(START(l));
            const int32_t cnt = lh(END(l)) - st + 1;
            const int32_t w = text_width(s + static_cast<uint32_t>(st), cnt, t);
            if (lh(b + 0xc) < w) sh(b + 0xc, static_cast<uint32_t>(w));
            if ((lhu(b + 0x12) & 4) == 0) {
                sw(OV(0x0016c838), static_cast<uint32_t>(ld(f + 0xc0)));
                const uint32_t fl = lhu(b + 0x12);
                const uint64_t lc = static_cast<uint64_t>(static_cast<int64_t>(lw(OV(0x0016c838) + static_cast<uint32_t>(lh(CCOL(l))) * 4)));
                const uint64_t tx = ld(f + 0xc8);
                if ((fl & 8) != 0) {
                    float fx;
                    if ((fl & 1) != 0) {
                        const int32_t hw = w >> 1;
                        fx = static_cast<float>(lh(b + 8) - hw) + static_cast<float>(lh(b + 0x14)) * 0.0625f;
                    } else {
                        fx = static_cast<float>(lh(b + 8)) + static_cast<float>(lh(b + 0x14)) * 0.0625f;
                    }
                    const float fy = static_cast<float>(y) + static_cast<float>(lh(b + 0x16)) * 0.0625f;
                    text_draw_f(lc, s + static_cast<uint32_t>(st), cnt, tx, t, fx, fy, 1.0f);
                } else if ((fl & 1) != 0) {
                    text_draw(lh(b + 8) - (w >> 1), y, lc, s + static_cast<uint32_t>(st), cnt, tx, t);
                } else {
                    text_draw(lh(b + 8), y, lc, s + static_cast<uint32_t>(st), cnt, tx, t);
                }
            }
        }
        y += lh(b + 0x10);
    }
    sw(OV(0x0015f460), 0);
    packet_scissor(0, lw(0x0013e500) - 1, 0, lw(0x0013e504) - 1);
    ee_stack_free(0xe0);
}

// A text box in the menus' font (0x1c34f0, effect texture 2).
// orig: 0x001fc4a8
void text_box_draw(uint32_t b, uint64_t colour, uint32_t s, int32_t n) {
    const uint64_t tex0 = effect_tex0(2);
    text_box(b, colour, s, n, tex0, OV(0x001c34f0));
}

}  // namespace rac1
