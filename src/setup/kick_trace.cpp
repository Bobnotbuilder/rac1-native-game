// The kick program of a packet (src/formats/moby_mrp.h KickOp): the ported VU1 moby program
// (micro 0x22d, the no-clip form; src/game/ovl/moby_vu1.cpp, proved against the uploaded
// original) run once on the packet's real index list / texture prims with slot data that names
// each slot: position (s, 0, 0) with the rows making w = 1 + s / 16 (so Q names the slot whose w
// it came from), colour (s, 0, 0, 0), UV (16 s, 0) with q = 0x1000. Each kicked GS vertex then
// tells which slot fed its position, Q, colour, UV and STQ's Q. The program 0x22d has no branch
// on vertex data (only on the index list), so the structure holds for any data.
#include "kick_trace.h"

#include <game/ovl/moby_vu1.h>
#include <runtime/vu/vu1.h>
#include <runtime/vu/vu_interpreter.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace rac1::mrp {

namespace {

constexpr uint32_t kB = 0x164;  // the buffer base (xtop) the trace uses
constexpr uint32_t kStartQ = 0xff, kStartPrime = 1021;

// the s-th prime (slots 1..97 -> 2..509)
uint32_t prime(uint32_t s) {
    static std::vector<uint32_t> p;
    if (p.empty())
        for (uint32_t v = 2; p.size() < 256; ++v) {
            bool is = true;
            for (uint32_t d = 2; d * d <= v && is; ++d) is = v % d != 0;
            if (is) p.push_back(v);
        }
    return p[s - 1];
}

void put(VUState& st, uint32_t qw, int lane, uint32_t v) { std::memcpy(&st.mem[((qw & 0x3ff) << 4) + 4 * static_cast<uint32_t>(lane)], &v, 4); }
void putf(VUState& st, uint32_t qw, int lane, float f) {
    uint32_t v;
    std::memcpy(&v, &f, 4);
    put(st, qw, lane, v);
}
[[maybe_unused]] uint32_t get(const VUState& st, uint32_t qw, int lane) {
    uint32_t v;
    std::memcpy(&v, &st.mem[((qw & 0x3ff) << 4) + 4 * static_cast<uint32_t>(lane)], 4);
    return v;
}

uint64_t run(VUState& st, uint32_t start) {
    VuMicroRun r;
    r.start(st, nullptr, 2048, start, &moby_vu1_step);
    r.limit_issue(2000000);
    while (r.pipe().issued < 2000000 && r.step()) {
    }
    r.finish();
    return r.pipe().issued;
}

}  // namespace

