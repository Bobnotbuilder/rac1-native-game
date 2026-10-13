// Oracle batch 153 (task 0419): the text lookup on a level's own text table. Run with ORACLE_LEVEL=n build/tests/ovl_oracle
// --batch 153 (levels 0..7 checked; the level's own original against the relocated port; needs the ISO).
// The level as its start lays it out (veldin_boot + level_setup_body, batch 59 checks that body against the original), so the text
// table *(0x15f660) (count 0x17943c) is the level's own block for the language 0x15ed88. Checked:
//  - the text lookup 0x203ad8 for the ids the items use: the quick select's names (the item rows 0x179ac0 + 0x4c i, +0x46 / +0x48),
//    the acquisition messages item_give shows (0x1b0340, the first and the "again" ones), and random ids;
//  - item_give 0x260860 for every item, owned or not (its message copied to 0x179118 with the timer 0x15f640).
// It also reports which item messages the original resolves to its placeholder record for a missing message (0x1796a8): a
// first acquisition message is only in the table of the level the game gives that item on.
#include "ovl_oracle.h"

#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace rac1::oracle {

void batch_153() {
    if (g_only_batch != 153) return;
    g_batch = 153;
    if (g_oracle_level < 0 || g_oracle_level > 7) {
        skip_check("text_lookup_items", 0x00203ad8, "levels 0..7");
        return;
    }
    g_ignored_words = vcross_lanes();
    static std::vector<uint8_t> snap;
    static int snap_level = -1;
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty() || snap_level != g_oracle_level) {
            veldin_boot();
            level_setup_body(1, 0);
            snap.assign(ram, ram + 0x02000000);
            snap_level = g_oracle_level;
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    level();
    // the ids the items use
    std::vector<uint32_t> ids;
    for (uint32_t i = 0; i < 0x4a; ++i) {
        const int32_t m = ee_read<int32_t>(OV(0x001b0340) + i * 4);
        if (m == -1) continue;
        ids.push_back(static_cast<uint32_t>(m));
    }
    for (uint32_t i = 0; i < 0x25; ++i)
        for (uint32_t o : {0x46u, 0x48u}) {
            const int32_t m = ee_read<int16_t>(OV(0x00179ac0) + i * 0x4c + o);
            if (m > 0) ids.push_back(static_cast<uint32_t>(m));
        }
    std::printf("level %d: text table of %d texts, %zu item ids\n", g_oracle_level, ee_read<int32_t>(OV(0x0017943c)), ids.size());

    // the original's answers: the item messages it resolves to the Paradox record
    {
        const uint32_t fn = g_oracle_level == 0 ? 0x00203ad8u : ovl_code_lookup(0x00203ad8u);
        const uint32_t paradox = OV(0x001796a8);
        std::string have, lack;
        int names_lacking = 0;
        for (uint32_t i = 0; i < 0x4a; ++i) {
            const int32_t m = ee_read<int32_t>(OV(0x001b0340) + i * 4);
            if (m == -1) continue;
            const OrigResult r = call_original(fn, {static_cast<uint32_t>(m)});
            char b[32];
            std::snprintf(b, sizeof b, " 0x%x%s", i % 0x25, i >= 0x25 ? "'" : "");
            (static_cast<uint32_t>(r.v0) == paradox ? lack : have) += b;
        }
        for (uint32_t i = 0; i < 0x25; ++i)
            for (uint32_t o : {0x46u, 0x48u}) {
                const int32_t m = ee_read<int16_t>(OV(0x00179ac0) + i * 0x4c + o);
                if (m > 0 && static_cast<uint32_t>(call_original(fn, {static_cast<uint32_t>(m)}).v0) == paradox) ++names_lacking;
            }
        std::printf("  original 0x%08x (\"%.40s\"): item names resolved to it: %d\n", paradox, static_cast<const char*>(ee_ram_ptr(paradox)), names_lacking);
        std::printf("  item messages found (' = the \"again\" message):%s\n", have.c_str());
        std::printf("  item messages resolved to the Paradox record:%s\n", lack.c_str());
        level();
    }

    check("text_lookup_items", 0x00203ad8, 300, Ret::Int32, [&ids, level] {
        level();
        if (ri(0, 7) == 0) return Args{{u(ri(0, 0x6000))}, {}};
        return Args{{ids[static_cast<size_t>(ri(0, static_cast<int>(ids.size()) - 1))]}, {}};
    }, [](A a) { return r32(sub_00203ad8(static_cast<int32_t>(a.i[0]))); });
    check("item_give_level", 0x00260860, 150, Ret::None, [level] {
        level();
        const uint32_t id = u(ri(1, 0x24));
        ee_write<uint8_t>(0x0013d4c0 + id, coin() ? 1 : 0);
        return Args{{id, u(ri(0, 1))}, {}};
    }, [](A a) { item_give(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1])); return uint64_t{0}; });
}

}  // namespace rac1::oracle
