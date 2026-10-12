// Ported overlay code: Novalis's scrolling surfaces and its water (see novalis_water.h): the draw callbacks the
// classes queue and the two VU1 set-ups, by hand. The strips / water grid themselves: novalis_water_asm.cpp (transliterated).
// Host: each callback's packets noted for the Modern Rendering Pipeline (mrp_fx.h: VU1 program 7's prim lists, drawn by the MRP
// in the world's projection).
#include <game/ovl/novalis_water.h>
#include <game/ovl/novalis_draw_cbs.h>

#include <game/math/vecmath.h>
#include <game/ovl/novalis_mobys.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/shadows.h>
#include <game/ovl/variants_l1.h>
#include <game/render/mrp_fx.h>

namespace rac1 {

namespace {
constexpr uint32_t CUR = L1_ADDR(0x001611c0u);  // the packet's cursor (gp - 0x5a40)
}  // namespace

// orig: L1:0x002b96c0
void nov_strips_vu7_setup_l1() { shadows_vu1_setup(); }

// orig: L1:0x002b8f98
void nov_water_vu7_setup_l1() {
    const uint32_t sp = ee_stack_alloc(0x70);
    const uint32_t s1 = L1_ADDR(0x00167240u);  // the camera's position
    mat4_scale_diag(M(sp), 1024.0f);
    vec3_scale(V(sp + 0x30), V(s1), -1024.0f);
    swc1(sp + 0x3c, 1.0f);
    if (lw(L1_ADDR(0x0015f5e0u)) != 7) {
        packet_ref(0x0010e810, lhu(0x0010e800));  // the VU1 program
        sw(L1_ADDR(0x0015f5e0u), 7);
    }
    sw(lwu(CUR), 0x10000000u);
    sw(lwu(CUR) + 4, 0);
    sw(lwu(CUR) + 8, 0x11000000u);    // FLUSH
    sw(lwu(CUR) + 0xc, 0x01000404u);  // STCYCL 4, 4
    const uint32_t s0 = lwu(CUR);
    sw(s0 + 0x10, 0);
    sw(s0 + 0x14, 0);
    sw(s0 + 0x18, 0);
    sw(s0 + 0x1c, 0x6c0c43a4u);  // UNPACK V4-32, 12 quadwords to 0x3a4
    mat4_mul(M(s0 + 0x20), M(s1 - 0x100), M(sp));
    mat4_mul(M(s0 + 0x60), M(s1 - 0x80), M(sp));
    const uint32_t v1 = L1_ADDR(0x0016cec0u);
    sw(s0 + 0xa8, 0x412);
    sw(s0 + 0xa0, 0x8000);  // the clipped fans' GIF tag (vf31), w the screen scale
    sw(s0 + 0xa4, 0x303ec000u);
    swc1(s0 + 0xac, lwc1(v1 + 0x210));
    lq_sq(s0 + 0xb0, v1 + 0x190);  // vf29: the screen scale
    lq_sq(s0 + 0xc0, v1 + 0x1a0);  // vf30: the screen offset
    swc1(s0 + 0xd0, lwc1(v1 + 0x22c));
    sw(s0 + 0xe4, 0x020001d2u);  // OFFSET 0x1d2
    swc1(s0 + 0xd4, lwc1(v1 + 0x228));
    sw(s0 + 0xe8, 0x15000000u);  // MSCALF 0: the start-up
    sw(s0 + 0xe0, 0x03000000u);  // BASE 0
    sw(s0 + 0xd8, 0);
    sw(s0 + 0xdc, 0);
    sw(s0 + 0xec, 0);
    const uint32_t a2 = s0 + 0xf0;
    const uint32_t a0 = lwu(CUR);
    sw(a0, lwu(a0) | static_cast<uint32_t>((static_cast<int32_t>(a2 - a0) >> 4) - 1));
    sw(CUR, a2);
    mrp_fx_note_vu7(s0 + 0x20);  // (host: the prim lists' projection, mrp_fx.h)
    packet_ref_13cf10();
    // a REF of the 7-quadword GS packet 0x1cac80 (DIRECT 7); the GS state cache (gp - 0x59d8 .. - 0x59cc) forgotten
    sw(lwu(CUR), 0x30000007u);
    sw(lwu(CUR) + 4, L1_ADDR(0x001cac80u));
    sw(lwu(CUR) + 8, 0);
    sw(lwu(CUR) + 0xc, 0x50000007u);
    sw(L1_ADDR(0x00161234u), 0xffffffffu);
    sw(L1_ADDR(0x00161228u), 0xffffffffu);
    sw(L1_ADDR(0x0016122cu), 0xffffffffu);
    sw(CUR, lwu(CUR) + 0x10);
    sw(L1_ADDR(0x00161230u), 0xffffffffu);
    ee_stack_free(0x70);
}

// The draw callbacks: VU1 program 7 set up, then the class's strips (the moby is not read).
// orig: L1:0x002f60f8
void nov_strips_676_draw_l1(uint32_t) {
    const MrpFxScope fx(kMrpFxPrimList);  // (host: drawn by the MRP, mrp_fx.h)
    nov_strips_vu7_setup_l1();
    nov_strips_draw_l1(8, L1_ADDR(0x001e2fc0u));
}

// orig: L1:0x002f6150
void nov_strips_678_draw_l1(uint32_t) {
    const MrpFxScope fx(kMrpFxPrimList);  // (host)
    nov_strips_vu7_setup_l1();
    nov_strips_draw_l1(2, L1_ADDR(0x001e3380u));
}

// orig: L1:0x002feb28
void nov_strips_761_draw_l1(uint32_t) {
    const MrpFxScope fx(kMrpFxPrimList);  // (host)
    nov_strips_vu7_setup_l1();
    nov_strips_draw_l1(4, L1_ADDR(0x001fbc80u));
}

// Class 1225's: its three strips drawn, then their wave height (+0x5c) set for the next frame to 0.5 + 0.5 sin((frame & 63) - 32)
// x pi / 32) (the frame counter 0x15f5cc).
// orig: L1:0x00309bf8
void nov_strips_1225_draw_l1(uint32_t) {
    {
        const MrpFxScope fx(kMrpFxPrimList);  // (host)
        nov_strips_vu7_setup_l1();
        nov_strips_draw_l1(3, L1_ADDR(0x00202d80u));
    }
    const float f = fpu_sub(int_to_float(static_cast<int32_t>(lw(L1_ADDR(0x0015f5ccu)) & 0x3f)), u2f(0x42000000u));
    const float s = sin_vu(fpu_mul(f, u2f(0x3dc90fdbu)));
    const float h = fpu_add(fpu_mul(s, u2f(0x3f000000u)), u2f(0x3f000000u));
    uint32_t p = L1_ADDR(0x00202d80u) + 0x11c;
    for (int32_t i = 2; i >= 0; --i, p -= 0x60) swc1(p, h);
}

// Class 751's: the 21 water tiles at 0x1e34c0.
// orig: L1:0x002fd0c0
void nov_water_751_draw_l1(uint32_t) {
    const MrpFxScope fx(kMrpFxPrimList);  // (host)
    nov_water_draw_l1(L1_ADDR(0x001e34c0u), 0x15);
}

// orig: (none) — registers the draw callbacks for the indirect calls (ovl_call_moby_fn)
void novalis_water_fns_register() {
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f60f8u), nov_strips_676_draw_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f6150u), nov_strips_678_draw_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002feb28u), nov_strips_761_draw_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x00309bf8u), nov_strips_1225_draw_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002fd0c0u), nov_water_751_draw_l1);
    novalis_draw_cbs_register();  // (the other draw callbacks, novalis_draw_cbs.cpp)
}

}  // namespace rac1
