// Ported overlay code: the item grids of the Weapons / Gadgets / Quick Select / Items pages (see
// pause_menu_pages.h): a grid of HUD icons moved through with
// the pad (to the neighbouring grids at its edges), cross equipping the item.
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/pause_menu.h>
#include <game/ovl/pause_menu_pages.h>

namespace rac1 {

namespace {
constexpr OvConst<0x001b9cf0u> MS{};   // the menu's state
constexpr uint32_t PAD = 0x0013c940;  // pad 1
// orig: (helper) EE CVT.W.S: truncate toward zero, saturating
int32_t cvt_w_s(float f) {
    if (f >= 2147483648.0f) return INT32_MAX;
    if (f <= -2147483648.0f) return INT32_MIN;
    return static_cast<int32_t>(f);
}
// orig: (helper) repeated in 0x002794a0 / 0x002793f0: a grid hidden from navigation (M+0x134
// with its flag 8, M+0x138 with its flag 4)
bool grid_skipped(uint32_t g) {
    bool skip = false;
    if (lw(MS + 0x134) != 0) skip = (lwu(g + 0x30) & 8) != 0;
    if (lw(MS + 0x138) != 0 && (lwu(g + 0x30) & 4) != 0) skip = true;
    return skip;
}
// orig: (helper) repeated in 0x002794a0: the column kept when moving between a grid of `from`
// columns and one of `to` columns (3 <-> 5: the 3 sit under the middle of the 5)
int32_t column_across(int32_t col, int32_t from, int32_t to) {
    if (to == 5 && from == 3) ++col;
    if (to == 3 && from == 5) {
        --col;
        if (!(col < 3)) col = 2;
        else if (!(-1 < col)) col = 0;
    }
    return col;
}
}  // namespace

// A grid's update (+0x3c the selected cell, +0x40 rows, +0x44 columns, +0x48 the cells: 10 bytes,
// +0 / +2 HUD frame key / index, +4 kind (0 an item of 0x13d4c0, 1 of 0x13d388), +6 the item,
// +8 its picture; +0x4c / +0x50 / +0x54 / +0x58 the grids above / below / left / right; +0x30
// flags: 1 no equipping, 0x8000 no wrapping). Cross on an owned item equips it in its slot
// (M+0x30.., the item table 0x179ac0 +8; again unequips but for slots 0 and 3), the item 0x18
// raises the hero's 0x141347 instead.
// orig: 0x002794a0
int32_t menu_grid_update(uint32_t w) {
    if (lwu(lwu(MS + 4) + 0x40) != w) return 0;
    if ((lwu(PAD + 0x1c4) & 0xd00) != 0 && lw(MS + 0x124) == 0) return 1;
    if ((lwu(PAD + 0x1c4) & 0x10) != 0) {
        const uint32_t parent = lwu(lwu(MS + 4) + 0x38);
        if (parent != 0) sw(MS + 8, parent);
        else if (lw(MS + 0x124) == 0) return -1;
    }
    const int32_t cols = lw(w + 0x44), sel = lw(w + 0x3c), rows = lw(w + 0x40);
    const int32_t col = ee_rem(sel, cols), row = ee_div(sel, cols);
    int32_t c = col;
    if ((lwu(PAD + 0x1c4) & 0x1000) != 0) {
        if (row != 0) {
            sw(w + 0x3c, static_cast<uint32_t>(sel - cols));
        } else if (lwu(w + 0x4c) != 0) {
            uint32_t g = lwu(w + 0x4c);
            while (grid_skipped(g)) g = lwu(g + 0x4c);
            const int32_t trows = lw(g + 0x40), tcols = lw(g + 0x44);
            sw(lwu(MS + 4) + 0x80, g);
            c = column_across(col, lw(w + 0x44), tcols);
            sw(g + 0x3c, static_cast<uint32_t>((trows - 1) * tcols + (c < tcols - 1 ? c : tcols - 1)));
        } else if ((lwu(w + 0x30) & 0x8000) == 0) {
            sw(w + 0x3c, static_cast<uint32_t>(cols * (rows - 1) + sel));
        }
    }
    if ((lwu(PAD + 0x1c4) & 0x4000) != 0) {
        if (row + 1 < rows) {
            sw(w + 0x3c, static_cast<uint32_t>(lw(w + 0x3c) + cols));
        } else if (lwu(w + 0x50) != 0) {
            uint32_t g = lwu(w + 0x50);
            while (grid_skipped(g)) g = lwu(g + 0x50);
            const int32_t tcols = lw(g + 0x44);
            sw(lwu(MS + 4) + 0x80, g);
            c = column_across(c, lw(w + 0x44), tcols);
            sw(g + 0x3c, static_cast<uint32_t>(c < tcols - 1 ? c : tcols - 1));
        } else if ((lwu(w + 0x30) & 0x8000) == 0) {
            sw(w + 0x3c, static_cast<uint32_t>(lw(w + 0x3c) - lw(w + 0x44) * (lw(w + 0x40) - 1)));
        }
    }
    if ((lwu(PAD + 0x1c4) & 0x8000) != 0) {
        if (c != 0) {
            sw(w + 0x3c, static_cast<uint32_t>(lw(w + 0x3c) - 1));
        } else if (lwu(w + 0x54) != 0) {
            sw(lwu(MS + 4) + 0x80, lwu(w + 0x54));
        } else if ((lwu(w + 0x30) & 0x8000) == 0) {
            sw(w + 0x3c, static_cast<uint32_t>(lw(w + 0x3c) - 1 + lw(w + 0x44)));
        }
    }
    if ((lwu(PAD + 0x1c4) & 0x2000) != 0) {
        if (c + 1 < cols) {
            sw(w + 0x3c, static_cast<uint32_t>(lw(w + 0x3c) + 1));
        } else if (lwu(w + 0x58) != 0) {
            sw(lwu(MS + 4) + 0x80, lwu(w + 0x58));
        } else if ((lwu(w + 0x30) & 0x8000) == 0) {
            sw(w + 0x3c, static_cast<uint32_t>(lw(w + 0x3c) + 1 - lw(w + 0x44)));
        }
    }
    if (lw(w + 0x3c) != sel || lwu(lwu(MS + 4) + 0x80) != 0) moby_sound_start(1, 0x11, lwu(w + 0x14));
    if ((lwu(PAD + 0x1c4) & 0x40) == 0) return 0;
    if ((lwu(w + 0x30) & 1) != 0) return 0;
    const uint32_t cell = lwu(w + 0x48) + static_cast<uint32_t>(lw(w + 0x3c) * 10);
    const int32_t item = lh(cell + 6);
    if (item == 0 || lbu(static_cast<uint32_t>(item) + 0x0013d4c0) == 0) {
        moby_sound_start(2, 0x11, lwu(w + 0x14));
        return 0;
    }
    const uint32_t slot = lwu(OV(0x00179ac0) + static_cast<uint32_t>(item * 0x4c) + 8);
    moby_sound_start(0, 0x11, lwu(w + 0x14));
    const uint32_t eq = MS + 0x30 + slot * 4;
    if (lw(eq) == lh(cell + 6) && slot != 0 && slot != 3) {
        sw(eq, 0);
        return 0;
    }
    const int32_t it = lh(cell + 6);
    if (it == 0x18) {
        if (sub_00233e98(0x18) == 0) return 0;
        if (lw(MS) != 3) return 0;
        const uint32_t n = sub_00233e98(0x18);
        sb(0x00141345, 1);
        uint32_t v = lbu(0x00141347) + n;
        if (!(static_cast<int32_t>(v) < 7)) v = 6;
        sb(0x00141347, v);
        return 0;
    }
    sw(MS + 0x30 + slot * 4, static_cast<uint32_t>(it));
    return 0;
}

// A grid's focus in after a change of focus (a1 0: nothing): a grid hidden from navigation
// (M+0x134 / M+0x138, its flags 8 / 4) passes the focus on down (+0x4c chain).
// orig: 0x002793f0
int32_t menu_grid_focus_in(uint32_t w, int32_t change) {
    if (change != 0) return 0;
    if (!grid_skipped(w)) return 0;
    uint32_t g = lwu(w + 0x4c);
    while (grid_skipped(g)) g = lwu(g + 0x4c);
    sw(lwu(MS + 4) + 0x80, g);
    return 0;
}

// A grid's icons laid out over its moby's panel (+0x40 / +0x44 its size; +0x34 / +0x38 the
// margins, the icon size 0x160350 / 0x160354; flag 2 a fixed row gap), scaled to the render
// target; the selected one (focused) on a pulsing frame; an item not owned not drawn; the HUD
// frame's index +2 (equipped, unless flag 0x20), +4 (a gold weapon, 0x13e520), +2 hidden.
// orig: 0x0027b590
int32_t menu_grid_draw(uint32_t w) {
    const bool focused = lwu(lwu(MS + 4) + 0x40) == w;
    const uint32_t d = lwu(lwu(w + 0x14) + 0x78);
    uint32_t cell = lwu(w + 0x48);
    sub_002a0ba8(0x42, 0x8000000044ull);
    sub_002a0ba8(0x47, 0xb);
    texture_block_begin(0);
    const int32_t cols = lw(w + 0x44);
    const float pw = lwc1(d + 0x40);
    const float iw = lwc1(OV(0x00160350));
    float x0, dx;
    if (!(cols < 2)) {
        x0 = lwc1(w + 0x34);
        const float m2 = fpu_add(x0, x0);
        const float t = fpu_mul(iw, static_cast<float>(cols));
        dx = fpu_add(iw, fpu_div(fpu_sub(fpu_sub(pw, m2), t), static_cast<float>(cols - 1)));
    } else {
        x0 = fpu_mul(fpu_sub(pw, iw), 0.5f);
        dx = 0.0f;
    }
    const float ih = lwc1(OV(0x00160354));
    float ph, y0, dy;
    const int32_t rows = lw(w + 0x40);
    if ((lwu(w + 0x30) & 2) != 0) {
        ph = lwc1(d + 0x44);
        dy = fpu_add(ih, u2f(0x3e19999a));
        y0 = lwc1(w + 0x38);
    } else {
        ph = lwc1(d + 0x44);
        if (!(rows < 2)) {
            y0 = lwc1(w + 0x38);
            const float m2 = fpu_add(y0, y0);
            const float t = fpu_mul(ih, static_cast<float>(rows));
            dy = fpu_add(ih, fpu_div(fpu_sub(fpu_sub(ph, m2), t), static_cast<float>(rows - 1)));
        } else {
            y0 = fpu_mul(fpu_sub(ph, ih), 0.5f);
            dy = 0.0f;
        }
    }
    int32_t big = lw(w + 0x24);
    if (big < lw(w + 0x20)) big = lw(w + 0x20);
    const float px = static_cast<float>(big << 4);
    const float k = ph < pw ? fpu_div(px, pw) : fpu_div(px, ph);
    const int32_t sw_ = cvt_w_s(fpu_mul(k, iw)), sh_ = cvt_w_s(fpu_mul(k, ih));
    float y = y0;
    for (int32_t r = 0; r < lw(w + 0x40); ++r) {
        float x = x0;
        const float ny = fpu_add(y, dy);
        if (lw(w + 0x44) > 0) {
            const int32_t sy = cvt_w_s(fpu_mul(k, y));
            const int32_t sy1 = sy + sh_;
            for (int32_t c = 0; c < lw(w + 0x44); ++c, cell += 10, x = fpu_add(x, dx)) {
                const int32_t sx = cvt_w_s(fpu_mul(k, x));
                const int32_t sx1 = sx + sw_;
                if (focused && lw(w + 0x3c) == static_cast<int32_t>((cell - lwu(w + 0x48)) / 10)) {
                    const int32_t a = abs_i(static_cast<int32_t>(lwu(OV(0x0015f3f8)) & 0x3f) - 0x20);
                    const uint32_t col = (static_cast<uint32_t>(a + 0x40) * 0x10202u) | 0x80000000u;
                    gs_flat_rect(sx - 0x30, sy - 0x30, sx1 + 0x30, sy1 + 0x30, col, 1);
                    gs_flat_rect(sx - 0x10, sy - 0x10, sx1 + 0x10, sy1 + 0x10, static_cast<uint64_t>(static_cast<int64_t>(lw(OV(0x00160270)))), 1);
                }
                const int32_t kind = lh(cell + 4);
                const uint32_t own = kind == 0 ? 0x0013d4c0u : 0x0013d388u;
                if (lbu(static_cast<uint32_t>(lh(cell + 6)) + own) == 0) continue;
                int32_t add = 0;
                if (lhu(cell + 4) == 0) {
                    const int32_t item = lh(cell + 6);
                    const uint32_t slot = lwu(OV(0x00179ac8) + static_cast<uint32_t>(item * 0x4c));
                    if (lw(MS + 0x30 + slot * 4) == item) add = (lwu(w + 0x30) & 0x20) == 0 ? 1 : 0;
                    if (add == 0) add = lbu(0x0013e520 + static_cast<uint32_t>(item)) != 0 ? 4 : 0;
                    if (lw(MS + 0x134) != 0 && (lwu(w + 0x30) & 8) != 0) add = 2;
                    if (lw(MS + 0x138) != 0 && (lwu(w + 0x30) & 4) != 0) add = 2;
                }
                const int32_t f = hud_frame(lhu(cell), lh(cell + 2) + add);
                hud_sprite_xyz(f, sx, sy, sw_, sh_, 0x80);
            }
        }
        y = ny;
    }
    texture_block_end();
    return 2;
}

}  // namespace rac1
