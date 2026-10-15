// Oracle batch 203 (levels/pokitaru-hoven, release 0.3.0): the PDA, hero item 0x20, used from hero_item_use (Veldin 0x227fa0): with the
// item in hand and its button pressed (in movement modes 0..2 / 4 / 5) it starts the vendor's screens with no Vendor moby (Veldin
// 0x29a6b0, vendor_screen_start_tr). Levels 8, 11, 12 and 14 run their own builds of hero_item_use (batalia_hero_tr_3.cpp,
// pokitaru_hero_tr_3.cpp, hoven_hero_tr_3.cpp, oltanis_hero_tr_3.cpp), whose call of the level's copy of 0x29a6b0 (L8:0x2a2ad0, L11:0x2bd5a8, L12:0x2b0f38, L14:0x2a0a58) was a
// TODO_PORT stub; it is now the vendor port (a glue line in each group). The unit batch 48 cannot check this item: its memory has no
// level, so the vendor screens' start spawns no moby. Here the state is the level after its first gameplay frame (the port's), the hero
// standing; the original is the level's own hero_item_use (or Veldin's on level 0). Run with ORACLE_LEVEL=0 / 8 / 11 / 12 / 13 / 14
// build/tests/ovl_oracle --batch 203 (needs the ISO; no dumps).
#include "ovl_oracle.h"

#include <game/ovl/globals.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>

#include <cstring>
#include <vector>

namespace rac1::oracle {

void batch_203() {
    if (g_only_batch != 203) return;
    g_batch = 203;
    const std::vector<uint32_t> saved_ignored = g_ignored_words;
    g_ignored_words = vcross_lanes();
    g_ignored_words.push_back(OV(0x00160188));  // (the particle update's saved return address)
    g_ignored_words.push_back(OV(0x0016018c));
    for (uint32_t a = 0; a < 0x100; a += 4) g_ignored_words.push_back(a);  // (the null page)
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // as in the runtime
    g_extra_noop_hooks = {0x00122298, 0x00120478, 0x001232c8, 0x00122fe0, 0x00118a80};  // (the kernel: as batch 124's)
    static std::vector<uint8_t> snap;
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            level_setup_body(1, 0);
            w32(OV(0x0016c060), 0xf);
            if (g_oracle_level == 14) w8(0x0013d3f0, 1);  // (Oltanis: the arrival scene seen, as batch 221: class 684 would start scene 0, its stream read from the disc)
            gameplay_frame();
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    constexpr uint32_t P = g::PLAYER, PAD = g::PAD;
    check("hero_item_use/pda", 0x00227fa0, 120, Ret::Int32, [level] {
        level();
        w32(P + 0x10b8, 0x20); w32(P + 0x10b4, 2); w8(P + 0x20a4, 0); w8(P + 0x20ac, 0);
        w32(P + 0x1090, ri(0, 7) ? ee_read<uint32_t>(P + 0x2080) : 0u);  // (the item in hand's moby: the hero's own stands in, as no PDA moby is spawned in the snapshot; 0: the early exit)
        static const uint32_t md[] = {0, 1, 2, 4, 5, 3, 7, 6};  // (3 / 6 / 7: no vendor)
        w32(P + 0x208c, md[ri(0, 7)]);
        const uint32_t mask = ri(0, 1) ? 0x80u : 0x20u;
        w32(P + 0x10a0, mask); w32(P + 0x10b0, u(ri(0, 25)));
        const uint32_t back = u(ri(0, 12));
        for (uint32_t i = 0; i < 30; ++i) w32(PAD + 0x1e0 + 4 * i, 0);
        w32(PAD + 0x1e0 + 4 * ((ee_read<uint16_t>(PAD + 0x18e) + 29u - back) % 30u), ri(0, 3) ? mask : 0u);
        w32(PAD + 0x190, 30);  // (the history's length: as batch 48's)
        w32(PAD + 0x1a0, ri(0, 1) ? mask : 0u); w32(PAD + 0x1a4, ri(0, 2) ? 0u : mask);
        seed_rand();
        return Args{};
    }, [](A) { return ri32(hero_item_use()); });
    g_ignored_words = saved_ignored;
    g_ee_null_is_ram = null_ram;
    g_extra_noop_hooks.clear();
}

}  // namespace rac1::oracle
