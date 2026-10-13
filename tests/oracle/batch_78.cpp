// Oracle batch 78: the level's shadow meshes 0x1fca48 (EE side 0x1fe6d8 / 0x1fc740 / 0x1fe08c /
// 0x2a0eb8, and the VU1 program they start) on Veldin's real level data (needs the ISO): its 8
// shadow records and meshes, moved about the view (in front, across the screen's edges, behind,
// near, far), with random lights.

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/shadows.h>
#include <game/ovl/shadows_vu1.h>
#include <runtime/gs/gs_emu.h>
#include <runtime/vu/vu1.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace rac1::oracle {

void batch_78() {
    if (g_only_batch != 78) return;
    g_batch = 78;
    g_ignored_words = vcross_lanes();
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    static std::vector<uint8_t> snap;
    auto level1 = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            particle_table_init();
            level_setup_body(1, 0);
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            w16(0x001518d0, 512);  // the display as the frame sets it (512 x 416)
            w16(0x001518d2, 416);
            screen_setup();
            projection_setup();
            render_view_setup();
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    if (std::getenv("ORACLE_SHADOW_INFO") != nullptr) {  // Veldin's shadow records and meshes
        level1();
        const int32_t n = ee_read<int32_t>(OV(0x0015f438));
        const uint32_t rec = ee_read<uint32_t>(OV(0x0015f43c)), tab = ee_read<uint32_t>(OV(0x0015f440));
        std::printf("shadows %d rec %08x tab %08x cam %g %g %g palette word %08x\n", n, rec, tab, ee_read<float>(OV(0x00166dc0)), ee_read<float>(OV(0x00166dc4)),
                    ee_read<float>(OV(0x00166dc8)), ee_read<uint32_t>(OV(0x0016e480)));
        std::printf("palette 0x16e480:");
        for (uint32_t k = 0; k < 256; k += 8) std::printf(" %08x", ee_read<uint32_t>(OV(0x0016e480) + 4 * k));
        std::printf("\n");
        for (int32_t i = 0; i < n && i < 400; ++i) {
            const uint32_t r = rec + 0x20 * u(i), m = ee_read<uint32_t>(tab + 4 * u(i));
            const uint32_t t = ee_read<uint32_t>(m + 4);
            std::printf(" %d pos %.2f %.2f %.2f c %g fade %g | mesh %08x w0 %08x tex %dx%d batches:", i, ee_read<float>(r), ee_read<float>(r + 4), ee_read<float>(r + 8),
                        ee_read<float>(r + 0xc), ee_read<float>(r + 0x10), m, ee_read<uint32_t>(m), 1 << ee_read<int32_t>(t), 1 << ee_read<int32_t>(t + 4));
            uint32_t a = m + 8;
            for (uint32_t b = 0; b < (ee_read<uint32_t>(m) >> 16) && b < 64; ++b) {
                a = (a + 0xf) & ~0xfu;
                const uint32_t c = ee_read<uint32_t>(a);
                std::printf(" %u", c);
                a += 4 + u((static_cast<int32_t>(c) >> 1) * 0xc);
                a = (a + 0xf) & ~0xfu;
                a += c * 4;
            }
            std::printf("\n");
        }
    }
    // the view's axes in the world (columns of 0x166c80) and the camera
    auto place = [](uint32_t a, float d, float x, float y) {
        const float cx = ee_read<float>(OV(0x00166dc0)), cy = ee_read<float>(OV(0x00166dc4)), cz = ee_read<float>(OV(0x00166dc8));
        float fw[3], rt[3], up[3];
        for (uint32_t k = 0; k < 3; ++k) {
            rt[k] = ee_read<float>(OV(0x00166c80) + 0x10 * k);
            up[k] = ee_read<float>(OV(0x00166c84) + 0x10 * k);
            fw[k] = ee_read<float>(OV(0x00166c88) + 0x10 * k);
        }
        wf(a, cx + fw[0] * d + rt[0] * x + up[0] * y);
        wf(a + 4, cy + fw[1] * d + rt[1] * x + up[1] * y);
        wf(a + 8, cz + fw[2] * d + rt[2] * x + up[2] * y);
    };
    // the records moved about the view, the lights near them, the packet ring, the texture queue
    auto shadows = [level1, place] {
        level1();
        const uint32_t rec = ee_read<uint32_t>(OV(0x0015f43c));
        w32(OV(0x0015f438), ri(0, 10) == 0 ? 0u : u(ri(1, 8)));
        for (uint32_t i = 0; i < 8; ++i) {
            const uint32_t r = rec + 0x20 * i;
            const int where = ri(0, 9);
            const float d = where == 0 ? rf(-40, 0) : where == 1 ? rf(0, 4) : where == 2 ? rf(40, 150) : rf(4, 40);
            const float side = where == 3 ? rf(0.5f, 1.5f) * std::fabs(d) + 20 : rf(0, 0.6f) * std::fabs(d);
            if (ri(0, 4) != 0) place(r, d, rf(-side, side), rf(-side, side) * 0.7f);
            wf(r + 0xc, rf(3, 15));
            wf(r + 0x10, ri(0, 3) == 0 ? rf(0, 10) : rf(10, 80));
            wrand(r + 0x14, 3);
        }
        for (uint32_t k = 0; k < 8; ++k) {
            const uint32_t l = OV(0x001802c0) + 0x20 * k;
            if (ri(0, 2) == 0) {
                wrand(l, 4);
                w32(l + 0x10, 0);
                w32(l + 0x14, 0);
                wrand(l + 0x18, 2);
                continue;
            }
            wfrand(l, 3, -0.3f, 2.5f);
            wrand(l + 0xc, 1);
            const uint32_t r = rec + 0x20 * u(ri(0, 7));
            for (uint32_t c = 0; c < 3; ++c) wf(l + 0x10 + 4 * c, ee_read<float>(r + 4 * c) + rf(-25, 25));
            wf(l + 0x1c, ri(0, 8) == 0 ? 0.0f : rf(1, 40));
        }
        w32(OV(0x0015f5e0), coin() ? 7u : u(ri(0, 8)));
        w32(OV(0x0015f418), u(ri(0, 8)));
        w32(0x0015ee74, u(ri(0x1000, 0x40000)) & ~0xffu);
        w32(OV(0x001611c0), 0x01e10000u + 0x10 * u(ri(0, 0x100)));
    };
    auto set_s6 = [](uint32_t s6) {
        g_pre_call = [s6](EeInterpreter& ee) { ee.cpu().set32(22, static_cast<int32_t>(s6)); };
    };
    check("packet_ref_13cf10", 0x002a0eb8, 20, Ret::None, [level1] {
        level1();
        w32(OV(0x001611c0), 0x01e10000u + 0x10 * u(ri(0, 0x100)));
        return Args{{}, {}};
    }, [](A) { packet_ref_13cf10(); return uint64_t{0}; });
    check("shadows_vu1_setup", 0x001fc740, 100, Ret::None, [shadows] {
        shadows();
        wf(OV(0x0015f308), coin() ? 0.0f : rf(-0.1f, 0.1f));
        return Args{{}, {}};
    }, [](A) { shadows_vu1_setup(); return uint64_t{0}; });
    {
        uint32_t s6 = 0;
        check("shadows_prepare", 0x001fe6d8, 300, Ret::None, [shadows, set_s6, &s6] {
            shadows();
            s6 = coin() ? 0x00150000u : R();
            set_s6(s6);
            if (ee_read<int32_t>(OV(0x0015f438)) == 0) w32(OV(0x0015f438), 1);  // (a do-while: 0 is the caller's test)
            return Args{{}, {}};
        }, [&s6](A) { shadows_prepare(s6); return uint64_t{0}; });
    }
    // the per-mesh packets: the records' alpha / clip bit and TEX0 as 0x1fe6d8 leaves them, the
    // lights on the scratchpad; t2 and vf5 as the caller leaves them
    check("shadows_packets", 0x001fe08c, 300, Ret::None, [shadows] {
        shadows();
        const uint32_t rec = ee_read<uint32_t>(OV(0x0015f43c));
        if (ee_read<int32_t>(OV(0x0015f438)) == 0) w32(OV(0x0015f438), 1);
        for (uint32_t i = 0; i < 8; ++i) {
            const uint32_t r = rec + 0x20 * i;
            const int a = ri(0, 5);
            w32(r + 0x14, (a == 0 ? 0u : a == 1 ? R() & 0xffffu : a == 2 ? 0xffu : u(ri(1, 254))) | (coin() ? 0x80000000u : 0u) | (ri(0, 4) == 0 ? (R() & 0x7fff0000u) : 0u));
            wrand(r + 0x18, 2);
        }
        uint8_t* spr = ee_get_scratchpad_base();
        std::memcpy(spr + 0x2400, ee_ram_ptr(OV(0x001802c0)), 0x100);
        g_pre_call = [](EeInterpreter& ee) {
            ee.cpu().set32(10, OV(0x0016e480));  // t2 as 0x1fe6d8 leaves it
            ee.vu0().vf[5].u[0] = ee_read<uint32_t>(OV(0x00166d50));  // vf5.x as 0x1ffdd8 leaves it
        };
        return Args{{}, {}};
    }, [](A) { shadows_packets(); return uint64_t{0}; });
    {
        uint32_t s6 = 0;
        check("shadows_draw", 0x001fca48, 300, Ret::None, [shadows, set_s6, &s6] {
            shadows();
            s6 = ri(0, 3) == 0 ? R() : 0x00150000u;
            set_s6(s6);
            return Args{{}, {}};
        }, [&s6](A) { shadows_draw(s6); return uint64_t{0}; });
    }
    g_pre_call = nullptr;
    // The VU1 program: the chain the (proved) draw wrote, walked from the same random VU1 state
    // once with the VU interpreter running the uploaded original and once with the port; the
    // kicked GIF packets, the VU1 data memory and registers (vf, vi, ACC, Q, I, clip flags; not
    // the MAC / status flags) compared.
    if (std::getenv("ORACLE_ONLY") == nullptr || std::string(std::getenv("ORACLE_ONLY")) == "shadows_vu1") {
        shadows_vu1_register();
        const int cases = 300;
        int ok = 0, shown = 0, kicks = 0, clipped = 0;
        for (int c = 0; c < cases; ++c) {
            shadows();
            if (ee_read<int32_t>(OV(0x0015f438)) == 0) w32(OV(0x0015f438), u(ri(1, 8)));
            w32(OV(0x0015f5e0), 0);  // the program uploaded in the chain
            const uint32_t ring0 = ee_read<uint32_t>(OV(0x001611c0));
            shadows_draw(0x00150000);
            const uint32_t ring1 = ee_read<uint32_t>(OV(0x001611c0));
            vu1::Vu1 init;
            for (auto& v : init.st.vf) for (uint32_t& w : v.u) w = R();
            if (coin())  // plausible clip positions (the first triangle's stale vertices)
                for (int r = 26; r <= 28; ++r) for (int l = 0; l < 4; ++l) init.st.vf[r].f[l] = rf(-300, 300);
            for (auto& v : init.st.vi) v = static_cast<int16_t>(R());
            for (uint32_t& w : init.st.acc.u) w = R();
            init.st.q = std::bit_cast<float>(R() & 0x7fffffffu);
            init.st.i = std::bit_cast<float>(R() & 0x7fffffffu);
            init.st.clipflag = R() & 0xffffff;
            for (size_t k = 0; k < init.st.mem.size(); k += 4) { const uint32_t r = R(); std::memcpy(&init.st.mem[k], &r, 4); }
            init.st.vf[0].u[0] = init.st.vf[0].u[1] = init.st.vf[0].u[2] = 0;
            init.st.vf[0].f[3] = 1.0f;
            init.st.vi[0] = 0;
            init.dbf = coin();
            auto run = [&](bool interp, std::vector<uint8_t>& out) {
                vu1::vu1_global() = init;
                vu1::g_force_interpreter = interp;
                vu1::g_kick_tap = [&out](const uint8_t* d, uint32_t q) { out.insert(out.end(), d, d + q * 16); };
                if (ring1 != ring0) gs::gs_global().vif1_chain(ring0, ring1);
                vu1::g_kick_tap = nullptr;
                vu1::g_force_interpreter = false;
                return vu1::vu1_global();
            };
            std::vector<uint8_t> ka, kb;
            const vu1::Vu1 a = run(true, ka);
            const vu1::Vu1 b = run(false, kb);
            if (!ka.empty()) ++kicks;
            for (size_t k = 0; k + 16 <= ka.size(); k += 16) {  // a fan's GIF tag (PRIM 0x7d) with vertices kicked: the clipper ran
                uint32_t w0, w1;
                std::memcpy(&w0, &ka[k], 4);
                std::memcpy(&w1, &ka[k + 4], 4);
                if (w1 == 0x303ec000u && (w0 & 0x7fff) != 0) { ++clipped; break; }
            }
            std::string why;
            if (ka != kb) why = "kicked packets (" + std::to_string(ka.size()) + " / " + std::to_string(kb.size()) + " bytes)";
            else if (a.st.mem != b.st.mem) why = "data memory";
            else if (std::memcmp(a.st.vf.data(), b.st.vf.data(), sizeof(a.st.vf)) != 0) why = "vf";
            else if (a.st.vi != b.st.vi) why = "vi";
            else if (std::memcmp(a.st.acc.u, b.st.acc.u, 16) != 0) why = "acc";
            else if (std::memcmp(&a.st.q, &b.st.q, 4) != 0) why = "q";
            else if (std::memcmp(&a.st.i, &b.st.i, 4) != 0) why = "i";
            else if (a.st.clipflag != b.st.clipflag) why = "clip flags";
            if (why.empty()) { ++ok; continue; }
            if (shown++ < 3) {
                std::printf("  [shadows_vu1] case %d: %s differ\n", c, why.c_str());
                if (ka != kb)
                    for (size_t k = 0; k < std::min(ka.size(), kb.size()); k += 4)
                        if (std::memcmp(&ka[k], &kb[k], 4) != 0) {
                            uint32_t x, y;
                            std::memcpy(&x, &ka[k], 4);
                            std::memcpy(&y, &kb[k], 4);
                            std::printf("    first at byte %zu: %08x vs %08x\n", k, x, y);
                            break;
                        }
                for (int r = 0; r < 32 && why == "vf"; ++r)
                    for (int l = 0; l < 4; ++l)
                        if (a.st.vf[r].u[l] != b.st.vf[r].u[l]) std::printf("    vf%d.%c %08x vs %08x\n", r, "xyzw"[l], a.st.vf[r].u[l], b.st.vf[r].u[l]);
                for (int r = 0; r < 16 && why == "vi"; ++r)
                    if (a.st.vi[r] != b.st.vi[r]) std::printf("    vi%d %04x vs %04x\n", r, static_cast<uint16_t>(a.st.vi[r]), static_cast<uint16_t>(b.st.vi[r]));
                for (size_t k = 0; k < a.st.mem.size() && why == "data memory"; k += 4)
                    if (std::memcmp(&a.st.mem[k], &b.st.mem[k], 4) != 0) { std::printf("    first at %zx\n", k); break; }
                if (why == "clip flags") std::printf("    %06x vs %06x\n", a.st.clipflag, b.st.clipflag);
            }
        }
        std::printf("%-26s 0x%08x  %s (%d/%d, %d with kicks, %d clipped)\n", "shadows_vu1", 0x0010e818u, ok == cases ? "ok" : "FAIL", ok, cases, kicks, clipped);
        if (ok != cases) ++g_fail;
    }
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
