// Oracle batch 116 (task 0346): the EE FPU's rounding toward zero, and the Vendor's closing that
// it decides. Run with build/tests/ovl_oracle --batch 116 (any level; the Vendor checks on a level
// with a Vendor: ORACLE_LEVEL=6 Nebula G34, =11 Pokitaru, where a walk past it hung the port).
//
// 1. ADD.S / SUB.S / MUL.S / MADD.S (the interpreter, on the instruction words) against the host's
//    own arithmetic in its toward-zero mode (PCSX2's "FPU.Roundmode = 3", chop): the reference the
//    interpreter is held to. Then the same instructions against the ports' fpu_add / fpu_sub /
//    fpu_mul (what the transliterations call).
// 2. The level's Vendor (class 11, Veldin's 0x2a57a8) opening for 12 frames with the hero 12 m
//    away, then closing with the hero gone: each frame of the sequence, original against port.
//    Then the property the hang broke: the closing stops at +0x90 = -4.2e-7 (1.0999998 less ten
//    steps of 0.1 toward zero), not at -0.1 (1.0000001 less eleven steps, to nearest), so the
//    child's scale (+0x2c = class scale x +0x90 x 2.5) stays a hair below zero, its radius
//    truncates to 0 and its grid rect (+0xa0) keeps x0 <= x1, y0 <= y1.

#include "ovl_oracle.h"

#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/shared_classes_tr.h>

#include <cfenv>
#include <cmath>
#include <cstring>
#include <vector>

