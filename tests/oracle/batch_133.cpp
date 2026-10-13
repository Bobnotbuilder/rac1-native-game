// Oracle batch 133 (task 0368b): the class-1471 effect manager's beams (fx1471_asm.cpp): the shooting robot 1440's beam (its start
// 0x2e2250, the draw callback 0x2e2af0 and its two helpers 0x2e3388 / 0x2e2c88, which build the beam's quads through 0x1fd228)
// (needs the ISO; ORACLE_LEVEL=N runs the level's own copies). On the level's data after its first gameplay frame: the three
// effect slots (state 0x161bf8, angles 0x161c08 / 0x161c18 / 0x161c28, lights 0x161c38 / 0x161c48, flags 0x161c58, 0x161c68,
// scroll 0x161c78, owner 0x161c88, the source matrix 0x1e6d20, the target 0x1e6de0, 0x1e6e10, the two 20-point strips per
// slot at 0x1e6e40) filled with beams from near the hero to a point a few units away, or left as the real start leaves them.

#include "ovl_oracle.h"

#include <game/ovl/fx1471.h>
#include <game/ovl/ovl_reloc.h>

#include <cmath>

namespace rac1::oracle {

void batch_133() {
    if (g_only_batch != 133) return;
    auto boot = [] { veldin_boot(); };
    g_batch = 133;
    g_ignored_words = vcross_lanes();
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // as in the runtime (null pointers read as RAM, as on the EE)
    static std::vector<uint8_t> snap;
    auto level = [boot] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            boot();
            level_setup_body(1, 0);
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    auto mobys_with = [](uint32_t fn) {
        std::vector<uint32_t> v;
        for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); m < ee_read<uint32_t>(OV(0x0015ffe0)); m += 0x100)
            if (ee_read<uint8_t>(m + 0x20) < 0xfe && ee_read<uint32_t>(m + 0x74) == fn) v.push_back(m);
        return v;
    };
    // the three slots: a beam each, from a point near the hero (the matrix's translation row) to a target 2..20 units away
    auto slots = [](bool strips) {
        const Vec4 h = ee_ref<Vec4>(0x0013f3d0);
        for (uint32_t i = 0; i < 3; ++i) {
            const uint32_t o = i * 4;
            w32(OV(0x00161bf8) + o, u(ri(0, 4)));
            wf(OV(0x00161c08) + o, rf(-3.2f, 3.2f));
            wf(OV(0x00161c18) + o, rf(-3.2f, 3.2f));
            wf(OV(0x00161c28) + o, rf(-3.2f, 3.2f));
            w32(OV(0x00161c38) + o, coin() ? 0xffffffffu : u(ri(0, 6)));
            w32(OV(0x00161c48) + o, coin() ? 0xffffffffu : u(ri(0, 6)));
            w32(OV(0x00161c58) + o, u(ri(0, 3)));
            w32(OV(0x00161c68) + o, u(ri(0, 3)));
            wf(OV(0x00161c78) + o, rf(-9, 9));
            w32(OV(0x00161c88) + o, ri(0, 3) == 0 ? 0u : 0x01851b80u + 0x100u * u(ri(0, 8)));
            const float a = rf(-3.2f, 3.2f), b = rf(-0.5f, 0.5f);
            const uint32_t mt = OV(0x001e6d20) + i * 0x40;
            const float ca = std::cos(a), sa = std::sin(a), cb = std::cos(b), sb = std::sin(b);
            const float rows[4][4] = {{ca * cb, sa * cb, sb, 0}, {-sa, ca, 0, 0}, {-ca * sb, -sa * sb, cb, 0},
                                      {h.x + rf(-4, 4), h.y + rf(-4, 4), h.z + rf(0, 3), 1}};
            for (uint32_t r = 0; r < 4; ++r)
                for (uint32_t c = 0; c < 4; ++c) wf(mt + r * 16 + c * 4, rows[r][c]);
            const float d = rf(2, 20), y = rf(-3.2f, 3.2f);
            const uint32_t tg = OV(0x001e6de0) + i * 0x10;
            wf(tg, rows[3][0] + d * std::cos(y));
            wf(tg + 4, rows[3][1] + d * std::sin(y));
            wf(tg + 8, rows[3][2] + rf(-2, 2));
            wf(tg + 12, 1.0f);
            wfrand(OV(0x001e6e10) + i * 0x10, 3, -1, 1);
            if (strips)
                for (uint32_t k = 0; k < 2; ++k)
                    for (uint32_t p = 0; p < 20; ++p) {
                        const uint32_t q = OV(0x001e6e40) + (i * 2 + k) * 0x140 + p * 0x10;
                        wf(q, rf(0, d));
                        wf(q + 4, rf(-0.3f, 0.3f));
                        wf(q + 8, rf(-0.3f, 0.3f));
                        wf(q + 12, rf(0, 1));
                    }
        }
        wf(OV(0x00166c00u - 0x5010), rf(0, 3));  // (0x161bf0: the lights' size)
    };
    // the start's 0x1e6e10[i] is a point through a matrix it builds on its stack from two VCROSS (0x1ff3c0) rows: its w lane comes from
    // the rows' stale w (undefined on the EE, 0 in the port); the particles 0x45 it spawns there copy that w (their position's w, +0x1c)
    const std::vector<uint32_t> ign0 = g_ignored_words;
    for (uint32_t i = 0; i < 3; ++i) g_ignored_words.push_back(OV(0x001e6e10) + i * 0x10 + 0xc);
    level();
    const uint32_t pool = ee_read<uint32_t>(OV(0x0016016c));  // (the particle records, 0x40 bytes each)
    for (uint32_t k = 0; k < 0x800; ++k) g_ignored_words.push_back(pool + k * 0x40 + 0x1c);
    check("fx1471_effect_start", 0x002e2250, 300, Ret::None, [level, slots] {
        level();
        slots(coin());
        const uint32_t i = u(ri(0, 2));
        if (coin()) w32(OV(0x00161bf8) + i * 4, coin() ? 2u : 3u);  // (the start's own cases)
        seed_rand();
        return Args{{ee_read<uint32_t>(OV(0x00161c88) + i * 4), i}, {}};
    }, [](A a) { fx1471_effect_start(a.i[0], a.i[1]); return uint64_t{0}; });
    g_ignored_words = ign0;
    check("fx1471_beam_prepare", 0x002e3388, 300, Ret::None, [level, slots] {
        level();
        slots(true);
        const uint32_t i = u(ri(0, 2));
        seed_rand();
        return Args{{ee_read<uint32_t>(OV(0x00161c88) + i * 4), i}, {}};
    }, [](A a) { fx1471_beam_prepare(a.i[0], a.i[1]); return uint64_t{0}; });
    level();
    const uint32_t mgr_fn = g_oracle_level == 0 ? 0x002e1df0u : ovl_code_lookup(0x002e1df0u);  // (the manager's update; 0: none on the level)
    std::vector<uint32_t> mgr = mgr_fn != 0 ? mobys_with(mgr_fn) : std::vector<uint32_t>{};
    std::printf("  class-1471 managers: %zu\n", mgr.size());
    if (mgr.empty()) {
        skip_check("fx1471_draw", 0x002e2af0, "no class-1471 manager on this level");
    } else {
        check("fx1471_draw", 0x002e2af0, 300, Ret::None, [level, slots, mgr] {
            level();
            slots(true);
            if (coin()) {  // (the slots as the start leaves them, from the original's own start)
                for (uint32_t i = 0; i < 3; ++i)
                    if (ee_read<uint32_t>(OV(0x00161bf8) + i * 4) >= 2) fx1471_effect_start(ee_read<uint32_t>(OV(0x00161c88) + i * 4), i);
            }
            seed_rand();
            return Args{{mgr[0]}, {}};
        }, [](A a) { fx1471_draw(a.i[0]); return uint64_t{0}; });
    }
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
