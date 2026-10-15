// Oracle batch 201 (levels/pokitaru-hoven, release 0.3.0): particle type 0x49's update (Veldin's 0x275320, slot 0x1b1fa4 of the
// particle table; L12:0x28dca8), first reached unported on Hoven (a `TODO_PORT ... moby callback` in every headless Hoven run),
// transliterated at Veldin's addresses (particle_49_tr.cpp, group veldin_particle49). Run with ORACLE_LEVEL=0 (Veldin's own original)
// and ORACLE_LEVEL=12 (Hoven's copy, the port relocated): build/tests/ovl_oracle --batch 201 (needs the ISO; no RAM dumps).
// The subject is a particle of the type allocated in the level's pool after its first gameplay frame (the port's), its position
// (inside the level's 512 x 512 box or out of it), velocity, ground height (+0x2c) and frame scale varied.
#include "ovl_oracle.h"

#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/particle_49_tr.h>

#include <cstring>
#include <vector>

namespace rac1::oracle {

void batch_201() {
    if (g_only_batch != 201) return;
    g_batch = 201;
    const std::vector<uint32_t> saved_ignored = g_ignored_words;
    g_ignored_words = vcross_lanes();
    g_ignored_words.push_back(OV(0x00160188));  // (the particle update's saved return address)
    g_ignored_words.push_back(OV(0x0016018c));
    for (uint32_t a = 0; a < 0x100; a += 4) g_ignored_words.push_back(a);  // (the null page: a stale pointer of a random state)
    g_extra_noop_hooks.clear();
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // as in the runtime

    static std::vector<uint8_t> snap;
    static int snap_level = -1;
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty() || snap_level != g_oracle_level) {
            veldin_boot();
            level_setup_body(1, 0);
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            snap.assign(ram, ram + 0x02000000);
            snap_level = g_oracle_level;
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    // a random state that sends the port through a pointer outside RAM is not a game state: dealt again (as batch 160's)
    auto retry = [](const std::function<uint32_t()>& setup, void (*fn)(uint32_t)) {
        static std::vector<uint8_t> saved;
        uint32_t q = 0;
        for (int attempt = 0; attempt < 40; ++attempt) {
            q = setup();
            saved.assign(ee_get_ram_base(), ee_get_ram_base() + 0x02000000);
            bool ok = true;
            const uint32_t stack_top = ee_stack_ptr();
            try { fn(q); } catch (const std::exception&) { ok = false; }
            ee_stack_ptr() = stack_top;
            std::memcpy(ee_get_ram_base(), saved.data(), saved.size());
            if (ok) break;
        }
        return q;
    };
    auto setup = [level] {
        level();
        const uint32_t q = alloc_particle(0x49);
        w16(q + 0xa, u(ri(0, 5) == 0 ? ri(-1, 2) : ri(3, 200)));
        w8(q + 2, u(ri(0, 3)));
        w8(q + 8, u(ri(0, 0xff)));
        const Vec4 h = ee_ref<Vec4>(0x0013f3d0);  // (about the hero, on the level's ground, or anywhere / out of the 0..512 box)
        if (ri(0, 2) != 0) VV(q + 0x10) = Vec4{h.x + rf(-20, 20), h.y + rf(-20, 20), h.z + rf(-5, 20), 1.0f};
        else wfrand(q + 0x10, 3, -40, 560);
        wfrand(q + 0x20, 3, -3, 3);
        wf(q + 0x2c, ri(0, 2) == 0 ? ee_read<float>(q + 0x18) + rf(-3, 3) : rf(-50, 300));
        wfrand(q + 0x30, 4, -2, 2);
        wf(0x0015ed64, ri(0, 3) == 0 ? rf(0.5f, 3.0f) : 1.0f);
        wf(0x0015ed6c, ri(0, 3) == 0 ? rf(0.5f, 3.0f) : 1.0f);
        seed_rand();
        return q;
    };
    if (g_oracle_level != 0 && g_oracle_level != 12) {
        std::printf("%-26s skipped (Veldin's code checked on levels 0 and 12: ORACLE_LEVEL=0 / 12)\n", "particle_49_update");
    } else {
        check("particle_49_update", 0x00275320, 300, Ret::None, [setup, retry] { return Args{{retry(setup, particle_49_update)}, {}}; },
              [](A a) { particle_49_update(a.i[0]); return uint64_t{0}; });
    }

    g_ignored_words = saved_ignored;
    g_ee_null_is_ram = null_ram;
    g_extra_noop_hooks.clear();
}

}  // namespace rac1::oracle
