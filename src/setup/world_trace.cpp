// The world exporter's decoding of the level's tfrags / tie classes / shrub classes and the kick traces:
// each packet's static VU1 image is built with the VIF (runtime/vu vif1_command, as the frame chain unpacks it), then the
// ported program (src/game/ovl/world_vu1_*.cpp, proved against the uploaded original by) runs once on
// inputs that name their source (the moby exporter's method, src/setup/kick_trace.cpp): each kicked GS
// vertex then says which input vertex fed its position, its Q, its UV and which lit colour its RGBA is.
#include "world_trace.h"

#include <formats/scene_wad.h>
#include <formats/level_textures.h>
#include <game/ovl/world_vu1.h>
#include <game/render/mrp_world_assets.h>
#include <runtime/vu/vu1.h>
#include <runtime/vu/vu_interpreter.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>

namespace rac1::mrpw {

namespace {

template <typename T> T rd(const std::vector<uint8_t>& b, size_t at) {
    T v{};
    if (at + sizeof(T) <= b.size()) std::memcpy(&v, b.data() + at, sizeof(T));
    return v;
}

void put(VUState& st, uint32_t qw, int lane, uint32_t v) { std::memcpy(&st.mem[((qw & 0x3ff) << 4) + 4 * static_cast<uint32_t>(lane)], &v, 4); }
void putf(VUState& st, uint32_t qw, int lane, float f) {
    uint32_t v;
    std::memcpy(&v, &f, 4);
    put(st, qw, lane, v);
}
uint32_t get(const VUState& st, uint32_t qw, int lane) {
    uint32_t v;
    std::memcpy(&v, &st.mem[((qw & 0x3ff) << 4) + 4 * static_cast<uint32_t>(lane)], 4);
    return v;
}
uint64_t run(VUState& st, uint32_t start, VuNativeStep step) {
    VuMicroRun r;
    r.start(st, nullptr, 2048, start, step);
    r.limit_issue(4000000);
    while (r.pipe().issued < 4000000 && r.step()) {
    }
    r.finish();
    return r.pipe().issued;
}

// the n-th prime (n >= 1)
uint32_t prime(uint32_t n) {
    static std::vector<uint32_t> p;
    if (p.empty())
        for (uint32_t v = 2; p.size() < 4096; ++v) {
            bool is = true;
            for (uint32_t d = 2; d * d <= v && is; ++d) is = v % d != 0;
            if (is) p.push_back(v);
        }
    return p[n - 1];
}

// A VIF list run into a VU1 image (from `base`, OFFSET 0: TOPS = base).
void vif_into(vu1::Vu1& vu, const std::vector<uint8_t>& vif) {
    for (size_t i = 0; i + 4 <= vif.size();) {
        uint32_t code;
        std::memcpy(&code, &vif[i], 4);
        i += 4;
        const uint32_t nb = vu1::vif_data_bytes(vu, code);
        const uint32_t cmd = (code >> 24) & 0x7f;
        if (cmd == 0x14 || cmd == 0x15 || cmd == 0x17) {  // (a program start inside the list: none expected)
            i += nb;
            continue;
        }
        if (i + nb > vif.size()) break;
        vu1::vif1_command(vu, code & 0x7fffffffu, vif.data() + i, nullptr);
        i += nb;
    }
}

// The shrub pass's constants (micro 0x000 reads qwords 0..2): the A+D GIF tag, the float tricks' biases (x: 0x4bc00408
// the output buffers' bias, y: 2^23), the fog's scale / offset, the fog's floor / ceiling. Values as the game uploads
// them (Veldin; the fog's are irrelevant to the trace).
void shrub_consts(VUState& st) {
    put(st, 0, 0, 1);
    put(st, 0, 1, 0x40000000u);
    put(st, 0, 2, 0xeeee);
    putf(st, 0, 3, 5.0f);
    put(st, 1, 0, 0x4bc00408u);
    put(st, 1, 1, 0x4b000000u);
    putf(st, 1, 2, -0.0267886f);
    put(st, 1, 3, 0x437effffu);
    putf(st, 2, 0, 53.55f);
    putf(st, 2, 1, 255.0f);
    put(st, 2, 2, 0);
    put(st, 2, 3, 0);
}

}  // namespace

// ---- shrubs ----

bool trace_shrub_packet(ShrubPacket& p, std::string& note) {
    p.ops.clear();
    p.verts.clear();
    p.kick_ok = false;
    const uint32_t n = static_cast<uint32_t>(p.pos.size());
    if (n == 0) {
        note = "no vertices";
        return false;
    }
    // the static image at buffer base 2 (the chain's BASE 2 / OFFSET 0 before MSCAL 0x011)
    constexpr uint32_t kBase = 2;
    auto st = std::make_unique<VUState>();
    st->vf[0].f[3] = 1.0f;
    const auto step = &world_shrub_a_fast;
    shrub_consts(*st);  // (the pass's first CNT, then its MSCAL 0x000; the packets then overwrite qword 2)
    run(*st, 0x000, step);
    for (size_t q = 0; q * 16 < p.image.size(); ++q)
        for (int l = 0; l < 4; ++l) put(*st, kBase + static_cast<uint32_t>(q), l, rd<uint32_t>(p.image, q * 16 + 4 * static_cast<size_t>(l)));
    // tagged inputs: vertex k (1-based) position (k, 0, prime(k) - 1), (s, t, h) = (16 k, 0, 4096); its slot offset and n | stop kept
    const uint32_t voff = static_cast<uint32_t>(p.header[3]);
    for (uint32_t k = 1; k <= n; ++k) {
        const uint32_t a = kBase + voff + k - 1;
        put(*st, a, 0, k);
        put(*st, a, 1, 0);
        put(*st, a, 2, prime(k) - 1);
        put(*st, a + n, 0, 16 * k);
        put(*st, a + n, 1, 0);
        put(*st, a + n, 2, 4096);
    }
    // the texture prims' data name their prim and qword
    const uint32_t ntags = static_cast<uint32_t>(p.header[1]);
    for (uint32_t t = 0; t < p.textures.size(); ++t)
        for (uint32_t r = 0; r < 4; ++r) {
            const uint32_t q = kBase + 1 + ntags + t * 4 + r;
            put(*st, q, 0, 0x5a000000u + t * 4 + r);
        }
    run(*st, 0x011, step);
    // one instance at 0xee: rows giving p = (65536 x, 0, z, 1 + z), the lit colour c = (c + 1, 0x40, 0x20) as integers, alpha 0x33
    constexpr uint32_t kInst = 0xee;
    put(*st, kInst, 0, 1);
    for (int r = 0; r < 4; ++r)
        for (int l = 0; l < 4; ++l) putf(*st, kInst + 1 + static_cast<uint32_t>(r), l, 0.0f);
    putf(*st, kInst + 1, 0, 65536.0f);
    putf(*st, kInst + 3, 2, 1.0f);
    putf(*st, kInst + 3, 3, 1.0f);
    putf(*st, kInst + 4, 3, 1.0f);
    for (uint32_t c = 0; c < 24; ++c) {
        put(*st, kInst + 5 + c, 0, c + 1);
        put(*st, kInst + 5 + c, 1, 0x40);
        put(*st, kInst + 5 + c, 2, 0x20);
        put(*st, kInst + 5 + c, 3, c == 0 ? 0x33 : 0x11);
    }
    std::vector<std::vector<uint8_t>> kicks;
    st->xgkick_hook = [&](uint32_t addr) {
        const uint32_t qwc = vu1::gif_packet_qwc(*st, addr);
        std::vector<uint8_t> b(static_cast<size_t>(qwc) * 16);
        for (uint32_t q = 0; q < qwc; ++q) std::memcpy(&b[q * 16], &st->mem[((addr + q) & 0x3ff) << 4], 16);
        kicks.push_back(std::move(b));
    };
    // the qwords at 2 x (block + 5) up to the second output buffer (0x208) name their address (kTwiceBase colours)
    for (uint32_t a = 2 * (kInst + 5); a < 0x208; ++a) {
        put(*st, a, 0, 0x10000u + a);
        put(*st, a, 1, 0x40);
        put(*st, a, 2, 0x20);
    }
    // the next instance's block (0x1c qwords on) tagged too: a vertex after a mid-list end flag reads its colour there
    for (uint32_t c = 0; c < 0x1c; ++c) {
        put(*st, kInst + 0x1c + 5 + c, 0, 0x100 + c);
        put(*st, kInst + 0x1c + 5 + c, 1, 0x40);
        put(*st, kInst + 0x1c + 5 + c, 2, 0x20);
    }
    run(*st, 0x063, step);
    st->xgkick_hook = nullptr;
    char buf[200];
    if (kicks.size() != 1) {
        std::snprintf(buf, sizeof buf, "%zu kicks", kicks.size());
        note = buf;
        return false;
    }
    const auto& pk = kicks[0];
    bool ok = true;
    auto fail = [&](const char* what) {
        if (ok) note = what;
        ok = false;
    };
    size_t at = 0;
    while (at + 16 <= pk.size()) {
        uint64_t lo, hi;
        std::memcpy(&lo, &pk[at], 8);
        std::memcpy(&hi, &pk[at + 8], 8);
        Op tag;
        tag.kind = OpKind::Tag;
        tag.tag[0] = lo;
        tag.tag[1] = hi;
        p.ops.push_back(tag);
        at += 16;
        const uint32_t nloop = lo & 0x7fff, flg = (lo >> 58) & 3, nreg = ((lo >> 60) & 0xf) == 0 ? 16 : (lo >> 60) & 0xf;
        if (flg != 0) {
            fail("a GIF tag not PACKED");
            break;
        }
        for (uint32_t i = 0; i < nloop; ++i)
            for (uint32_t r = 0; r < nreg && at + 16 <= pk.size(); ++r) {
                const uint32_t reg = static_cast<uint32_t>(hi >> (4 * r)) & 0xf;
                uint32_t w[4];
                std::memcpy(w, &pk[at], 16);
                if (reg == 0xe) {  // A+D: the prims' data name themselves
                    if ((w[0] & 0xff000000u) != 0x5a000000u || (w[0] & 3) != (i * nreg + r) % 4) {
                        std::snprintf(buf, sizeof buf, "an A+D qword not from a prim (%08x)", w[0]);
                        fail(buf);
                    } else if ((i * nreg + r) % 4 == 0) {
                        Op t;
                        t.kind = OpKind::Texture;
                        t.index = static_cast<uint16_t>((w[0] & 0xffffffu) / 4);
                        p.ops.push_back(t);
                    }
                    at += 16;
                    continue;
                }
                if (nreg == 3 && r == 0 && (hi & 0xfff) == 0x412 && at + 48 <= pk.size()) {  // STQ, RGBAQ, XYZF2
                    float s, q;
                    std::memcpy(&s, &pk[at], 4);
                    std::memcpy(&q, &pk[at + 8], 4);
                    int32_t c[4], x;
                    std::memcpy(c, &pk[at + 16], 16);
                    std::memcpy(&x, &pk[at + 32], 4);
                    ShrubKickVertex v;
                    // Q names the vertex whose w (its prime) the projection took
                    int qs = -1;
                    for (uint32_t k = 1; k <= n; ++k)
                        if (std::fabs(q * static_cast<float>(prime(k)) - 1.0f) < 1e-5f) {
                            qs = static_cast<int>(k);
                            break;
                        }
                    // UV: s = 16 u / 4096 x Q;  position: X = ftoi4(65536 k Q)
                    const double u = static_cast<double>(s) / q * 256.0;
                    const double kp = static_cast<double>(x) / 16.0 / 65536.0 / q;
                    const long ui = std::lround(u), ki = std::lround(kp);
                    int col = c[0] - 1;
                    if (c[0] >= 0x10000 + 2 * static_cast<int32_t>(kInst + 5) && c[0] < 0x10000 + 2 * static_cast<int32_t>(kInst + 5) + 0x7fff) col = kTwiceBase + (c[0] - 0x10000 - 2 * static_cast<int32_t>(kInst + 5));
                    else if (c[0] < 1 || c[0] > 24) col = -1;
                    if (qs < 0 || std::fabs(u - static_cast<double>(ui)) > 1e-3 || std::fabs(kp - static_cast<double>(ki)) > 2e-3 || ui < 1 || ki < 1 ||
                        ui > static_cast<long>(n) || ki > static_cast<long>(n) || col < 0 || c[1] != 0x40 || c[2] != 0x20) {
                        std::snprintf(buf, sizeof buf, "a vertex it could not attribute (q %g s %g x %d rgba %d %d %d %d)", static_cast<double>(q), static_cast<double>(s), x, c[0],
                                      c[1], c[2], c[3]);
                        fail(buf);
                        if (std::getenv("MRPW_DEBUG") != nullptr)
                            for (uint32_t k = 0; k < n; ++k) std::fprintf(stderr, "    v%u: pos %d %d %d off %d  uv %d %d %d w %04x\n", k, p.pos[k][0], p.pos[k][1], p.pos[k][2], p.pos[k][3], p.uv[k][0], p.uv[k][1], p.uv[k][2], static_cast<uint16_t>(p.uv[k][3]));
                    } else {
                        v.pos = static_cast<uint16_t>(ki - 1);
                        v.q = static_cast<uint16_t>(qs - 1);
                        v.uv = static_cast<uint16_t>(ui - 1);
                        v.colour = static_cast<uint16_t>(col);
                    }
                    if (c[3] != 0x33) fail("an alpha not the instance's");
                    if (std::getenv("MRPW_DEBUG2") != nullptr)
                        std::fprintf(stderr, "    kicked: q %g (1/%g) s %g x %d rgba %d %d %d %d -> pos %ld uv %ld\n", static_cast<double>(q), 1.0 / static_cast<double>(q),
                                     static_cast<double>(s), x, c[0], c[1], c[2], c[3], ki, ui);
                    Op o;
                    o.kind = OpKind::Vertex;
                    o.index = static_cast<uint16_t>(p.verts.size());
                    p.verts.push_back(v);
                    p.ops.push_back(o);
                    at += 48;
                    r = nreg;  // (the three registers taken)
                    continue;
                }
                Op o;
                o.kind = OpKind::Other;
                std::memcpy(o.tag, &pk[at], 16);
                p.ops.push_back(o);
                fail("a GIF register the trace does not know");
                at += 16;
            }
        if (((lo >> 15) & 1) != 0) break;  // EOP
    }
    p.kick_ok = ok;
    p.kick_note = note;
    return ok;
}

namespace {

bool decode_shrub_class(const std::vector<uint8_t>& b, ShrubClass& c, std::string& err) {
    if (b.size() < 0x40) {
        err = "short class";
        return false;
    }
    c.header.assign(b.begin(), b.begin() + 0x40);
    const int32_t np = rd<int16_t>(b, 0x28);
    for (int32_t i = 0; i < np; ++i) {
        const int32_t off = rd<int32_t>(b, 0x40 + 8 * static_cast<size_t>(i)), size = rd<int32_t>(b, 0x44 + 8 * static_cast<size_t>(i));
        if (off <= 0 || size <= 0 || static_cast<size_t>(off) + static_cast<size_t>(size) > b.size()) {
            err = "bad packet entry";
            return false;
        }
        ShrubPacket p;
        p.vif.assign(b.begin() + off, b.begin() + off + size);
        vu1::Vu1 vu;
        vif_into(vu, p.vif);
        p.image = shrub_image(p);  // (the same run, kept by the asset's loader)
        for (int l = 0; l < 4; ++l) p.header[l] = static_cast<int32_t>(get(vu.st, 0, l));
        const int32_t ntex = p.header[0], ntags = p.header[1], nv = p.header[2], voff = p.header[3];
        if (ntex < 0 || ntags < 0 || nv <= 0 || voff <= 0 || voff + 2 * nv > 0x3f0) {
            err = "bad packet header";
            return false;
        }
        for (int32_t t = 0; t < ntex; ++t) {
            TexturePrim tp;
            for (uint32_t r = 0; r < 4; ++r) {
                const uint32_t q = 1 + static_cast<uint32_t>(ntags + t * 4) + r;
                tp.ad[r][0] = static_cast<uint64_t>(get(vu.st, q, 0)) | static_cast<uint64_t>(get(vu.st, q, 1)) << 32;
                tp.ad[r][1] = static_cast<uint64_t>(get(vu.st, q, 2)) | static_cast<uint64_t>(get(vu.st, q, 3)) << 32;
            }
            tp.local_texture = static_cast<int32_t>(get(vu.st, 1 + static_cast<uint32_t>(ntags + t * 4) + 3, 0));  // (TEX0's word: the local texture)
            p.textures.push_back(tp);
        }
        for (int32_t k = 0; k < nv; ++k) {
            std::array<int16_t, 4> a{}, u{};
            for (int l = 0; l < 4; ++l) {
                a[static_cast<size_t>(l)] = static_cast<int16_t>(get(vu.st, static_cast<uint32_t>(voff + k), l));
                u[static_cast<size_t>(l)] = static_cast<int16_t>(get(vu.st, static_cast<uint32_t>(voff + nv + k), l));
            }
            p.pos.push_back(a);
            p.uv.push_back(u);
        }
        c.packets.push_back(std::move(p));
    }
    return true;
}

}  // namespace

// ---- ties ----

namespace {

// The tie pass's constants (the chain's 9 qwords from 0x1c2710 to 0x3c6, then MSCAL 0x008): the A+D tag (NLOOP 5), the
// strips' GIF tag (PRIM 0x7c: STQ, RGBAQ, XYZF2), the output buffers' float biases, the instance blocks' addresses
// (0xc6 / 0xf2), the alpha-test state packets. Veldin's values (constant: the pass copies them from its data).
void tie_consts(VUState& st) {
    static const uint32_t q[9][4] = {
        {0x00000005, 0x10000000, 0x0000000e, 0x00000000}, {0x00000000, 0x303e4000, 0x00000412, 0x00000000}, {0x4bc002c1, 0x00000000, 0x4bc002c1, 0x00000000},
        {0x4b00028e, 0x4b0001d6, 0x4b00028e, 0x4b0001d6}, {0x000000c6, 0x000000f2, 0x000000c6, 0x000000f2}, {0x00000000, 0xffffffff, 0x00000000, 0x00000000},
        {0x00008001, 0x10000000, 0x0000000e, 0x00000000}, {0x0005340b, 0x00000000, 0x00000047, 0x00000000}, {0x0005000e, 0x00000000, 0x00000047, 0x00000000},
    };
    for (uint32_t i = 0; i < 9; ++i)
        for (int l = 0; l < 4; ++l) put(st, 0x3c6 + i, l, q[i][l]);
}

}  // namespace

bool trace_tie_packet(TiePacket& p, std::string& note) {
    p.ops.clear();
    p.verts.clear();
    p.kick_ok = false;
    const uint32_t nd = p.n_dinky, nf = p.n_fat, n = nd + nf;
    if (n == 0) {
        note = "no vertices";
        return false;
    }
    auto st = std::make_unique<VUState>();
    st->vf[0].f[3] = 1.0f;
    const auto step = &world_tie_a_fast;
    tie_consts(*st);
    run(*st, 0x008, step);
    for (size_t q = 0; q * 16 < p.image.size(); ++q)
        for (int l = 0; l < 4; ++l) put(*st, static_cast<uint32_t>(q), l, rd<uint32_t>(p.image, q * 16 + 4 * static_cast<size_t>(l)));
    // tagged inputs: vertex k (1-based: dinky, then fat) position (k, 0, prime(k) - 1), UV (16 k, 0, 4096); slots kept; fat deltas 0
    for (uint32_t k = 1; k <= nd; ++k) {
        const uint32_t a = 0x32 + 2 * (k - 1);
        put(*st, a, 0, k);
        put(*st, a, 1, 0);
        put(*st, a, 2, prime(k) - 1);
        put(*st, a + 1, 0, 16 * k);
        put(*st, a + 1, 1, 0);
        put(*st, a + 1, 2, 4096);
    }
    for (uint32_t f = 0; f < nf; ++f) {
        const uint32_t k = nd + f + 1, a = 0x32 + 2 * nd + 3 * f;
        put(*st, a, 0, 0);
        put(*st, a, 1, 0);
        put(*st, a, 2, 0);
        put(*st, a + 1, 0, k);
        put(*st, a + 1, 1, 0);
        put(*st, a + 1, 2, prime(k) - 1);
        put(*st, a + 2, 0, 16 * k);
        put(*st, a + 2, 1, 0);
        put(*st, a + 2, 2, 4096);
    }
    // the texture prims' data name their slot and qword
    for (uint32_t t = 0; t < p.prims.size(); ++t)
        for (uint32_t r = 0; r < 5; ++r) put(*st, 1 + 5 * t + r, 0, 0x5a000000u + t * 8 + r);
    run(*st, 0x006, step);
    const uint32_t block = static_cast<uint32_t>(st->vi[3]) & 0x3ff;
    // one instance: rows giving p = (65536 x, 0, z, 1 + z); morph 1 (fat: base + delta = base), colour factors z 1 / w 0; state 0, fog 100
    for (int r = 0; r < 6; ++r)
        for (int l = 0; l < 4; ++l) putf(*st, block + static_cast<uint32_t>(r), l, 0.0f);
    putf(*st, block, 0, 65536.0f);
    putf(*st, block + 2, 2, 1.0f);
    putf(*st, block + 2, 3, 1.0f);
    putf(*st, block + 3, 3, 1.0f);
    putf(*st, block + 4, 0, 1.0f);
    putf(*st, block + 4, 2, 1.0f);
    putf(*st, block + 5, 3, 100.0f);
    // the lit colours (as STMOD 1 with the row 0x4b000000 leaves them: 2^23 + byte): table 0x346 c + 1, table 0x386 0x80 + c + 1
    for (uint32_t c = 0; c < 64; ++c) {
        put(*st, 0x346 + c, 0, 0x4b000000u + c + 1);
        put(*st, 0x386 + c, 0, 0x4b000000u + 0x80 + c + 1);
        for (uint32_t t : {0x346u, 0x386u}) {
            put(*st, t + c, 1, 0x4b000040u);
            put(*st, t + c, 2, 0x4b000020u);
            put(*st, t + c, 3, 0x4b000080u);
        }
    }
    std::vector<std::vector<uint8_t>> kicks;
    st->xgkick_hook = [&](uint32_t addr) {
        const uint32_t qwc = vu1::gif_packet_qwc(*st, addr);
        std::vector<uint8_t> b(static_cast<size_t>(qwc) * 16);
        for (uint32_t q = 0; q < qwc; ++q) std::memcpy(&b[q * 16], &st->mem[((addr + q) & 0x3ff) << 4], 16);
        kicks.push_back(std::move(b));
    };
    // a second run for the fat vertices' other two colours: colour factors z 0 / w 1, the colours c = (c, c^2) as plain floats
    // (table 0x386: c + 64), so a fat vertex's RGBA is ((c1 + c2) / 2, (c1^2 + c2^2) / 2)
    auto st2 = std::make_unique<VUState>(*st);
    putf(*st2, block + 4, 2, 0.0f);
    putf(*st2, block + 4, 3, 1.0f);
    for (uint32_t c = 0; c < 128; ++c) {
        const uint32_t a = c < 64 ? 0x346 + c : 0x386 + (c - 64);
        putf(*st2, a, 0, static_cast<float>(c));
        putf(*st2, a, 1, static_cast<float>(c * c));
        putf(*st2, a, 2, 0.0f);
        putf(*st2, a, 3, 0.0f);
    }
    std::vector<std::array<int, 2>> fat_pairs;  // per kicked vertex in order
    if (nf > 0) {
        st2->xgkick_hook = [&](uint32_t addr) {
            const uint32_t qwc = vu1::gif_packet_qwc(*st2, addr);
            std::vector<uint8_t> b(static_cast<size_t>(qwc) * 16);
            for (uint32_t q = 0; q < qwc; ++q) std::memcpy(&b[q * 16], &st2->mem[((addr + q) & 0x3ff) << 4], 16);
            size_t at = 0;
            while (at + 16 <= b.size()) {
                uint64_t lo, hi;
                std::memcpy(&lo, &b[at], 8);
                std::memcpy(&hi, &b[at + 8], 8);
                at += 16;
                const uint32_t nloop = lo & 0x7fff, nreg = ((lo >> 60) & 0xf) == 0 ? 16 : (lo >> 60) & 0xf;
                const bool vtx = nreg == 3 && (hi & 0xfff) == 0x412;
                for (uint32_t i = 0; i < nloop && at < b.size(); ++i) {
                    if (vtx) {
                        float r, g;
                        std::memcpy(&r, &b[at + 16], 4);
                        std::memcpy(&g, &b[at + 20], 4);
                        const double s = 2.0 * r, p2 = 2.0 * g, prod = (s * s - p2) / 2.0, disc = s * s - 4.0 * prod;
                        int c1 = -1, c2 = -1;
                        if (disc >= -1e-6) {
                            const double rt = std::sqrt(std::max(0.0, disc));
                            c1 = static_cast<int>(std::lround((s - rt) / 2.0));
                            c2 = static_cast<int>(std::lround((s + rt) / 2.0));
                        }
                        fat_pairs.push_back({c1, c2});
                        at += 48;
                    } else {
                        at += 16 * nreg;
                    }
                }
                if (((lo >> 15) & 1) != 0) break;
            }
        };
        run(*st2, 0x000, step);
        st2->xgkick_hook = nullptr;
    }
    run(*st, 0x000, step);
    st->xgkick_hook = nullptr;
    char buf[240];
    bool ok = true;
    auto fail = [&](const char* what) {
        if (ok) note = what;
        ok = false;
    };
    int table = -1;
    int drawing_kicks = 0;
    for (const auto& pk : kicks) {
        // (a kick without vertices is the alpha-test state packet the program sends when an instance's state differs from
        // the previous one's (program state): not the packet's; the CPU reference leaves it out)
        bool has_vertices = false;
        for (size_t at = 0; at + 16 <= pk.size(); at += 16) {
            uint64_t hi;
            std::memcpy(&hi, &pk[at + 8], 8);
            uint64_t lo;
            std::memcpy(&lo, &pk[at], 8);
            if (((lo >> 60) & 0xf) == 3 && (hi & 0xfff) == 0x412) has_vertices = true;
        }
        if (!has_vertices) continue;
        if (++drawing_kicks > 1) fail("two drawing kicks");
        size_t at = 0;
        while (at + 16 <= pk.size()) {
            uint64_t lo, hi;
            std::memcpy(&lo, &pk[at], 8);
            std::memcpy(&hi, &pk[at + 8], 8);
            Op tag;
            tag.kind = OpKind::Tag;
            tag.tag[0] = lo;
            tag.tag[1] = hi;
            p.ops.push_back(tag);
            at += 16;
            const uint32_t nloop = lo & 0x7fff, flg = (lo >> 58) & 3, nreg = ((lo >> 60) & 0xf) == 0 ? 16 : (lo >> 60) & 0xf;
            if (flg != 0) {
                fail("a GIF tag not PACKED");
                break;
            }
            for (uint32_t i = 0; i < nloop; ++i)
                for (uint32_t r = 0; r < nreg && at + 16 <= pk.size(); ++r) {
                    const uint32_t reg = static_cast<uint32_t>(hi >> (4 * r)) & 0xf;
                    uint32_t w[4];
                    std::memcpy(w, &pk[at], 16);
                    if (reg == 0xe) {
                        Op o;
                        if ((w[0] & 0xff000000u) == 0x5a000000u && (w[0] & 7) == 0) {
                            o.kind = OpKind::Texture;
                            o.index = static_cast<uint16_t>((w[0] & 0xffffffu) / 8);
                            p.ops.push_back(o);
                        } else if ((w[0] & 0xff000000u) == 0x5a000000u) {
                            // (the prim's other qwords: part of the Texture op)
                        } else {
                            o.kind = OpKind::Other;  // (an A+D qword of the constants: the alpha-test state packets)
                            std::memcpy(o.tag, w, 16);
                            p.ops.push_back(o);
                        }
                        at += 16;
                        continue;
                    }
                    if (nreg == 3 && r == 0 && (hi & 0xfff) == 0x412 && at + 48 <= pk.size()) {
                        float s, q;
                        std::memcpy(&s, &pk[at], 4);
                        std::memcpy(&q, &pk[at + 8], 4);
                        uint32_t c[4];
                        int32_t x;
                        std::memcpy(c, &pk[at + 16], 16);
                        std::memcpy(&x, &pk[at + 32], 4);
                        TieKickVertex v;
                        int qs = -1;
                        for (uint32_t k = 1; k <= n; ++k)
                            if (std::fabs(q * static_cast<float>(prime(k)) - 1.0f) < 1e-5f) {
                                qs = static_cast<int>(k);
                                break;
                            }
                        const double u = static_cast<double>(s) / q * 256.0;
                        const double kp = static_cast<double>(x) / 16.0 / 65536.0 / q;
                        const long ui = std::lround(u), ki = std::lround(kp);
                        const uint32_t cl = c[0] & 0xff;
                        int col = -1, tb = -1;
                        if ((c[0] & 0xffffff00u) == 0x4b000000u && cl >= 1 && cl <= 64) col = static_cast<int>(cl - 1), tb = 0;
                        else if ((c[0] & 0xffffff00u) == 0x4b000000u && cl >= 0x81 && cl <= 0xc0) col = static_cast<int>(cl - 0x81), tb = 1;
                        if (qs < 0 || std::fabs(u - static_cast<double>(ui)) > 1e-3 || std::fabs(kp - static_cast<double>(ki)) > 2e-3 || ui < 1 || ki < 1 ||
                            ui > static_cast<long>(n) || ki > static_cast<long>(n) || col < 0 || c[1] != 0x4b000040u || c[2] != 0x4b000020u) {
                            std::snprintf(buf, sizeof buf, "a vertex it could not attribute (q %g s %g x %d rgba %08x %08x %08x %08x)", static_cast<double>(q), static_cast<double>(s), x,
                                          c[0], c[1], c[2], c[3]);
                            fail(buf);
                        } else {
                            v.pos = static_cast<uint16_t>(ki - 1);
                            v.q = static_cast<uint16_t>(qs - 1);
                            v.uv = static_cast<uint16_t>(ui - 1);
                            v.colour = static_cast<uint16_t>(col);
                            v.fat = static_cast<uint8_t>(v.pos >= nd ? 1 : 0);
                            if (table < 0) table = tb;
                            else if (table != tb) fail("two colour tables in one draw");
                        }
                        Op o;
                        o.kind = OpKind::Vertex;
                        o.index = static_cast<uint16_t>(p.verts.size());
                        p.verts.push_back(v);
                        p.ops.push_back(o);
                        at += 48;
                        r = nreg;
                        continue;
                    }
                    Op o;
                    o.kind = OpKind::Other;
                    std::memcpy(o.tag, &pk[at], 16);
                    p.ops.push_back(o);
                    fail("a GIF register the trace does not know");
                    at += 16;
                }
            if (((lo >> 15) & 1) != 0) break;
        }
    }
    if (nf > 0) {
        if (fat_pairs.size() != p.verts.size()) fail("the second run kicked another vertex count");
        else
            for (size_t i = 0; i < p.verts.size(); ++i) {
                TieKickVertex& v = p.verts[i];
                if (v.fat == 0) continue;
                int a = fat_pairs[i][0], b = fat_pairs[i][1];
                if (table == 1) {
                    a -= 64;
                    b -= 64;
                }
                if (a < 0 || b < 0 || a > 63 || b > 63) {
                    std::snprintf(buf, sizeof buf, "a fat vertex's colour pair not found (%d %d)", fat_pairs[i][0], fat_pairs[i][1]);
                    fail(buf);
                    continue;
                }
                v.colour2[0] = static_cast<uint8_t>(a);
                v.colour2[1] = static_cast<uint8_t>(b);
            }
    }
    p.kick_ok = ok;
    p.kick_note = note;
    return ok;
}

namespace {

// One VIF code + its data into the image (the chain's codes for a tie packet, world_cull.cpp 0x295fa0).
void vif_code(vu1::Vu1& vu, uint32_t code, const uint8_t* data, size_t avail) {
    const uint32_t nb = vu1::vif_data_bytes(vu, code);
    std::vector<uint8_t> buf(nb, 0);
    std::memcpy(buf.data(), data, std::min<size_t>(nb, avail));
    vu1::vif1_command(vu, code, buf.data(), nullptr);
}

bool decode_tie_class(const std::vector<uint8_t>& b, TieClass& c, std::string& err) {
    if (b.size() < 0x70) {
        err = "short class";
        return false;
    }
    c.header.assign(b.begin(), b.begin() + 0x70);
    c.scale = rd<float>(b, 0x40);
    const uint32_t ntex = b[0x23];
    const uint32_t adgif = rd<uint32_t>(b, 0x2c);
    for (uint32_t t = 0; t < ntex; ++t) {
        TexturePrim tp;
        tp.n = 5;
        for (uint32_t r = 0; r < 5; ++r) {
            tp.ad[r][0] = rd<uint64_t>(b, adgif + 0x50 * t + 0x10 * r);
            tp.ad[r][1] = rd<uint64_t>(b, adgif + 0x50 * t + 0x10 * r + 8);
        }
        tp.local_texture = static_cast<int32_t>(rd<uint32_t>(b, adgif + 0x50 * t));  // (TEX0's word: the local texture)
        c.textures.push_back(tp);
    }
    for (uint32_t lod = 0; lod < 3; ++lod) {
        const uint32_t list = rd<uint32_t>(b, 4 * lod), count = b[0x20 + lod];
        for (uint32_t j = 0; j < count; ++j) {
            TiePacket p;
            p.lod = static_cast<uint8_t>(lod);
            p.index_in_lod = static_cast<uint8_t>(j);
            std::memcpy(p.entry.data(), b.data() + list + 16 * j, 16);
            const uint32_t data = list + rd<uint32_t>(b, list + 16 * j);
            const uint32_t nprim = p.entry[4], csize = p.entry[7], vofs = p.entry[8], vsize = p.entry[9], iofs = p.entry[0xa], icount = p.entry[0xb];
            const uint32_t iq = (icount * 4 + 15) / 16;
            const uint32_t end = data + std::max({0x20u + 16 * csize, 16 * (vofs + vsize), 16 * (iofs + 2 * iq)});
            if (data >= b.size() || end > b.size() || nprim == 0 || nprim > 4) {
                err = "bad packet entry";
                return false;
            }
            p.data.assign(b.begin() + data, b.begin() + end);
            // the chain (0x295fa0's packet part): header qword -> 0; the prims (their source offsets at +0x10, / 0x50) -> 1 + 5 k;
            // the control qwords -> 1 + 5 nprim (V4_8 unsigned); the vertices -> 0x32 (V4_16); the colour index lists -> 0xcc, 0xf8
            vu1::Vu1 vu;
            vif_code(vu, 0x6c010000u, b.data() + data, 16);
            for (uint32_t k = 0; k < nprim; ++k) {
                const uint32_t src = rd<uint32_t>(b, data + 0x10 + 4 * k);
                const int32_t prim = static_cast<int32_t>(src / 0x50);
                p.prims.push_back(prim);
                if (src % 0x50 != 0 || static_cast<uint32_t>(prim) >= ntex) {
                    err = "a prim source offset outside the class's prims";
                    return false;
                }
                vif_code(vu, 0x6c050000u | (1 + 5 * k), b.data() + adgif + src, 0x50);
            }
            vif_code(vu, 0x6e004000u | ((4 * csize) << 16) | (1 + 5 * nprim), b.data() + data + 0x20, 16 * csize);
            vif_code(vu, 0x6d000000u | ((2 * vsize & 0xff) << 16) | 0x32, b.data() + data + 16 * vofs, 16 * vsize);
            vif_code(vu, 0x6e004000u | (icount << 16) | 0xcc, b.data() + data + 16 * iofs, 16 * iq);
            vif_code(vu, 0x6e004000u | (icount << 16) | 0xf8, b.data() + data + 16 * (iofs + iq), 16 * iq);
            p.image = tie_image(c, p);
            // the vertex counts: the control's byte 8 (its qword 2, x) = the dinky vertices' unpacked qwords + 4; the rest of the
            // vertex block (2 x the entry's size in unpacked qwords) is the fat vertices, 3 qwords each
            const uint32_t cq = 1 + 5 * nprim;
            p.n_dinky = get(vu.st, cq + 2, 2);  // (bytes 10 / 11 of the control: the dinky / fat vertex counts; byte 8 = 2 x dinky + 4, 9 = 3 x fat)
            p.n_fat = get(vu.st, cq + 2, 3);
            if (2 * p.n_dinky + 3 * p.n_fat > 2 * vsize) {
                err = "vertex counts past the vertex block";
                return false;
            }
            if (std::getenv("MRPW_TIEDBG") != nullptr)
                std::fprintf(stderr, "    tie lod %u pk %u: control %u %u %u %u | %u %u %u %u | %u %u %u %u, vsize %u -> dinky %u fat %u (rest %u)\n", lod, j, get(vu.st, cq, 0),
                             get(vu.st, cq, 1), get(vu.st, cq, 2), get(vu.st, cq, 3), get(vu.st, cq + 1, 0), get(vu.st, cq + 1, 1), get(vu.st, cq + 1, 2), get(vu.st, cq + 1, 3),
                             get(vu.st, cq + 2, 0), get(vu.st, cq + 2, 1), get(vu.st, cq + 2, 2), get(vu.st, cq + 2, 3), vsize, p.n_dinky, p.n_fat,
                             2 * vsize > 2 * p.n_dinky ? (2 * vsize - 2 * p.n_dinky) % 3 : 0u);
            for (uint32_t k = 0; k < p.n_dinky; ++k) {
                std::array<int16_t, 8> v{};
                for (int l = 0; l < 4; ++l) {
                    v[static_cast<size_t>(l)] = static_cast<int16_t>(get(vu.st, 0x32 + 2 * k, l));
                    v[static_cast<size_t>(4 + l)] = static_cast<int16_t>(get(vu.st, 0x33 + 2 * k, l));
                }
                p.dinky.push_back(v);
            }
            const uint32_t f0 = 0x32 + 2 * p.n_dinky;
            for (uint32_t k = 0; k < p.n_fat; ++k) {
                std::array<int16_t, 12> v{};
                for (int q = 0; q < 3; ++q)
                    for (int l = 0; l < 4; ++l) v[static_cast<size_t>(4 * q + l)] = static_cast<int16_t>(get(vu.st, f0 + 3 * k + static_cast<uint32_t>(q), l));
                p.fat.push_back(v);
            }
            for (uint32_t k = 0; k < icount; ++k) p.colour_index.push_back({static_cast<uint8_t>(get(vu.st, 0xcc + k, 0)), static_cast<uint8_t>(get(vu.st, 0xcc + k, 1)),
                                                                           static_cast<uint8_t>(get(vu.st, 0xcc + k, 2)), static_cast<uint8_t>(get(vu.st, 0xcc + k, 3))});
            c.packets.push_back(std::move(p));
        }
    }
    return true;
}

}  // namespace

// The tfrag block (a header: the entries' table offset, their count; 0x40-byte entries, their data at the table + entry +0x10)
// decoded into w.tfrags: the levels' core block and the title's (its loader's 0x241458 on s5 + H[+0x10])
bool decode_tfrag_block(const std::vector<uint8_t>& tb, World& w, std::string& err, bool dump) {
    const uint32_t table = rd<uint32_t>(tb, 0), count = rd<uint32_t>(tb, 4);
    if (dump) std::printf("tfrag block: %zu bytes table %x count %u\n", tb.size(), table, count);
    for (uint32_t i = 0; i < count; ++i) {
        TfragEntry t;
        t.index = i;
        const size_t e = table + 0x40 * static_cast<size_t>(i);
        if (e + 0x40 > tb.size()) {
            err = "tfrag table past the block";
            return false;
        }
        std::memcpy(t.header.data(), tb.data() + e, 0x40);
        auto h16 = [&](size_t o) { return rd<uint16_t>(tb, e + o); };
        const size_t data = table + rd<uint32_t>(tb, e + 0x10);
        const uint16_t lod2 = h16(0x14), shared = h16(0x16), lod0 = h16(0x1a), tex = h16(0x1c), rgba = h16(0x1e);
        const uint8_t csize = tb[e + 0x20], l2size = tb[e + 0x21], l1size = tb[e + 0x22], l0size = tb[e + 0x23];
        const uint8_t ntex = tb[e + 0x28], rgba_loc = tb[e + 0x2a];
        const uint8_t rc[3] = {tb[e + 0x24], tb[e + 0x25], tb[e + 0x26]};
        size_t end = std::max<size_t>({static_cast<size_t>(lod2) + 16u * l2size, static_cast<size_t>(shared) + 16u * l1size, static_cast<size_t>(shared) + 16u * csize,
                                       static_cast<size_t>(lod0) + 16u * l0size, static_cast<size_t>(rgba) + 16u * ((std::max({rc[0], rc[1], rc[2]}) * 4u + 15u) / 16u),
                                       static_cast<size_t>(tex) + 0x50u * ntex});
        if (data + end > tb.size()) {
            err = "tfrag data past the block";
            return false;
        }
        t.data.assign(tb.begin() + static_cast<long>(data), tb.begin() + static_cast<long>(data + end));
        for (uint32_t k = 0; k < ntex; ++k) {
            TexturePrim tp;
            tp.n = 5;
            for (uint32_t r = 0; r < 5; ++r) {
                tp.ad[r][0] = rd<uint64_t>(t.data, tex + 0x50 * k + 0x10 * r);
                tp.ad[r][1] = rd<uint64_t>(t.data, tex + 0x50 * k + 0x10 * r + 8);
            }
            tp.local_texture = tp.texture = static_cast<int32_t>(rd<uint32_t>(t.data, tex + 0x50 * k));
            t.textures.push_back(tp);
        }
        // the three images: the chain's REFs (each with STCYCL 4 / 4 first), then the colours
        const uint16_t refs[3][2][2] = {{{lod2, l2size}, {0, 0}}, {{shared, l1size}, {0, 0}}, {{shared, csize}, {lod0, l0size}}};
        for (int l = 0; l < 3; ++l) {
            TfragImage& im = t.lods[static_cast<size_t>(l)];
            for (int r = 0; r < 2; ++r) im.refs[static_cast<size_t>(r)] = {static_cast<uint16_t>(refs[l][r][0] / 16), refs[l][r][1]};
            tfrag_image(t, l, im);
            im.colour_addr = rgba_loc;
            im.colour_count = rc[l];
        }
        w.tfrags.push_back(std::move(t));
    }
    if (dump) {
        std::printf("tfrags: %u entries\n", count);
        for (const auto& t : w.tfrags)
            for (int l = 0; l < 3; ++l) {
                const auto& im = t.lods[static_cast<size_t>(l)];
                std::printf("  tfrag %u lod %d: %zu qwords;", t.index, l, im.written.size());
                for (int q = 0; q < 2 && 16 * q + 16 <= static_cast<int>(im.image.size()); ++q)
                    for (int b = 0; b < 4; ++b) std::printf(" %x", rd<uint32_t>(im.image, 16 * static_cast<size_t>(q) + 4 * static_cast<size_t>(b)));
                std::printf("\n");
            }
    }
    return true;
}

// The title's level (8.2): one decompressed file H, its data base s5 = H + H[+4]; the
// tie classes H[+0x20] records (0x20 bytes) at H + H[+0x24], the shrub classes H[+0x28] (0x30 bytes) at H + H[+0x2c], each
// record: the class at s5 + its offset (+0), its o_class (+4), its texture remap (+0x10, 16 bytes); the tfrags (the ground, the
// half-pipe arch's shell) at s5 + H[+0x10], the levels' tfrag block form
bool export_title(const std::vector<uint8_t>& H, World& w, std::string& err, bool dump) {
    w.level = kTitleLevel;
    const uint32_t s5 = rd<uint32_t>(H, 4);
    struct Kind {
        uint32_t count_at, list_at, stride;
    };
    const Kind kinds[2] = {{0x28, 0x2c, 0x30}, {0x20, 0x24, 0x20}};  // shrubs, ties
    for (int k = 0; k < 2; ++k) {
        const uint32_t n = rd<uint32_t>(H, kinds[k].count_at), list = rd<uint32_t>(H, kinds[k].list_at);
        for (uint32_t i = 0; i < n; ++i) {
            const size_t r = static_cast<size_t>(list) + static_cast<size_t>(i) * kinds[k].stride;
            const uint32_t off = rd<uint32_t>(H, r);
            const int32_t o_class = rd<int32_t>(H, r + 4);
            if (off == 0 || static_cast<size_t>(s5) + off >= H.size()) continue;
            const std::vector<uint8_t> bytes(H.begin() + static_cast<long>(s5 + off), H.end());  // (the decoders bound every read by the size)
            std::array<uint8_t, 16> remap{};
            if (r + 0x20 <= H.size()) std::memcpy(remap.data(), H.data() + r + 0x10, 16);
            std::string e2;
            if (k == 0) {
                ShrubClass c;
                c.o_class = o_class;
                c.index = i;
                c.texture_remap = remap;
                if (!decode_shrub_class(bytes, c, e2)) {
                    err = "title shrub class " + std::to_string(o_class) + ": " + e2;
                    return false;
                }
                for (auto& pk : c.packets) {
                    for (auto& tp : pk.textures) tp.texture = tp.local_texture >= 0 && tp.local_texture < 16 ? c.texture_remap[static_cast<size_t>(tp.local_texture)] : -1;
                    std::string note;
                    if (!trace_shrub_packet(pk, note)) std::fprintf(stderr, "title shrub %d: kick trace: %s\n", c.o_class, note.c_str());
                }
                if (dump) std::printf("title shrub %d (#%u): %zu packets\n", c.o_class, i, c.packets.size());
                w.shrubs.push_back(std::move(c));
            } else {
                TieClass c;
                c.o_class = o_class;
                c.index = i;
                c.texture_remap = remap;
                if (!decode_tie_class(bytes, c, e2)) {
                    err = "title tie class " + std::to_string(o_class) + ": " + e2;
                    return false;
                }
                for (auto& tp : c.textures) tp.texture = tp.local_texture >= 0 && tp.local_texture < 16 ? c.texture_remap[static_cast<size_t>(tp.local_texture)] : -1;
                for (auto& pk : c.packets) {
                    std::string note;
                    if (!trace_tie_packet(pk, note)) std::fprintf(stderr, "title tie %d lod %d packet %d: kick trace: %s\n", c.o_class, pk.lod, pk.index_in_lod, note.c_str());
                }
                if (dump) std::printf("title tie %d (#%u): %zu packets, %zu prims\n", c.o_class, i, c.packets.size(), c.textures.size());
                w.ties.push_back(std::move(c));
            }
        }
    }
    // the tfrags: the block at s5 + H[+0x10] (the loader's 0x241458 = the overlays' tfrag setup, its texture table H + H[+0x34])
    const uint32_t tf = rd<uint32_t>(H, 0x10);
    if (dump) std::printf("title: H %zu bytes, s5 %x, tfrag block %x\n", H.size(), s5, tf);
    if (static_cast<size_t>(s5) + tf < H.size()) {  // (offset 0: the block starts the data, as in most levels' cores)
        const std::vector<uint8_t> tb(H.begin() + static_cast<long>(s5 + tf), H.end());
        if (!decode_tfrag_block(tb, w, err, dump)) return false;
    }
    return true;
}

bool export_level(const LevelSource& src, World& w, std::string& err, bool dump) {
    w.level = src.level;
    const auto& hdr = src.decoded.header;
    // the shrub classes (the core's table)
    std::map<uint32_t, const LevelCoreSubAsset*> by_offset;
    for (const auto& a : src.decoded.sub_assets) by_offset[a.offset] = &a;
    for (int32_t i = 0; i < hdr.shrub_classes.count; ++i) {
        ShrubClassRecord e;
        std::memcpy(&e, src.core_index.data() + hdr.shrub_classes.offset + static_cast<size_t>(i) * sizeof e, sizeof e);
        auto it = by_offset.find(static_cast<uint32_t>(e.data));
        if (it == by_offset.end()) continue;
        const LevelCoreSubAsset& a = *it->second;
        std::vector<uint8_t> bytes(src.core_data.begin() + a.offset, src.core_data.begin() + a.offset + a.size);
        if (a.packed) {
            std::vector<uint8_t> un;
            if (!decompress_wad(std::span<const std::byte>(reinterpret_cast<const std::byte*>(bytes.data()), bytes.size()), un)) {
                err = "shrub class " + std::to_string(e.class_id) + ": cannot decompress";
                return false;
            }
            bytes = std::move(un);
        }
        ShrubClass c;
        c.o_class = e.class_id;
        c.index = static_cast<uint32_t>(i);
        c.texture_remap = e.texture_remap;
        std::string e2;
        if (!decode_shrub_class(bytes, c, e2)) {
            err = "shrub class " + std::to_string(e.class_id) + ": " + e2;
            return false;
        }
        for (auto& pk : c.packets) {
            for (auto& tp : pk.textures) tp.texture = tp.local_texture >= 0 && tp.local_texture < 16 ? c.texture_remap[static_cast<size_t>(tp.local_texture)] : -1;
            std::string note;
            if (!trace_shrub_packet(pk, note)) std::fprintf(stderr, "level %d shrub %d: kick trace: %s\n", src.level, c.o_class, note.c_str());
        }
        if (dump) {
            std::printf("shrub %d (#%d): %zu packets\n", c.o_class, i, c.packets.size());
            for (size_t k = 0; k < c.packets.size(); ++k) {
                const auto& p = c.packets[k];
                size_t nv = 0, odd = 0;
                for (const auto& o : p.ops)
                    if (o.kind == OpKind::Vertex) {
                        const auto& v = p.verts[o.index];
                        ++nv;
                        if (v.pos != v.q || v.pos != v.uv) ++odd;
                    }
                std::printf("  packet %zu: header %d %d %d %d, ops %zu, kicked vertices %zu (pos / q / uv not the same vertex: %zu)%s%s\n", k, p.header[0], p.header[1],
                            p.header[2], p.header[3], p.ops.size(), nv, odd, p.kick_ok ? "" : " TRACE FAILED: ", p.kick_ok ? "" : p.kick_note.c_str());
            }
        }
        w.shrubs.push_back(std::move(c));
    }
    // the tfrags (the core's tfrag block: a header (table offset, count), 0x40-byte entries, their data)
    if (hdr.tfrag_block >= 0 && static_cast<size_t>(hdr.tfrag_block) < src.core_data.size()) {  // (the block is often at offset 0 of the core data)
        const auto* ta = [&]() -> const LevelCoreSubAsset* {
            for (const auto& a : src.decoded.sub_assets)
                if (a.kind == "tfrags") return &a;
            return nullptr;
        }();
        std::vector<uint8_t> tb;
        if (ta != nullptr) {
            tb.assign(src.core_data.begin() + ta->offset, src.core_data.begin() + ta->offset + ta->size);
            if (ta->packed) {
                std::vector<uint8_t> un;
                if (!decompress_wad(std::span<const std::byte>(reinterpret_cast<const std::byte*>(tb.data()), tb.size()), un)) {
                    err = "tfrags: cannot decompress";
                    return false;
                }
                tb = std::move(un);
            }
        } else {  // (offset 0: the core index decoder lists no "tfrags" sub-asset; the block is plain data from there)
            tb.assign(src.core_data.begin() + hdr.tfrag_block, src.core_data.end());
            if (tb.size() >= 3 && std::memcmp(tb.data(), "WAD", 3) == 0) {
                std::vector<uint8_t> un;
                if (!decompress_wad(std::span<const std::byte>(reinterpret_cast<const std::byte*>(tb.data()), tb.size()), un)) {
                    err = "tfrags: cannot decompress";
                    return false;
                }
                tb = std::move(un);
            }
        }
        if (!decode_tfrag_block(tb, w, err, dump)) return false;
    }
    // the tie classes
    for (int32_t i = 0; i < hdr.tie_classes.count; ++i) {
        ClassRecord e;
        std::memcpy(&e, src.core_index.data() + hdr.tie_classes.offset + static_cast<size_t>(i) * sizeof e, sizeof e);
        auto it = by_offset.find(static_cast<uint32_t>(e.data));
        if (it == by_offset.end()) continue;
        const LevelCoreSubAsset& a = *it->second;
        std::vector<uint8_t> bytes(src.core_data.begin() + a.offset, src.core_data.begin() + a.offset + a.size);
        if (a.packed) {
            std::vector<uint8_t> un;
            if (!decompress_wad(std::span<const std::byte>(reinterpret_cast<const std::byte*>(bytes.data()), bytes.size()), un)) {
                err = "tie class " + std::to_string(e.class_id) + ": cannot decompress";
                return false;
            }
            bytes = std::move(un);
        }
        TieClass c;
        c.o_class = e.class_id;
        c.index = static_cast<uint32_t>(i);
        c.texture_remap = e.texture_remap;
        std::string e2;
        if (!decode_tie_class(bytes, c, e2)) {
            err = "tie class " + std::to_string(e.class_id) + ": " + e2;
            return false;
        }
        for (auto& tp : c.textures) tp.texture = tp.local_texture >= 0 && tp.local_texture < 16 ? c.texture_remap[static_cast<size_t>(tp.local_texture)] : -1;
        for (auto& pk : c.packets) {
            std::string note;
            if (!trace_tie_packet(pk, note)) std::fprintf(stderr, "level %d tie %d lod %d packet %d: kick trace: %s\n", src.level, c.o_class, pk.lod, pk.index_in_lod, note.c_str());
        }
        if (dump) {
            std::printf("tie %d (#%d): %zu packets, %zu prims\n", c.o_class, i, c.packets.size(), c.textures.size());
            for (const auto& p : c.packets) {
                size_t nv = 0, odd = 0;
                for (const auto& o : p.ops)
                    if (o.kind == OpKind::Vertex) {
                        const auto& v = p.verts[o.index];
                        ++nv;
                        if (v.pos != v.q || v.pos != v.uv) ++odd;
                    }
                std::printf("  lod %d packet %d: dinky %u fat %u (vsize %d) idx %d, ops %zu, kicked vertices %zu (odd %zu)%s%s\n", p.lod, p.index_in_lod, p.n_dinky, p.n_fat,
                            p.entry[9], p.entry[0xb], p.ops.size(), nv, odd, p.kick_ok ? "" : " TRACE FAILED: ", p.kick_ok ? "" : p.kick_note.c_str());
            }
        }
        w.ties.push_back(std::move(c));
    }
    return true;
}

}  // namespace rac1::mrpw
