// Oracle batch 420 (issue #42, package P5: quitting to the title): the boot module read back from the disc 0x243d38
// (front_read_back, level_exit.cpp: the table of contents' +0x4f8 / +0x4fc, its place below 0x1ff8000, the overlay header
// 0x15ee4c and its first word, the read started through 0x265f40), on its own and inside the level exit 0x292e08's quit path
// (next level -1, the pause menu's Quit: the card's wait over, the fade, the read, its sceCdSync, the DMA handlers removed), on
// the level's real data (needs the ISO; no dumps). sceCdRead / its sync / its error and break are the host's on both sides
// (libcdvd.h): the sectors land in EE RAM on both. The pieces the read-back module's main runs (frontbin:0x1e9658, ported as
// boot_return_start) are the boot module's ports, proved by batches 89 / 101 / 130.
// Run: build/tests/ovl_oracle --batch 420, and with ORACLE_LEVEL=N for other levels' copies (the same code).

#include "ovl_oracle.h"

#include <game/lib/libcdvd.h>
#include <game/lib/libmc.h>
#include <game/lib/movie_player.h>
#include <game/ovl/level_change.h>
#include <game/ovl/ovl_reloc.h>
#include <runtime/ipu/movie_hw.h>
#include <runtime/kernel/disc.h>

#include <cstring>
#include <vector>

namespace rac1::oracle {

void batch_420() {
    if (g_only_batch != 420) return;
    g_batch = 420;
    g_ignored_words = vcross_lanes();
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    g_host_clock_fixed = 1759658400;
    movie_hw_enable(false);
    g_extra_hooks = {{0x00121780, [](EeCpu& c) { c.set32(2, cd_read_clock(c.r32(4))); }},
                     {0x0012d6d8, [](EeCpu& c) { clock_to_local(c.r32(4)); }},
                     {0x00121450, [](EeCpu& c) { c.set32(2, cd_read(c.r32(4), c.r32(5), c.r32(6), c.r32(7))); }},
                     {0x00120c30, [](EeCpu& c) { c.set32(2, cd_sync(static_cast<int32_t>(c.r32(4)))); }},
                     {0x00121630, [](EeCpu& c) { c.set32(2, cd_get_error()); }},
                     {0x001216c8, [](EeCpu& c) { c.set32(2, cd_break()); }},
                     {0x00123d68, [](EeCpu& c) { c.set32(2, mc_get_info(static_cast<int32_t>(c.r32(4)), static_cast<int32_t>(c.r32(5)), c.r32(6), c.r32(7), c.r32(8))); }},
                     {0x00123c30, [](EeCpu& c) { c.set32(2, mc_sync(static_cast<int32_t>(c.r32(4)), c.r32(5), c.r32(6))); }}};
    // the callees left TODO_PORT on the level exit's path: the kernel's vsync wait, the DMA kick and handlers, the GS display
    // registers, the vsync callback (as batch 84)
    g_extra_noop_hooks = {0x00122298, OVF(0x002a08c8), 0x001187d0, 0x001190f8, OVF(0x001f8540), 0x00122e68};
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
    constexpr uint32_t TOC = 0x00137b80;

    // the read on its own: the table of contents' row as the disc has it, or another file's (a smaller count: the place
    // below 0x1ff8000 moves), the header pointer and the words where it lands as they were left
    check("front_read_back", 0x00243d38, 24, Ret::Int32, [level] {
        level();
        w8(0x0015ed58, u(ri(0, 1)));
        wrand(0x0015eebc, 2);
        w32(0x0015ee4c, R());
        if (ri(0, 2) != 0) {
            const uint32_t row = TOC + 8 * u(ri(1, 0x200));  // (another file's row: its own sector, a count of 1..0x1bc)
            w32(TOC + 0x4f8, ee_read<uint32_t>(row) != 0 ? ee_read<uint32_t>(row) : 1500u);
            w32(TOC + 0x4fc, u(ri(1, 0x1bc)));
        }
        return Args{};
    }, [](A) { return ri32(front_read_back()); });

    // the level exit's quit path, to its end (the boot module read into memory, the card idle)
    check("level_exit_quit", 0x00292e08, 6, Ret::None, [level] {
        level();
        w32(0x0015ed84, u(ri(0, 18)));
        w32(OV(0x0015f5c0), 0xffffffffu);
        w32(0x0015ed88, u(ri(0, 5)));
        for (uint32_t i = 0; i < 0x20; ++i) w8(0x0013dd40 + i, u(ri(0, 2)));
        w32(0x0013d290 + 0xd4, 2);
        w32(0x0013d290 + 0xdc, 0xffffffffu);
        w32(OV(0x0015f5d8), 1);
        mc_reset();
        return Args{};
    }, [](A) {
        mc_reset();
        if (level_exit_start() != LevelExit::Quit)
            for (int f = 0; f < 40000 && !level_exit_frame(); ++f) {
            }
        return uint64_t{0};
    });

    movie_hw_enable(true);
    mc_reset();
    g_extra_noop_hooks.clear();
    g_extra_hooks.clear();
    g_host_clock_fixed = 0;
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
