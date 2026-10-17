// Oracle batch 410 (issue #42, package P4): the levels' own builds of engine functions that a Veldin port stands in for, each
// against the level's own copy on its own overlay. Run with ORACLE_LEVEL=<n> build/tests/ovl_oracle --batch 410 (needs the ISO; no
// dumps: states built from the level), on levels 0, 2, 3, 10 and 13 (each check says where it runs; elsewhere it is skipped):
//   - find_rail (0x20cd08; Kerwan's own build L3:0x00205830, other thresholds: find_rail_l3_v) on synthetic rail tables, the
//     points spread across both builds' thresholds, the sub-state and PLAYER+0x1ca varied: levels 0 and 3;
//   - hero_rail_grab_check (0x20d330; Kerwan's L3:0x00205a68, its only caller) with rails around the hand point: level 3;
//   - cutscene_level_extras (0x23e268; Aridia's L2:0x002400c8, Kerwan's L3:0x0022c728: their level's branch only) on the
//     level's own mobys as the scene's, scene 5 around frames 900..1100 (Kerwan's sparks): levels 0, 2 and 3;
//   - the hit record 0x259bc8 (Gemlik's L13:0x00265c30, Orxon's L10:0x0024c6b8: the same instructions, the levels' own ring
//     and counter): levels 0, 10 and 13.
// The level is the port's after its first gameplay frame (as batches 112 / 240).
#include "ovl_oracle.h"

#include <game/ovl/cutscene.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <runtime/kernel/disc.h>

#include <cstring>

