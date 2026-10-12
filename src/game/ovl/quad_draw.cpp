// Ported overlay code: the quad drawer (see quad_draw.h).
#include <game/ovl/quad_draw.h>

#include <game/ovl/ovl_reloc.h>
#include <algorithm>
#include <cmath>
#include <utility>

#include <game/math/vecmath.h>
#include <game/render/mrp_fx.h>
#include <game/ovl/vu0_macro.h>

namespace rac1 {

using namespace vu0m;

// orig: 0x001fd85c
uint32_t packet_direct_begin(uint32_t qwc) {
    const uint32_t at = lwu(OV(0x001611c0));
    const uint32_t tag = 0x10000000u | qwc;
    sd(at, 0);
    sd(at + 8, 0);
    sw(at, tag);
    sw(at + 0xc, tag | 0x40000000u);
    sw(OV(0x001611c0), at + (qwc << 4) + 0x10);
    return at + 0x10;
}

// orig: 0x001fd7dc
void quad_gs_state(uint32_t desc) {
    const uint32_t at = packet_direct_begin(5);
    sd(at + 0x10, ld(desc + 0x70));
    sd(at + 0x18, ld(desc + 0x78));
    sd(at, 0x2400000000000001ull);
    sd(at + 8, 0x68);
    sd(at + 0x30, ld(desc + 0x80));
    sd(at + 0x38, 0x14);
    sd(at + 0x20, 0x2000000000008001ull);
    sd(at + 0x28, 0xee);
    sd(at + 0x40, ld(desc + 0x88));
    sd(at + 0x48, 0x42);
}

namespace {
// orig: (helper) repeated in 0x001fd228 — a vertex's XYZF2 (FTOI4 lanes): x, y low halves, z >> 4,
// the fog byte (w >> 4) clamped to 0..255 over z's top byte
void xyzf2(uint32_t a, const Vf& p) {
    const uint32_t x = f2u(p.v[0]), y = f2u(p.v[1]);
    const int32_t z = static_cast<int32_t>(f2u(p.v[2])) >> 4, w = static_cast<int32_t>(f2u(p.v[3])) >> 4;
    sw(a, (x & 0xffff) | (y << 16));
    sw(a + 4, static_cast<uint32_t>(z));
    sb(a + 7, static_cast<uint32_t>(w < 0 ? 0 : w > 0xff ? 0xff : w));
}
// orig: (helper) repeated in 0x001fd228 — a lane pair as a 64-bit word (sd of a QMFC2 half)
uint64_t pair(float a, float b) { return static_cast<uint64_t>(f2u(a)) | (static_cast<uint64_t>(f2u(b)) << 32); }
// orig: (helper) repeated in 0x001fd228 — FTOI4 / FTOI0 of all four lanes
Vf ftoiv(const Vf& p, float scale) {
    Vf o;
    for (int i = 0; i < 4; ++i) o.v[i] = bits(ftoi(p.v[i], scale));
    return o;
}
}  // namespace


namespace {
// orig: (helper) the colour word's four bytes as floats (PEXTLB / PEXTLH with zero, VITOF0)
Vf colour_floats(uint32_t c) {
    Vf o;
    for (int i = 0; i < 4; ++i) o.v[i] = static_cast<float>((c >> (8 * i)) & 0xff);
    return o;
}
// orig: (helper) FTOI4 lanes back to a colour word: PPACH, PADDH 2, PSRLH 4, PPACB
uint32_t colour_word(const Vf& f) {
    uint32_t w = 0;
    for (int i = 0; i < 4; ++i) {
        const uint32_t h = (ftoi(f.v[i], 16.0f) + 2u) & 0xffffu;
        w |= ((h >> 4) & 0xffu) << (8 * i);
    }
    return w;
}
// orig: (helper) VMINI / VMAX of one lane (the VU's integer compare of float bits)
float fp_min1(float a, float b) {
    const int32_t x = static_cast<int32_t>(f2u(a)), y = static_cast<int32_t>(f2u(b));
    return u2f(static_cast<uint32_t>((x < 0 && y < 0) ? std::max(x, y) : std::min(x, y)));
}
// orig: (helper) VMAX of one lane
float fp_max1(float a, float b) {
    const int32_t x = static_cast<int32_t>(f2u(a)), y = static_cast<int32_t>(f2u(b));
    return u2f(static_cast<uint32_t>((x < 0 && y < 0) ? std::min(x, y) : std::max(x, y)));
}
// orig: (helper) a 64-bit register of two float lanes (lo, hi) as the branch tests see it
bool gtz64(float lo, float hi) { return (f2u(hi) & 0x80000000u) == 0 && (f2u(lo) | f2u(hi)) != 0; }
bool ltz64(float hi) { return (f2u(hi) & 0x80000000u) != 0; }
bool lez64(float lo, float hi) { return !gtz64(lo, hi); }

// The vertex where the edge a -> b crosses the plane `plane` (0 +x, 1 -x, 2 +y, 3 -y, 4 +z,
// 5 -z: coordinate = w), its uv and colour interpolated, at `out` (+0x1c left as it was).
// Vertex records: +0 the position, +0x10 (s, t), +0x18 the colour word.
// orig: 0x001fcf90
void clip_edge_vertex(uint32_t out, uint32_t a, uint32_t b, uint32_t plane) {
    const uint32_t off = (plane >> 1) << 2, sign = plane << 31;
    const float aw = lwc1(a + 0xc), bw = lwc1(b + 0xc);
    const float ac = u2f(lwu(a + off) ^ sign), bc = u2f(lwu(b + off) ^ sign);
    const float as = lwc1(a + 0x10), at = lwc1(a + 0x14);
    const float den = fpu_add(fpu_sub(bw, aw), fpu_sub(ac, bc));
    const float t = fpu_div(fpu_sub(ac, aw), den);
    const Vf ca = colour_floats(lwu(a + 0x18)), cb = colour_floats(lwu(b + 0x18));
    const Vf pa = lqc2(a), pb = lqc2(b);
    Vf col, pos;
    for (int i = 0; i < 4; ++i) {
        col.v[i] = msub(madd(mul(ca.v[i], 1.0f), cb.v[i], t), ca.v[i], t);
        pos.v[i] = msub(madd(mul(pa.v[i], 1.0f), pb.v[i], t), pa.v[i], t);
    }
    swc1(out + 0x10, fpu_add(fpu_mul(fpu_sub(lwc1(b + 0x10), as), t), as));
    swc1(out + 0x14, fpu_add(fpu_mul(fpu_sub(lwc1(b + 0x14), at), t), at));
    sqc2(out, pos);
    sw(out + 0x18, colour_word(col));
}

// The vertex at t along a -> b (position, uv, colour; +0x1c from a) at `out`.
// orig: 0x001fd108
void lerp_vertex(uint32_t out, uint32_t a, uint32_t b, float t) {
    const Vf ca = colour_floats(lwu(a + 0x18)), cb = colour_floats(lwu(b + 0x18));
    const float u = sub(1.0f, t);
    const Vf pa = lqc2(a), pb = lqc2(b);
    Vf col, pos;
    for (int i = 0; i < 4; ++i) {
        col.v[i] = madd(mul(ca.v[i], u), cb.v[i], t);
        pos.v[i] = madd(mul(pa.v[i], u), pb.v[i], t);
    }
    Vf uv = lqc2(a + 0x10);
    const Vf uvb = lqc2(b + 0x10);
    for (int i = 0; i < 2; ++i) uv.v[i] = madd(mul(uv.v[i], u), uvb.v[i], t);
    sqc2(out + 0x10, uv);
    sqc2(out, pos);
    sw(out + 0x18, colour_word(col));
}

// The point of the segment a -> b nearest to c (its parameter clamped to 0..1) at `out`; the
// unclamped parameter returned (also left in vf8.x).
// orig: 0x001fd1c8
float closest_on_segment(uint32_t out, uint32_t a, uint32_t b, uint32_t c) {
    const Vf p1 = lqc2(a), p2 = lqc2(b), p3 = lqc2(c);
    Vf d1, d2;
    for (int i = 0; i < 3; ++i) {
        d1.v[i] = sub(p2.v[i], p1.v[i]);
        d2.v[i] = sub(p3.v[i], p1.v[i]);
    }
    const float l2 = madd(add(mul(d1.v[1], d1.v[1]), mul(d1.v[0], d1.v[0])), 1.0f, mul(d1.v[2], d1.v[2]));
    const float dp = madd(add(mul(d1.v[1], d2.v[1]), mul(d1.v[0], d2.v[0])), 1.0f, mul(d1.v[2], d2.v[2]));
    const float q = div(dp, l2);
    const float t = add(0.0f, q);
    const float tc = fp_max1(fp_min1(t, 1.0f), 0.0f);
    Vf r = p1;
    for (int i = 0; i < 3; ++i) r.v[i] = madd(mul(d1.v[i], tc), p1.v[i], 1.0f);
    sqc2(out, r);
    return t;
}

// orig: (helper) VSQRT
float vsqrt(float x) { return vuc(std::sqrt(std::fabs(vuc(x)))); }

// A triangle staged at t8 (three vertex records and the first again) clipped against the six
// planes of the guard band (Sutherland-Hodgman between the two scratchpad buffers t8 / t8_other,
// which swap at each plane); a polygon of more than 3 corners started at the corner nearest
// the quad's first corner `corner` (an extra corner inserted on its edges when the nearest
// point of an edge lies inside it), so the strip fans from it; drawn as a GIF triangle strip
// (REGLIST PRIM 0x7d, then ST, RGBAQ, XYZF2 per corner). The rotations at 0x1fdca4 / 0x1fdcc4
// need the corner found nearest not to be (unreached in the oracle's cases; ported as written,
// including their count register left from the second nearest-point call).
// orig: 0x001fd89c
void tri_clip_draw(uint32_t& t8, uint32_t& t8_other, const Vf& corner) {
    const Vf vf27 = lqc2(OV(0x0016cbd0)), vf28 = lqc2(OV(0x0016cbe0)), vf29 = lqc2(OV(0x0016cbf0)), vf30 = lqc2(OV(0x00160be0)), vf31 = lqc2(OV(0x00160bf0));
    uint32_t n = 3;
    for (int32_t plane = 5;; --plane) {
        uint32_t out = t8_other;
        const uint32_t base = out;
        for (uint32_t i = 0; i < n; ++i) {
            const uint32_t a = t8 + (i << 5), b = t8 + ((i + 1) << 5);
            const Vf p1 = lqc2(a), p3 = lqc2(b);
            uint32_t clip = 0;
            clipw(clip, p1, p1.v[3]);
            clipw(clip, p3, p3.v[3]);
            sqc2(out, p1);
            lq_sq(out + 0x10, a + 0x10);
            const uint32_t c = clip >> plane;
            const uint32_t in1 = c & 0x40;
            if (in1 == 0) out += 0x20;
            if (((in1 ^ (c << 6)) & 0x40) != 0) {
                clip_edge_vertex(out, a, b, static_cast<uint32_t>(plane));
                out += 0x20;
            }
        }
        std::swap(t8, t8_other);
        lq_sq(out, t8);
        lq_sq(out + 0x10, t8 + 0x10);
        n = (out - base) >> 5;
        if (n == 0) return;
        if (plane == 0) break;
    }
    if (static_cast<int32_t>(n) - 3 > 0) {
        // the corners scaled by vf29 (kept at 0x70001ea0), the one nearest the quad's corner
        constexpr uint32_t T = 0x70001ea0;
        Vf d[10];
        for (uint32_t k = 1; k <= n; ++k) {
            Vf p = lqc2(t8 + ((k - 1) << 5));
            for (int i = 0; i < 3; ++i) p.v[i] = mul(p.v[i], vf29.v[i]);
            sqc2(T + ((k - 1) << 5), p);
            d[k] = p;
        }
        sqc2(T + (n << 5), d[1]);
        for (uint32_t k = 1; k <= n; ++k) {
            for (int i = 0; i < 3; ++i) {
                d[k].v[i] = sub(d[k].v[i], corner.v[i]);
                d[k].v[i] = mul(d[k].v[i], d[k].v[i]);
            }
            d[k].v[1] = madd(add(d[k].v[1], d[k].v[0]), 1.0f, d[k].v[2]);
        }
        uint32_t best = 0;
        float bestd = d[1].v[1];
        for (uint32_t k = n; k >= 2; --k)
            if ((f2u(sub(d[k].v[1], bestd)) & 0x80000000u) != 0) {
                best = k - 1;
                bestd = d[k].v[1];
            }
        const uint32_t A = T + (best << 5);
        sqc2(T - 0x20, corner);
        const float t1 = closest_on_segment(0x70001fe0, A, A + 0x20, T - 0x20);
        const uint32_t prev = best != 0 ? A - 0x20 : T + ((n - 1) << 5);
        const float t2 = closest_on_segment(0x70001ff0, A, prev, T - 0x20);
        // the distances from the quad's corner to the two nearest edge points
        auto dist = [&corner](uint32_t a) {
            Vf v = lqc2(a);
            for (int i = 0; i < 3; ++i) {
                v.v[i] = sub(v.v[i], corner.v[i]);
                v.v[i] = mul(v.v[i], v.v[i]);
            }
            return madd(add(v.v[0], v.v[1]), 1.0f, v.v[2]);
        };
        const float s1 = dist(0x70001fe0);
        const float d1 = add(0.0f, vsqrt(s1));
        const float s2 = dist(0x70001ff0);
        const float d2 = vsqrt(s2);
        const float e = vf30.v[0];
        const float t4x = sub(t2, e), t4y = sub(t1, e);  // t4
        const float t3x = sub(t2, 1.0f), t3y = sub(t1, 1.0f);  // t3
        const float t5x = s2, t5y = sub(d1, d2);  // t5: vf22 (x its squared sum)
        uint32_t a0 = 0x70001ff0, a1 = A, a2 = prev;
        auto rotate = [&](uint32_t dst, uint32_t start, uint32_t count) {
            uint32_t src = (start << 5) + t8, left = count;
            for (;;) {
                lq_sq(dst, src);
                lq_sq(dst + 0x10, src + 0x10);
                src += 0x20;
                dst += 0x20;
                --left;
                if (left == 0) {
                    std::swap(t8, t8_other);
                    return;
                }
                if (left == start) src = t8;
            }
        };
        bool done = false;
        if (!gtz64(t4x, t4y)) {
            a2 = n;
            a0 = t8_other;
            if (!gtz64(0.0f, t4x)) {
                a1 = best;
                rotate(a0, a1, a2);
                done = true;
            }
        }
        if (!done) {  // 0x1fdc94
            a1 = best + 1;
            bool to_cb4 = gtz64(t5x, t5y);
            if (!to_cb4) {
                a0 = t8_other;
                if (ltz64(t3y)) to_cb4 = true;
                else {
                    a2 = n;
                    if (a1 == n) a1 = 0;
                    rotate(a0, a1, a2);
                    done = true;
                }
            }
            if (!done && to_cb4) {  // 0x1fdcb4
                bool to_cdc = ltz64(t5y);
                if (!to_cdc) {
                    a0 = t8_other;
                    if (ltz64(t3x)) to_cdc = true;
                    else {
                        a1 = best - 1;
                        if (best == 0) {
                            a2 = n;
                            a1 = n - 1;
                        }
                        rotate(a0, a1, a2);
                        done = true;
                    }
                }
                if (!done) {  // 0x1fdcdc
                    a0 = t8_other;
                    bool to_d1c = gtz64(t5x, t5y);
                    a1 = best << 5;
                    if (!to_d1c && lez64(t2, t1)) to_d1c = true;
                    if (!to_d1c) {  // an extra corner on the edge to the next one
                        a1 += t8;
                        lerp_vertex(a0, a1, a1 + 0x20, t1);
                        a0 += 0x20;
                        a1 = best + 1;
                        if (a1 == n) a1 = 0;
                        a2 = n;
                        ++n;
                        rotate(a0, a1, a2);
                        done = true;
                    } else {  // 0x1fdd1c
                        a0 = t8_other;
                        a1 = best << 5;
                        if (!ltz64(t5y) && !lez64(0.0f, t2)) {  // an extra corner on the edge to the previous one
                            a1 += t8;
                            a2 = best != 0 ? a1 - 0x20 : ((n - 1) << 5) + t8;
                            lerp_vertex(a0, a1, a2, t2);
                            ++n;
                            a2 = n - 1;
                            a0 += 0x20;
                            a1 = best;
                            rotate(a0, a1, a2);
                        }
                        done = true;
                    }
                }
            }
        }
    }
    // the strip
    const uint32_t at0 = packet_direct_begin(((n * 3) >> 1) + 3);
    sw(at0 + 4, 0x44000000u);
    sw(at0, 1);
    sd(at0 + 8, 0x4120);
    sd(at0 + 0x10, 0x7d);
    sd(at0 + 0x38, 0x412);
    sw(at0 + 0x34, 0x34000000u);
    sw(at0 + 0x30, (n | 0x8000u) - 1);
    uint32_t at = at0 + 0x18, step = 0x28;
    uint32_t r = t8;
    for (uint32_t left = n; left != 0; --left) {
        Vf p = lqc2(r);
        for (int i = 0; i < 4; ++i) p.v[i] = mul(p.v[i], vf27.v[i]);
        const float q = div(vf31.v[0], p.v[3]);
        Vf uv = lqc2(r + 0x10);
        uv.v[3] = mul(1.0f, 1.0f);
        r += 0x20;
        for (int i = 0; i < 4; ++i) p.v[i] = add(mul(p.v[i], q), vf28.v[i]);
        uv.v[0] = mul(uv.v[0], q);
        uv.v[1] = mul(uv.v[1], q);
        uv.v[3] = mul(uv.v[3], q);
        const Vf sp = ftoiv(p, 16.0f);
        sd(at, pair(uv.v[0], uv.v[1]));
        sd(at + 8, pair(uv.v[2], uv.v[3]));
        xyzf2(at + 0x10, sp);
        at += step;
        step = 0x18;
    }
}
}  // namespace

// orig: 0x001fd228
void quad_draw(uint32_t desc, uint32_t matrix) {
    const MrpFxScope fx(kMrpFxQuad, desc, matrix);  // (host: its packets drawn by the MRP, mrp_fx.h)
    Vf q[4] = {lqc2(desc), lqc2(desc + 0x10), lqc2(desc + 0x20), lqc2(desc + 0x30)};
    const Vf cam = lqc2(OV(0x00166dc0));
    const Vf vf30 = lqc2(OV(0x00160be0));
    if (matrix != 0) {
        const Vf m[4] = {lqc2(matrix), lqc2(matrix + 0x10), lqc2(matrix + 0x20), lqc2(matrix + 0x30)};
        for (Vf& p : q) p = transform(m, p.v[0], p.v[1], p.v[2], p.v[3]);
    }
    for (Vf& p : q)
        for (int i = 0; i < 3; ++i) p.v[i] = mul(sub(p.v[i], cam.v[i]), vf30.v[2]);
    const Vf rows[4] = {lqc2(OV(0x00166cc0)), lqc2(OV(0x00166cd0)), lqc2(OV(0x00166ce0)), lqc2(OV(0x00166cf0))};
    const Vf vf26 = lqc2(OV(0x0016cbc0));
    Vf c[4];
    for (int k = 3; k >= 0; --k) c[k] = transform(rows, q[k].v[0], q[k].v[1], q[k].v[2], 1.0f);
    const Vf vf19 = {{mul(vf26.v[0], vf30.v[1]), mul(vf26.v[1], vf30.v[1]), vf26.v[2], vf26.v[3]}};
    Vf g[4];
    for (int k = 0; k < 4; ++k)
        for (int i = 0; i < 4; ++i) g[k].v[i] = mul(c[k].v[i], vf19.v[i]);
    uint32_t clip = 0;
    for (int k = 3; k >= 0; --k) clipw(clip, g[k], g[k].v[3]);
    // trivially rejected: all four out of one side (a corner behind the eye counts as out
    // left / right and up / down when it has no such bit; the first corner's test is a no-op)
    {
        uint32_t t9 = clip;
        uint32_t t7 = t9 & 0x3f;
        t9 >>= 6;
        bool draw = t7 == 0;
        for (int k = 1; k < 4 && !draw; ++k) {
            if ((f2u(g[k].v[3]) & 0x80000000u) != 0) {
                if ((t9 & 3) == 0) t9 |= 3;
                if ((t9 & 0xc) == 0) t9 |= 0xc;
            }
            t7 &= t9;
            t9 >>= 6;
            draw = t7 == 0;
        }
        if (!draw) return;
    }
    Vf s[4];
    for (int i = 0; i < 4; ++i) s[0].v[i] = mul(c[0].v[i], vf26.v[i]);
    Vf vf1 = rows[0];
    vf1.v[2] = madd(madd(add(s[0].v[2], g[1].v[2]), g[2].v[2], 1.0f), g[3].v[2], 1.0f);
    for (int k = 1; k < 4; ++k)
        for (int i = 0; i < 4; ++i) s[k].v[i] = mul(c[k].v[i], vf26.v[i]);
    vf1 = ftoiv(vf1, 1.0f);
    clip = 0;
    for (int k = 0; k < 4; ++k) clipw(clip, s[k], s[k].v[3]);
    const Vf vf31 = lqc2(OV(0x00160bf0));
    const Vf vf28 = lqc2(OV(0x0016cbe0));
    if (clip != 0) {  // partly off screen: the two triangles through the clipper
        const Vf vf29 = lqc2(OV(0x0016cbf0));
        quad_gs_state(desc);  // (its $s1 test reads the register 0x1fd2cc cleared: always first)
        uint32_t t8 = 0x70001c40, t8_other = 0x70001c40 + 0x120;
        const uint64_t rgba01 = ld(desc + 0x40), rgba23 = ld(desc + 0x48);
        const uint64_t uv0 = ld(desc + 0x50), uv1 = ld(desc + 0x58), uv2 = ld(desc + 0x60), uv3 = ld(desc + 0x68);
        const uint64_t c0 = rgba01 & 0xffffffffu, c1 = rgba01 >> 32, c2 = rgba23 & 0xffffffffu, c3 = rgba23 >> 32;
        auto vert = [](uint32_t a, const Vf& p, uint64_t uv, uint64_t col) {
            sqc2(a, p);
            sd(a + 0x10, uv);
            sd(a + 0x18, col);
        };
        vert(t8, s[0], uv0, c0);
        vert(t8 + 0x60, s[0], uv0, c0);
        vert(t8 + 0x20, s[1], uv1, c1);
        vert(t8 + 0x40, s[2], uv2, c2);
        Vf corner;
        for (int i = 0; i < 4; ++i) corner.v[i] = mul(s[0].v[i], vf29.v[i]);
        tri_clip_draw(t8, t8_other, corner);
        vert(t8, s[1], uv1, c1);
        vert(t8 + 0x60, s[1], uv1, c1);
        vert(t8 + 0x20, s[2], uv2, c2);
        vert(t8 + 0x40, s[3], uv3, c3);
        for (int i = 0; i < 4; ++i) corner.v[i] = mul(s[1].v[i], vf29.v[i]);
        tri_clip_draw(t8, t8_other, corner);
        return;
    }
    // on screen: A+D TEX1 / ALPHA, then a REGLIST of CLAMP, TEX0, PRIM 0x7c and per corner ST,
    // RGBAQ, XYZF2
    const uint32_t at = packet_direct_begin(0xc);
    Vf vf14 = lqc2(desc + 0x50);
    float Q = div(vf31.v[0], c[0].v[3]);
    Vf p[4];
    float qs[4];
    for (int k = 0; k < 4; ++k) {  // screen = vf28 + clip x Q, FTOI4
        if (k > 0) Q = div(vf31.v[0], c[k].v[3]);
        qs[k] = mul(1.0f, Q);
        for (int i = 0; i < 4; ++i) p[k].v[i] = madd(mul(vf28.v[i], 1.0f), c[k].v[i], Q);
        p[k] = ftoiv(p[k], 16.0f);
        if (k == 0) {
            vf14.v[0] = mul(vf14.v[0], Q);
            vf14.v[1] = mul(vf14.v[1], Q);
        } else if (k == 1) {
            vf14.v[2] = mul(vf14.v[2], Q);
            vf14.v[3] = mul(vf14.v[3], Q);
        }
    }
    Vf vf15 = lqc2(desc + 0x60);
    vf15.v[0] = mul(vf15.v[0], qs[2]);
    vf15.v[1] = mul(vf15.v[1], qs[2]);
    vf15.v[2] = mul(vf15.v[2], qs[3]);
    vf15.v[3] = mul(vf15.v[3], qs[3]);
    sd(at + 0x00, 0x2000000000000001ull);
    sd(at + 0x08, 0xee);
    sd(at + 0x10, ld(desc + 0x80));
    sd(at + 0x18, 0x14);
    sd(at + 0x20, ld(desc + 0x88));
    sd(at + 0x28, 0x42);
    sd(at + 0x30, 0xf400000000008001ull);
    sd(at + 0x38, 0x0412412412412068ull);
    sd(at + 0x40, ld(desc + 0x70));
    sd(at + 0x48, ld(desc + 0x78));
    sd(at + 0x50, 0x7c);
    sd(at + 0x58, pair(vf14.v[0], vf14.v[1]));
    sd(at + 0x70, pair(vf14.v[2], vf14.v[3]));
    sd(at + 0x88, pair(vf15.v[0], vf15.v[1]));
    sd(at + 0xa0, pair(vf15.v[2], vf15.v[3]));
    xyzf2(at + 0x68, p[0]);
    xyzf2(at + 0x80, p[1]);
    xyzf2(at + 0x98, p[2]);
    xyzf2(at + 0xb0, p[3]);
    sd(at + 0xb8, 0);
    for (uint32_t k = 0; k < 4; ++k)
        sd(at + 0x60 + 0x18 * k, static_cast<uint64_t>(lwu(desc + 0x40 + 4 * k)) | (static_cast<uint64_t>(f2u(qs[k])) << 32));
}

// orig: 0x001f9660
void decals_draw() {
    const uint32_t d = ee_stack_alloc(0x90);
    const uint64_t tex0 = effect_tex0(0);
    sd(d + 0x78, tex0);
    sd(d + 0x80, 0x0000ff9000000260ull);
    sd(d + 0x70, 5);
    sd(d + 0x88, 0x8000000044ull);
    for (uint32_t k = 0; k < 4; ++k) {
        swc1(d + 0x50 + 8 * k, lwc1(OV(0x0016c7e0) + 0x10 * k + 8));
        swc1(d + 0x54 + 8 * k, lwc1(OV(0x0016c7e0) + 0x10 * k + 0xc));
        sw(d + 0x40 + 4 * k, 0x40808080);
    }
    int32_t i = 0;
    if (lw(OV(0x0015f434)) > 0) {
        do {
            Vec4 n = V(OV(0x0016e090) + static_cast<uint32_t>(i) * 0x20);
            vec3_set_length(n, n, 1.0f);
            ++i;
            const uint32_t e = OV(0x0016e080) + static_cast<uint32_t>(i - 1) * 0x20;
            const float size = lwc1(e + 0xc);
            for (uint32_t k = 0; k < 4; ++k) {  // the corner moved along the plane: (o - n (o . n)) x size
                const float ox = lwc1(OV(0x0016c7e0) + 0x10 * k), oy = lwc1(OV(0x0016c7e0) + 0x10 * k + 4);
                const float dn = fpu_add(fpu_mul(ox, n.x), fpu_mul(oy, n.y));
                const uint32_t c = d + 0x10 * k;
                lq_sq(c, e);
                swc1(c, fpu_add(lwc1(c), fpu_mul(fpu_sub(ox, fpu_mul(n.x, dn)), size)));
                swc1(c + 4, fpu_add(lwc1(c + 4), fpu_mul(fpu_sub(oy, fpu_mul(n.y, dn)), size)));
                swc1(c + 8, fpu_sub(lwc1(c + 8), fpu_mul(fpu_mul(n.z, dn), size)));
            }
            quad_draw(d, 0);
        } while (i < lw(OV(0x0015f434)));
    }
    ee_stack_free(0x90);
}

}  // namespace rac1