namespace {
// One trace: the program at `start` (0x00e the no-clip form, 0x00a the clipping one: then the clip
// scale B+294 made tiny, nothing reaches the guard band, the tagged triangles are kept whole).
bool trace_one(Packet& p, uint32_t start, std::vector<KickOp>& ops, std::string& note) {
    ops.clear();
    if (p.lod == 2 || p.n_slots == 0) return false;  // (the shadow mesh: another program)
    auto st = std::make_unique<VUState>();
    st->vf[0].f[3] = 1.0f;
    run(*st, 0x000);  // the set-up program (the output buffers in vf27)
    // the packet's VIF list into the buffer (the index list, the texture prims, UVs)
    vu1::Vu1 vu;
    vu.tops = kB;
    for (size_t i = 0; i + 4 <= p.vif.size();) {
        uint32_t code;
        std::memcpy(&code, &p.vif[i], 4);
        i += 4;
        const uint32_t nb = vu1::vif_data_bytes(vu, code);
        vu1::vif1_command(vu, code & ~0x10000000u, p.vif.data() + i, nullptr);
        i += nb;
    }
    st->mem = vu.st.mem;
    const uint32_t n = p.n_slots;
    for (uint32_t s = 1; s <= n; ++s) {
        const uint32_t a = kB - 1 + s;
        put(*st, a, 0, s);
        put(*st, a, 1, 0);
        put(*st, a, 2, prime(s) - 1);  // (w = 1 + z: the slot's prime)
        put(*st, a, 3, s);
        put(*st, a + 97, 0, s);
        put(*st, a + 97, 1, 0);
        put(*st, a + 97, 2, 0);
        put(*st, a + 97, 3, 0);
        put(*st, a + 194, 0, 16 * s);
        put(*st, a + 194, 1, 0);
        put(*st, a + 194, 2, 0x1000);
        put(*st, a + 194, 3, 0);
    }
    // the texture prims: each register's data names its prim and register
    for (size_t t = 0; t < p.textures.size(); ++t)
        for (uint32_t r = 0; r < 4; ++r) {
            const uint32_t q = kB + p.vif_tex_addr + static_cast<uint32_t>(t) * 4 + r;
            put(*st, q, 0, 0x5a000000u + static_cast<uint32_t>(t) * 4 + r);
            put(*st, q, 1, 0);
        }
    // the constants: the A+D tag (fog floor 0), the PRIM tag (proj 1.0), offset 0, the rows
    put(*st, kB + 291, 0, 0);
    put(*st, kB + 291, 1, 0x10000000u);
    put(*st, kB + 291, 2, 0xe);
    put(*st, kB + 291, 3, 0);
    put(*st, kB + 292, 0, 0x3f800000u);
    put(*st, kB + 292, 1, 0x30000000u | (1u << 14) | (0x4u << 15));  // NREG 3, PRE, PRIM (tri strip)
    put(*st, kB + 292, 2, 0x412);
    put(*st, kB + 292, 3, 0);
    for (int l = 0; l < 4; ++l) put(*st, kB + 296, l, 0);
    for (int l = 0; l < 3; ++l) putf(*st, kB + 294, l, 1e-12f);
    putf(*st, kB + 294, 3, 1.0f);
    putf(*st, kB + 297, 0, 65536.0f);
    putf(*st, kB + 297, 1, 0.0f);
    putf(*st, kB + 297, 2, 0.0f);
    putf(*st, kB + 297, 3, 0.0f);
    for (int l = 0; l < 4; ++l) putf(*st, kB + 298, l, 0.0f);
    putf(*st, kB + 299, 0, 0.0f);
    putf(*st, kB + 299, 1, 0.0f);
    putf(*st, kB + 299, 2, 0.0f);
    putf(*st, kB + 299, 3, 1.0f);
    putf(*st, kB + 300, 0, 0.0f);
    putf(*st, kB + 300, 1, 1048576.0f);
    putf(*st, kB + 300, 2, 0.0f);
    putf(*st, kB + 300, 3, 1.0f);
    st->xtop = kB;
    std::vector<std::vector<uint8_t>> kicks;
    st->xgkick_hook = [&](uint32_t addr) {
        const uint32_t qwc = vu1::gif_packet_qwc(*st, addr);
        std::vector<uint8_t> b(static_cast<size_t>(qwc) * 16);
        for (uint32_t q = 0; q < qwc; ++q) std::memcpy(&b[q * 16], &st->mem[((addr + q) & 0x3ff) << 4], 16);
        kicks.push_back(std::move(b));
    };
    // (Q as the program finds it at its start: the previous run's, a value no slot gives)
    st->q = 1.0f / static_cast<float>(kStartPrime);
    run(*st, start);
    st->xgkick_hook = nullptr;
    // Q of slot s: 1 / prime(s) (kStartQ: the start's); products of two identify both factors
    auto qof = [](uint32_t s) { return 1.0f / static_cast<float>(s == kStartQ ? kStartPrime : prime(s)); };
    std::vector<uint32_t> cand;
    for (uint32_t s2 = 1; s2 <= n; ++s2) cand.push_back(s2);
    cand.push_back(kStartQ);
    auto find_q = [&](float q, uint8_t& a, uint8_t& b) {
        for (uint32_t s2 : cand)
            if (std::fabs(qof(s2) - q) <= 1e-5f * q) {
                a = static_cast<uint8_t>(s2);
                b = 0;
                return true;
            }
        for (uint32_t s2 : cand)
            for (uint32_t r : cand)
                if (r >= s2 && std::fabs(qof(s2) * qof(r) - q) <= 1e-5f * q) {
                    a = static_cast<uint8_t>(s2);
                    b = static_cast<uint8_t>(r);
                    return true;
                }
        return false;
    };
    bool ok = true;
    char buf[160];
    for (const auto& pk : kicks) {
        size_t at = 0;
        while (at + 16 <= pk.size()) {
            uint64_t lo, hi;
            std::memcpy(&lo, &pk[at], 8);
            std::memcpy(&hi, &pk[at + 8], 8);
            at += 16;
            const uint32_t nloop = lo & 0x7fff, flg = (lo >> 58) & 3, nreg = ((lo >> 60) & 0xf) == 0 ? 16 : (lo >> 60) & 0xf;
            if ((lo >> 46) & 1) {
                KickOp r;
                r.kind = KickKind::Restart;
                ops.push_back(r);
            }
            if (flg == 0 && nreg == 1 && (hi & 0xf) == 0xe) {
                for (uint32_t i = 0; i < nloop && at + 16 <= pk.size(); ++i, at += 16) {
                    uint32_t d;
                    std::memcpy(&d, &pk[at], 4);
                    if ((d & 0xff000000u) != 0x5a000000u || (d & 3) != i % 4) {
                        std::snprintf(buf, sizeof buf, "an A+D register not from a prim (%08x)", d);
                        note = buf;
                        ok = false;
                    }
                    if (i % 4 == 0) {
                        KickOp t;
                        t.kind = KickKind::Texture;
                        t.texture = static_cast<int16_t>(((d & 0xffffffu) - (d & 3)) / 4);
                        ops.push_back(t);
                    }
                }
            } else if (flg == 0 && nreg == 3 && (hi & 0xfff) == 0x412) {
                for (uint32_t i = 0; i < nloop && at + 48 <= pk.size(); ++i, at += 48) {
                    float s, q;
                    uint32_t c, x, y, f;
                    std::memcpy(&s, &pk[at], 4);
                    std::memcpy(&q, &pk[at + 8], 4);
                    std::memcpy(&c, &pk[at + 16], 4);
                    std::memcpy(&x, &pk[at + 32], 4);
                    std::memcpy(&y, &pk[at + 36], 4);
                    std::memcpy(&f, &pk[at + 44], 4);
                    KickOp v;
                    v.col = static_cast<uint8_t>(c);
                    v.adc = ((f >> 15) & 1) != 0;
                    // Y = 2^20 Q 16 (the pos Q), X = 2^16 s Q 16
                    const float qp = static_cast<float>(y) / 16777216.0f;
                    uint8_t dummy = 0;
                    bool good = find_q(qp, v.pos_q, dummy) && dummy == 0 && c >= 1 && c <= n;
                    // (the integer truncation of 65536 Q: the nearest slot)
                    if (!good) {
                        float best = 1e9f;
                        for (uint32_t s2 : cand)
                            if (std::fabs(qof(s2) * 16777216.0f - static_cast<float>(y)) < best) {
                                best = std::fabs(qof(s2) * 16777216.0f - static_cast<float>(y));
                                v.pos_q = static_cast<uint8_t>(s2);
                            }
                        good = best <= 1.5f && c >= 1 && c <= n;
                    }
                    if (good) {
                        const float ps = static_cast<float>(x) / (1048576.0f * qof(v.pos_q));
                        const long pr = std::lround(ps);
                        good = pr >= 1 && pr <= static_cast<long>(n) && std::fabs(ps - static_cast<float>(pr)) < 0.01f;
                        v.pos = static_cast<uint8_t>(pr);
                    }
                    if (good) {
                        const float us = s / q * 256.0f;
                        const long ur = std::lround(us);
                        good = ur >= 1 && ur <= static_cast<long>(n) && std::fabs(us - static_cast<float>(ur)) < 0.01f && find_q(q, v.uv_q, v.uv_q2);
                        v.uv = static_cast<uint8_t>(ur);
                    }
                    if (!good && ok) {
                        std::snprintf(buf, sizeof buf, "kicked vertex %zu not attributed (x %u y %u stq %g/%g colour %u)", ops.size(), x, y, s, q, c);
                        note = buf;
                        ok = false;
                    }
                    ops.push_back(v);
                }
            } else {
                const uint32_t body = flg == 0 ? nloop * nreg : flg == 1 ? (nloop * nreg + 1) / 2 : nloop;
                at += body * 16;
            }
            if ((lo >> 15) & 1) break;
        }
    }
    return ok;
}
}  // namespace

bool trace_kicks(Packet& p, std::string& note) {
    p.kicks_ok = false;
    p.kicks.clear();
    p.kicks_clip.clear();
    if (p.lod == 2 || p.n_slots == 0) return false;
    std::string n1, n2;
    const bool a = trace_one(p, 0x00e, p.kicks, n1);
    const bool b = trace_one(p, 0x00a, p.kicks_clip, n2);
    note = !a ? n1 : !b ? "clipping program: " + n2 : std::string();
    p.kicks_ok = a && b;
    p.kicks_note = note;
    return p.kicks_ok;
}

}  // namespace rac1::mrp