namespace rac1::oracle {

namespace {
// a check run only on the levels listed (the others: skipped and counted)
void on_levels(std::initializer_list<int> lv, const char* name, uint32_t orig, int n, Ret ret, const std::function<Args()>& gen,
               const std::function<uint64_t(const Args&)>& port) {
    if (std::find(lv.begin(), lv.end(), g_oracle_level) == lv.end()) {
        skip_check(name, orig, "not this level's build");
        return;
    }
    check(name, orig, n, ret, gen, port);
}
}  // namespace

void batch_410() {
    if (g_only_batch != 410) return;
    g_batch = 410;
    const std::vector<uint32_t> saved_ignored = g_ignored_words;
    g_ignored_words = vcross_lanes();
    g_ignored_words.push_back(OV(0x00160188));  // the particle update's saved return address
    g_ignored_words.push_back(OV(0x0016018c));
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // as in the runtime
    // 989snd's IOP server (as batch 240's): the stream system's disc reads done, the other commands answered by a function of them
    sif_rpc_register(0x123456, [](uint32_t fno, uint32_t send, uint32_t, uint32_t recv, uint32_t) {
        auto result = [](uint32_t id, uint32_t a0) -> uint32_t {
            const uint32_t h = (id * 0x9e3779b1u) ^ (a0 * 0x85ebca6bu);
            return ((h >> 7) % 4 == 0) ? 0u : ((h >> 3) | 1u);
        };
        if (fno == 0x4d) {
            const uint32_t n = ee_read<uint32_t>(send);
            uint32_t p = send + 4;
            for (uint32_t i = 0; i < n; ++i) {
                const uint32_t id = ee_read<uint16_t>(p), size = ee_read<uint16_t>(p + 2);
                ee_write<uint32_t>(recv + 4 + i * 4, result(id, size != 0 ? ee_read<uint32_t>(p + 4) : 0u));
                if (id == 0x38) {
                    disc_read_ee(ee_read<uint32_t>(p + 0xc), ee_read<uint32_t>(p + 4), ee_read<uint32_t>(p + 8));
                    ee_write<uint32_t>(0x00137b10, 0);
                    ee_write<uint32_t>(0x00137b00, 0);
                }
                p += 4 + ((size + 3) & ~3u);
            }
            ee_write<uint32_t>(recv, 0xffffffffu);
            ee_write<uint32_t>(recv + 4 + n * 4, 0xffffffffu);
            return;
        }
        ee_write<uint32_t>(recv + 4, result(fno, send != 0 ? ee_read<uint32_t>(send) : 0u));
        ee_write<uint32_t>(recv, 0xffffffffu);
        ee_write<uint32_t>(recv + 8, 0xffffffffu);
    });

    // the level after its first gameplay frame (the port's)
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
    // the level's live mobys
    auto live_mobys = [] {
        std::vector<uint32_t> v;
        for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); m < ee_read<uint32_t>(OV(0x0015ffe0)); m += 0x100)
            if (ee_read<uint8_t>(m + 0x20) < 0xfe && ee_read<uint32_t>(m + 0x24) != 0) v.push_back(m);
        return v;
    };
    // a rail table of up to 4 rails (0x20 each: centre, radius +0xc, path +0x10, loop flag +0x14) whose path points lie around
    // `c`, spread past both builds' xy (0.3 / 0.9 / 1.4 / 1.7) and z (1.5 / 10) thresholds
    auto rails_around = [](const Vec4& c) {
        const int nr = ri(0, 4);
        w32(OV(0x0015f710), u(nr));
        w32(OV(0x0015f70c), T3);
        for (int r = 0; r < 4; ++r) {
            const uint32_t e = T3 + 0x20 * u(r), path = T + 0x100 * u(r);
            wf(e, c.x + rf(-3, 3)); wf(e + 4, c.y + rf(-3, 3)); wf(e + 8, c.z + rf(-3, 3)); wf(e + 0xc, ri(0, 5) == 0 ? rf(0, 3) : rf(4, 14));
            w32(e + 0x10, path); w32(e + 0x14, u(ri(0, 1)));
            const int n = ri(0, 8) == 0 ? 0 : ri(2, 6);
            w32(path, u(n));
            const float spread = ri(0, 1) ? 1.2f : 2.4f;
            for (int i = 0; i < n; ++i) {
                wf(path + 0x10 + 16 * i, c.x + rf(-spread, spread)); wf(path + 0x14 + 16 * i, c.y + rf(-spread, spread));
                wf(path + 0x18 + 16 * i, c.z + (ri(0, 3) == 0 ? rf(-12, 12) : rf(-2, 2))); wf(path + 0x1c + 16 * i, rf(0.5f, 3));
            }
        }
    };

    // ---- the rail search: Veldin's build (level 0) and Kerwan's own (level 3)
    on_levels({0, 3}, "find_rail", 0x0020cd08, 600, Ret::Int32, [rails_around] {
        restore_baseline();
        wfrand(M, 4, -3, 3);
        rails_around(ee_ref<Vec4>(M));
        w32(g::PLAYER + 0x208c, u(ri(0, 5))); w16(g::PLAYER + 0x1ca, u(ri(0, 1)));
        wrand(OUT, 12);
        const uint32_t skip = ri(0, 3) ? 0u : T + 0x100 * u(ri(0, 3)), only = ri(0, 3) ? 0u : T + 0x100 * u(ri(0, 3));
        return Args{{M, OUT, OUT + 0x10, OUT + 0x20, OUT + 0x24, OUT + 0x28, skip, only}, {}};
    }, [](A a) { return ri32(find_rail(a.i[0], a.i[1], a.i[2], a.i[3], a.i[4], a.i[5], a.i[6], a.i[7])); });

    // ---- Kerwan's rail grab check (the hand point's rail, its only call of the rail search)
    on_levels({3}, "hero_rail_grab_check", 0x0020d330, 400, Ret::None, [level, rails_around] {
        level();
        const uint32_t P = g::PLAYER;
        w16(P + 0x1e4, ri(0, 6) == 0 ? 1u : 0u);
        { static const uint32_t subs[] = {2, 2, 4, 4, 0, 1, 3}; w32(P + 0x208c, subs[ri(0, 6)]); }
        w16(P + 0x41e, ri(0, 2) == 0 ? 1u : 0u); w16(P + 0x1ca, u(ri(0, 1)));
        w32(0x0013cae4, ee_read<uint32_t>(0x0013cae4) ^ (coin() ? 0x80u : 0u));
        Vec4 hand;
        sub_0020d2f8_v(hand);
        rails_around(hand);
        return Args{};
    }, [](A) { hero_rail_grab_check(); return uint64_t{0}; });

    // ---- the scenes' level extras: Veldin's, Aridia's and Kerwan's builds, the level's own mobys as the scene's
    on_levels({0, 2, 3}, "cutscene_level_extras", 0x0023e268, 300, Ret::None, [level, live_mobys] {
        level();
        const std::vector<uint32_t> ms = live_mobys();
        constexpr uint32_t SCN = 0x0016c860;
        const int n = ms.empty() ? 0 : ri(0, 8);
        w16(OV(SCN) + 0x44, u(n));
        for (int i = 0; i < 8; ++i) {
            const uint32_t m = ms.empty() ? 0u : ms[static_cast<size_t>(ri(0, static_cast<int>(ms.size()) - 1))];
            w32(OV(SCN) + 0x178 + 4 * u(i), m);
            if (m != 0 && ri(0, 3) == 0) w16(m + 0xa6, coin() ? 0x1b1u : 0x212u);  // (the classes the branches look for)
        }
        if (ri(0, 4) == 0) w32(OV(SCN) + 0x184, 0);
        w32(OV(SCN) + 0x30, ri(0, 3) ? 5u : u(ri(0, 7)));
        w32(OV(SCN) + 0x34, u(ri(0, 3) ? ri(880, 1120) : ri(0, 1400)));
        seed_rand();
        return Args{};
    }, [](A) { cutscene_level_extras(); return uint64_t{0}; });

    // ---- the hit record: Veldin's, Orxon's and Gemlik's builds (the level's own ring 0x178100 and counter 0x173e54)
    on_levels({0, 10, 13}, "hit_record", 0x00259bc8, 300, Ret::None, [] {
        restore_baseline();
        wrand(M, 0x40);
        const uint32_t ring = OV(0x00178100);
        for (uint32_t k = 0; k < 64; ++k) {
            wrand(ring + k * 0x40, 0x10);
            wf(ring + k * 0x40 + 0x2c, rf(0, 4));
            if (ri(0, 2) == 0) w32(ring + k * 0x40 + 0x34, M);
        }
        w8(M + 0xa4, ri(0, 3) == 0 ? 0xffu : u(ri(0, 63)));
        w32(OV(0x00173e54), u(ri(0, 63)));
        wfrand(T, 4, -50, 50);
        if (ri(0, 4) == 0) wfrand(T + 0x10, 4, -0.001f, 0.001f); else wfrand(T + 0x10, 4, -3, 3);
        return Args{{M, R(), R(), T, T + 0x10}, {rf(0, 4)}};
    }, [](A a) { sub_00259bc8(a.i[0], a.i[1], a.i[2], a.i[3], a.i[4], a.f[0]); return uint64_t{0}; });

    //@@NEXT
    g_ignored_words = saved_ignored;
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
