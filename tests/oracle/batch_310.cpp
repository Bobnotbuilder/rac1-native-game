// Oracle batch 310 (gameplay bugs, issue #18 "returning to Umbris after finishing it"): the gameplay loader 0x241940
// (gameplay_load, level_load.cpp) whole, on a level's real data, with the state a return to a planet reads: the planets'
// visited / done bytes 0x13dd40 / 0x13dd58 (2: the level finished), the level's row of the kill bits 0x14c190 (a moby broken
// or collected once, by its index +0xb2), the checkpoints 0x14c050 and their copy for this visit 0x15fc88, the visit's own
// kill bits 0x1ba4d0, the slot bytes 0x1bb684 and the respawning groups' counts 0x14d590. The loader's moby pass decides from
// them which placed mobys spawn and with how many bolts (the spawn rules: flags 1 / 2 / 4 / 8 / 0x10 of each placement): the
// part of a level's start that differs on a return. The port's loader is a hand port that no batch ran whole before (batch 58
// proves its leaves); this one compares the whole of EE RAM after it with the original's.
//
// Run with ORACLE_LEVEL=<n> build/tests/ovl_oracle --batch 310 on any level (needs the ISO; no RAM dump): the cases are on the
// level's own placements, after its setup (as batch 119 builds it); each case reads the level's gameplay data again from the disc
// (the first load decompressed it over its compressed copy), as the loader's own restart does. B310_PLAIN=1 leaves out the return's
// state (the level's start as it is). Run on every level, 0 .. 18: all pass. A mutation of the port's spawn rules (the flag 4 kill
// bit ignored, a broken moby's halved bolts not halved) fails on levels 3, 7 and 12.

#include "ovl_oracle.h"

#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>

#include <cstdlib>
#include <cstring>
#include <vector>

namespace rac1::oracle {

void batch_310() {
    if (g_only_batch != 310) return;
    g_batch = 310;
    g_ignored_words = vcross_lanes();
    // the scratchpad words the pose function (0x245f18) spills its caller's $t8 / $t9 into (the transliterated callers have no
    // such registers), as batch 119
    for (uint32_t a = 0x70003700; a < 0x70003720; a += 4) g_ignored_words.push_back(a);
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // as in the runtime
    // the level set up as a level start does (the loader already run once by level_setup_body, as the game's own start)
    // (the scratchpad too: the loader's callees use it as their work area, and a case must not start from what the previous
    // case's port left there)
    static std::vector<uint8_t> snap, spr;
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        uint8_t* sp = static_cast<uint8_t*>(ee_ram_ptr(0x70000000u));
        if (snap.empty()) {
            veldin_boot();
            level_setup_body(1, 0);
            snap.assign(ram, ram + 0x02000000);
            spr.assign(sp, sp + 0x4000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
            std::memcpy(sp, spr.data(), spr.size());
        }
    };
    level();
    const uint32_t lv = ee_read<uint32_t>(0x0015ed84);
    // the placements' count (the loader's moby block): the kill bits and slot bytes are varied over the level's uids
    const uint32_t H = ee_read<uint32_t>(OV(0x00173e04));
    const uint32_t mp = H + ee_read<uint32_t>(H + 0x44);
    const int32_t placed = static_cast<int32_t>(ee_read<uint32_t>(mp));
    std::printf("batch 310: level %u, %d placed mobys\n", lv, placed);
    // the return's state: a level finished (or not), some of its mobys broken before (all, none or a random part of them), its
    // checkpoints passed, the respawning groups' counts
    auto revisit = [lv, placed] {
        const int mode = ri(0, 3);  // 0 a first visit, 1 visited, 2 finished (the report's case), 3 random bytes
        for (uint32_t k = 0; k < 0x13; ++k) {
            const uint32_t v = mode == 3 ? u(ri(0, 2)) : (k == lv ? u(mode) : u(ri(0, 2)));
            w8(0x0013dd58 + k, v);
            w8(0x0013dd40 + k, u(ri(0, 1)));
        }
        const uint32_t words = static_cast<uint32_t>((placed + 31) / 32);
        const int kill = ri(0, 3);  // 0 none, 1 all, 2 / 3 a random part
        for (uint32_t w = 0; w < words && w < 0x40; ++w) {
            const uint32_t bits = kill == 0 ? 0u : (kill == 1 ? 0xffffffffu : R() & R());
            w32(0x0014c190 + lv * 0x100 + w * 4, bits);
            w32(OV(0x001ba4d0) + w * 4, coin() ? 0u : (R() & R()));
        }
        for (int32_t i = 0; i < placed && i < 0x800; ++i) w8(OV(0x001bb684) + u(i), ri(0, 15) == 0 ? u(ri(1, 17)) : 0u);
        for (uint32_t c = 0; c < 0x10; ++c) {
            const uint32_t v = ri(0, 1) ? 0xffu : u(ri(0, 3));
            w8(0x0014c050 + lv * 0x10 + c, v);
            w8(OV(0x0015fc88) + c, coin() ? v : (ri(0, 1) ? 0xffu : 0u));
        }
        for (uint32_t g = 0; g < 0x40; ++g) w16(0x0014d590 + lv * 0x100 + g * 4 + 2, ri(0, 2) == 0 ? u(ri(0, 40)) : 0u);
    };
    check("gameplay_load/revisit", 0x00241940, 24, Ret::Int32, [level, revisit] {
        level();
        if (std::getenv("B310_PLAIN") == nullptr) revisit();  // (B310_PLAIN: the level's own start state, for comparison)
        // the level's gameplay data read again from the disc to the start of the level's area, as the loader's restart (a0 0)
        // does it: the first load decompressed it over its compressed copy. Both sides then decompress the same data (a0 1)
        const uint32_t s8 = ee_read<uint32_t>(OV(0x00173e18));
        if (ee_read<int32_t>(0x0015ed80) != 0) disc_read_wait(s8, ee_read<uint32_t>(0x0013a4f8), ee_read<uint32_t>(0x0013a4fc));
        else disc_read_wait(s8, ee_read<uint32_t>(0x0013a4f0), ee_read<uint32_t>(0x0013a4f4));
        w32(0x0015ee50, s8);
        return Args{{1}, {}};
    }, [](A a) { return r32(gameplay_load(static_cast<int32_t>(a.i[0]))); });
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