namespace rac1::oracle {

namespace {
// a float in the normal range: exponent around 2^e0 +- spread, random mantissa and sign
float rnd_float(int e0, int spread) {
    const int e = std::clamp(e0 + ri(-spread, spread), 1, 254);
    uint32_t w = (R() & 0x807fffffu) | (static_cast<uint32_t>(e) << 23);
    float f;
    std::memcpy(&f, &w, 4);
    return f;
}
// the host's float arithmetic in its toward-zero mode (out of line, the operands and the result through volatile: the
// compiler may not move the operation out of the mode switch)
[[gnu::noinline]] float chop(int op, float a, float b, float c) {
    const int saved = std::fegetround();
    std::fesetround(FE_TOWARDZERO);
    volatile float va = a, vb = b, vc = c;
    volatile float r;
    switch (op) {
    case 0: r = va + vb; break;
    case 1: r = va - vb; break;
    case 2: r = va * vb; break;
    default: { volatile float t = va * vb; r = vc + t; break; }
    }
    std::fesetround(saved);
    return r;
}
// MIPS words: f0 = f12 <op> f13 (MADD.S: f0 = f14 + f12 * f13, ACC loaded with ADDA.S f14, f1 = 0)
void put_fpu_code(int op) {
    const uint32_t c = TEST_AREA + 0x8000;
    uint32_t words[6];
    int n = 0;
    if (op < 3) {
        words[n++] = 0x460d6000u | static_cast<uint32_t>(op);  // add.s / sub.s / mul.s $f0, $f12, $f13
    } else {
        words[n++] = 0x44800800u;  // mtc1 $zero, $f1
        words[n++] = 0x46017018u;  // adda.s $f14, $f1
        words[n++] = 0x460d601cu;  // madd.s $f0, $f12, $f13
    }
    words[n++] = 0x03e00008u;  // jr $ra
    words[n++] = 0x00000000u;  // nop
    for (int i = 0; i < n; ++i) w32(c + 4 * i, words[i]);
}
Args fpu_case(int op) {
    put_fpu_code(op);
    const int e0 = ri(100, 154);
    float a = rnd_float(e0, 6), b = rnd_float(e0, ri(0, 1) ? 3 : 30), c = rnd_float(e0, 8);
    if (ri(0, 7) == 0) { a = u2f(0x3f800001u); b = u2f(0x3dcccccdu); }  // (the Vendor's closing step)
    return Args{{}, {a, b, c}};
}
}  // namespace

void batch_116() {
    if (g_only_batch != 116) return;
    g_batch = 116;

    // ---- 1. the FPU's rounding
    static const char* const names[4] = {"fpu_add_chop", "fpu_sub_chop", "fpu_mul_chop", "fpu_madd_chop"};
    static const char* const pnames[4] = {"fpu_add_port", "fpu_sub_port", "fpu_mul_port", "fpu_madd_port"};
    for (int op = 0; op < 4; ++op) {
        std::printf("%-26s (interpreter vs host chop)\n", names[op]);
        if (!check_fn(names[op], TEST_AREA + 0x8000, 4000, Ret::Float, [op] { return fpu_case(op); },
                      [op](A a) { return rf32(chop(op, a.f[0], a.f[1], a.f[2])); })) ++g_fail;
        std::printf("%-26s (interpreter vs fpu_*)\n", pnames[op]);
        if (!check_fn(pnames[op], TEST_AREA + 0x8000, 4000, Ret::Float, [op] { return fpu_case(op); }, [op](A a) {
                const float x = a.f[0], y = a.f[1], z = a.f[2];
                switch (op) {
                case 0: return rf32(fpu_add(x, y));
                case 1: return rf32(fpu_sub(x, y));
                case 2: return rf32(fpu_mul(x, y));
                default: return rf32(fpu_add(z, fpu_mul(x, y)));
                }
            })) ++g_fail;
    }

    // ---- 2. the Vendor's opening and closing on the level's own data
    const std::vector<uint32_t> saved_ignored = g_ignored_words;
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // as in the runtime
    g_ignored_words = vcross_lanes();
    for (uint32_t a = 0; a < 0x100; a += 4) g_ignored_words.push_back(a);
    // the Vendor's callees not ported or host stand-ins (as batch 103's shared classes)
    g_extra_noop_hooks = {0x00122298, 0x00118a80, 0x00122fe0, 0x001232c8, OVF(0x002a08c8), OVF(0x0029a6b0), OVF(0x00299460), OVF(0x0023e008),
                          OVF(0x0026ced0), OVF(0x0024d430), OVF(0x00266040), OVF(0x00264740)};
    static std::vector<uint8_t> snap;
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            level_setup_body(1, 0);
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    level();
    const uint32_t fn = ovl_xlate_code(0x002a57a8);
    uint32_t vendor = 0;
    for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); m < ee_read<uint32_t>(OV(0x0015ffe0)); m += 0x100)
        if (ee_read<uint8_t>(m + 0x20) < 0xfe && ee_read<uint32_t>(m + 0x74) == fn) { vendor = m; break; }
    if (vendor == 0) {
        skip_check("vendor_open_close", 0x002a57a8, "no Vendor on this level");
    } else {
        // the hero at `dist` m from the Vendor (the same height)
        auto hero_at = [vendor](float dist) {
            Vec4 q = ee_ref<Vec4>(vendor + 0x10);
            q.x += dist;
            ee_ref<Vec4>(0x0013f3d0) = q;
        };
        // one frame of the Vendor, by the original (interpreter) or the port, the frame counter moved on
        auto frame = [vendor, fn](bool orig) {
            if (orig) call_original(fn, {vendor});
            else vendor_update_tr(vendor);
            w32(OV(0x0015f5cc), ee_read<uint32_t>(OV(0x0015f5cc)) + 1);
        };
        // The closing's next step (state 1, +0x90 above zero: +0x90 - 0.1, the decomp's `lui at,0x3dcc / ori at,at,0xcccd`,
        // in the thread's rounding) going below -1e-3: a closing one step too far, which the PS2 never takes (rounded to
        // nearest, RAC1_FPU_NEAREST=1: 1.49e-8 - 0.1). Not run: the orb's grid rect may then turn inside out and neither the
        // original's interpreter nor the port return (the hang).
        auto overshoots = [vendor] {
            const float v = ee_read<float>(ee_read<uint32_t>(vendor + 0x78) + 0x90);
            return ee_read<uint8_t>(vendor + 0x20) == 1 && v > 0.0f && fpu_sub(v, u2f(0x3dcccccdu)) < -1e-3f;
        };
        // opened (state 2 from +0x90 = 0, 12 frames with the hero 12 m away), then up to `k` frames closing (hero 200 m away);
        // false when it stopped before an overshooting step
        auto sequence = [vendor, level, hero_at, frame, overshoots](int k, bool orig) {
            level();
            w32(OV(0x0015f5cc), 1);
            w8(vendor + 0x20, 2);
            w32(ee_read<uint32_t>(vendor + 0x78) + 0x90, 0);
            hero_at(12.0f);
            for (int i = 0; i < 12; ++i) frame(orig);
            hero_at(200.0f);
            for (int i = 0; i < k; ++i) {
                if (overshoots()) return false;
                frame(orig);
            }
            return !overshoots();
        };

        // the end of the closing, by the original and by the port: +0x90 and the child's rect
        bool closes = true;
        for (int side = 0; side < 2; ++side) {
            const char* name = side == 0 ? "vendor_closed_orig" : "vendor_closed_port";
            const bool stopped = !sequence(40, side == 0);
            const uint32_t pv = ee_read<uint32_t>(vendor + 0x78), child = ee_read<uint32_t>(pv + 0xc);
            const uint32_t v90 = ee_read<uint32_t>(pv + 0x90), rect = ee_read<uint32_t>(child + 0xa0);
            const bool ordered = static_cast<int32_t>(rect) < 0 || ((rect & 0xff) <= ((rect >> 16) & 0xff) && ((rect >> 8) & 0xff) <= (rect >> 24));
            const bool ok = !stopped && v90 == 0xb4e00000u && ordered;  // -4.17232513e-07: 1.0999998 (0x3f8ccccb) less eleven steps of 0.1 toward zero
            std::printf("%-26s 0x%08x  %s (+0x90 %08x%s, the child's scale %08x, rect %08x)\n", name, 0x002a57a8u, ok ? "ok" : "FAIL", v90,
                        stopped ? " and the next step overshoots" : "", ee_read<uint32_t>(child + 0x2c), rect);
            if (!ok) ++g_fail;
            closes = closes && ok;
        }

        // each frame of the opening and the closing, original against port
        std::printf("%-26s 0x%08x\n", "vendor_open_close", 0x002a57a8u);
        if (!closes) {
            std::printf("%-26s 0x%08x  FAIL (not run: the closing overshoots)\n", "vendor_open_close", 0x002a57a8u);
            ++g_fail;
        } else if (!check_fn("vendor_open_close", fn, 40, Ret::None, [vendor, sequence] {
                       sequence(ri(0, 30), true);
                       return Args{{vendor}, {}};
                   }, [](A a) { vendor_update_tr(a.i[0]); return uint64_t{0}; }, Scope::FullRam)) {
            ++g_fail;
        }
    }
    g_extra_noop_hooks.clear();
    g_ignored_words = saved_ignored;
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
