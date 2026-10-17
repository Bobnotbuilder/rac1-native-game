// Oracle batch 411 (issue #42, package P3): the pause menu's widget functions the port's dispatch lacked, on the level's real
// data with the menu open on their pages (as batch 76), and the levels' own copies of two shared widget functions (the same
// instructions as Veldin's, other data) that the Veldin ports stand in for. Run with ORACLE_LEVEL=<n> build/tests/ovl_oracle
// --batch 411 (needs the ISO; no dumps), on any level (0 .. 18):
//   - the Goodies page's (state 9, shown once the game is finished) sub-pages: cheats_list_focus_in 0x277e28 (page 0x26),
//     level_movies_focus_in 0x27f970 (0x28), sketchbook_update 0x27f9b0 (0x29), epilogue_update 0x27fa98 and
//     epilogue_arrows_draw 0x27fba0 (0x2a), skill_points_text_draw 0x27fd38 (0x24): pause_menu_special.cpp;
//   - items_help_focus_in 0x27a6e0 and menu_label_update 0x2779c8 at the level's other copies (kCopies: the level's own
//     addresses; its pages hand them to menu_widget_call through the code pointer pairs), the Veldin ports as the port side.
#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/pause_menu.h>
#include <game/ovl/pause_menu_pages.h>
#include <game/ovl/planet_map.h>
#include <runtime/kernel/disc.h>

#include <map>

namespace rac1::oracle {

namespace {
constexpr uint32_t PAD = 0x0013c940;
constexpr OvConst<0x001b9cf0u> MS{};
// the Goodies page's sub-pages (Veldin's overlay data, menu.md §13.2)
constexpr OvConst<0x001b6d40u> P_24{};
constexpr OvConst<0x001b7b30u> P_CHEATS{};
constexpr OvConst<0x001b80e0u> P_28{};
constexpr OvConst<0x001b7658u> P_29{};
constexpr OvConst<0x001b74b8u> P_2A{};

// The levels' own copies of items_help_focus_in (0x27a6e0) and menu_label_update (0x2779c8) that the overlay maps do not
// pair with Veldin's (the inventory's stand-ins, docs/port-inventory.md P3): level, Veldin's function, the level's address
struct Copy {
    int level;
    uint32_t veldin, at;
};
constexpr Copy kCopies[] = {
    {1, 0x0027a6e0, 0x002904a0},
    {2, 0x0027a6e0, 0x0027bbf8},
    {3, 0x0027a6e0, 0x00269350},
    {4, 0x002779c8, 0x0026acf8},
    {4, 0x0027a6e0, 0x0026da10},
    {5, 0x002779c8, 0x002a1cd0},
    {5, 0x0027a6e0, 0x002a49e8},
    {6, 0x0027a6e0, 0x002894c0},
    {7, 0x0027a6e0, 0x002a3100},
    {8, 0x0027a6e0, 0x00284e30},
    {9, 0x0027a6e0, 0x00299500},
    {10, 0x0027a6e0, 0x0026d600},
    {11, 0x0027a6e0, 0x0029fa68},
    {12, 0x0027a6e0, 0x00293388},
    {13, 0x0027a6e0, 0x00286860},
    {14, 0x0027a6e0, 0x00282e70},
    {15, 0x0027a6e0, 0x0026a388},
    {16, 0x0027a6e0, 0x002759d8},
    {17, 0x0027a6e0, 0x00274790},
    {18, 0x002779c8, 0x00279950},
    {18, 0x0027a6e0, 0x0027c668},
};

// a check of the original at the level's own address `at` against the Veldin port
void at_check(const char* name, uint32_t at, int n, Ret ret, const std::function<Args()>& gen, const std::function<uint64_t(const Args&)>& port) {
    if (g_only_batch != 0 && g_only_batch != g_batch) return;
    if (const char* only = std::getenv("ORACLE_ONLY"); only != nullptr && std::strcmp(only, name) != 0) return;
    std::printf("%-26s L%d:0x%08x\n", name, g_oracle_level, at);
    if (!check_fn(name, at, n, ret, gen, port, Scope::FullRam)) ++g_fail;
}
}  // namespace

void batch_411() {
    if (g_only_batch != 411) return;
    g_batch = 411;
    const std::vector<uint32_t> saved_ignored = g_ignored_words;
    g_ignored_words = vcross_lanes();
    // the callees left TODO_PORT / host-side (as batch 76's)
    g_extra_noop_hooks = {0x00122298, OVF(0x002a0f18), OVF(0x002a08c8), OVF(0x0028ed58),
                          OVF(0x00299a68), OVF(0x0024fe50), OVF(0x0024ff88), OVF(0x0024fad0),
                          OVF(0x00265aa8)};
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
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
    static std::vector<uint8_t> snap, snap_menu;
    static std::map<uint32_t, std::vector<uint8_t>> snap_page;
    auto packet = [] {  // the frame packet: buffer 0, the cursor at a random point of it
        w32(OV(0x001611b8), ee_read<uint32_t>(OV(0x00173e08)));
        w32(OV(0x001611bc), ee_read<uint32_t>(OV(0x00173e08)) + 0x30000);
        w32(OV(0x001611d0), 0);
        w32(OV(0x001611c0), ee_read<uint32_t>(OV(0x001611b8)) + u(ri(0, 0x100)) * 0x10);
    };
    auto level = [packet] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            ee_write<uint16_t>(0x001518d0, 512);
            ee_write<uint16_t>(0x001518d2, 416);
            particle_table_init();
            level_setup_body(1, 0);
            hud_init();
            w32(OV(0x0016c060), 0xf);
            if (g_oracle_level == 18) w32(0x0013f2e8, 0xffffffffu);  // (the sound listener's cached points, as batch 270)
            if (g_oracle_level == 14) w8(0x0013d3f0, 1);  // (the arrival scene seen: its stream read from the disc, as batch 221)
            gameplay_frame();
            w32(0x0015ec8c, 1);  // the stream system up (the menu's pictures are read through it)
            if (g_oracle_level == 18) {  // the disc idle and the streams' first step, as batch 270
                w32(0x00137b00, 0);
                sub_00264bf8();
            }
            snap.assign(ram, ram + 0x02000000);
            for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); ee_read<uint8_t>(m + 0x20) != 0xff; m += 0x100) {
                const uint32_t fn = ee_read<uint32_t>(m + 0x74);
                if (fn != 0 && !ovl_moby_fn_ported(fn)) g_extra_noop_hooks.push_back(fn);
            }
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
        packet();
    };
    auto frames = [](int n) {
        for (int f = 0; f < n; ++f) {
            w32(PAD + 0x1c4, 0);
            w32(PAD + 0x1b4, 0);
            menu_update();
        }
    };
    // the save data the pages read: items, gold bolts, skill points, the cheats unlocked (0x15edc0) and switched on (0x15edb0)
    auto progress = [] {
        for (uint32_t i = 0; i < 0x80; ++i) w8(0x0013d388 + i, ri(0, 2) == 0 ? 1u : 0u);
        for (uint32_t i = 0; i < 0x50; ++i) w8(0x0013d4c0 + i, ri(0, 2) == 0 ? 1u : 0u);
        for (uint32_t i = 0; i < 0x20; ++i) w8(0x0013d408 + i, ri(0, 1));
        for (uint32_t i = 0; i < 0x25; ++i) w8(0x0013e520 + i, ri(0, 3) == 0 ? 1u : 0u);
        for (uint32_t i = 0; i < 0x10; ++i) w8(0x0015edb0 + i, ri(0, 1));
        for (uint32_t i = 0; i < 0x10; ++i) w8(0x0015edc0 + i, ri(0, 1));
    };
    // the menu open (the port's frames from the pause page; the game finished once, so that the Goodies entry is there)
    auto menu = [level, frames, packet] {
        uint8_t* ram = ee_get_ram_base();
        if (snap_menu.empty()) {
            level();
            w32(0x0015ee20, 1);
            w32(OV(0x0016c06c), 0);
            menu_open(0);
            frames(16);
            snap_menu.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap_menu.data(), snap_menu.size());
        }
        packet();
    };
    // the menu on page p (its focus-in functions run by the port's frames)
    auto page = [menu, packet, frames](uint32_t p) {
        uint8_t* ram = ee_get_ram_base();
        auto& s = snap_page[p];
        if (s.empty()) {
            menu();
            w32(MS + 8, p);
            frames(16);
            s.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, s.data(), s.size());
        }
        packet();
    };
    auto buttons = [] {
        static const uint32_t b[] = {0x1000, 0x4000, 0x40, 0x10, 0x800, 0x100, 0x400, 4, 8, 0x20, 0x2000, 0x8000, 0};
        const uint32_t k = b[ri(0, 12)] | (ri(0, 4) == 0 ? b[ri(0, 12)] : 0u);
        w32(PAD + 0x1c4, coin() ? k : b[ri(0, 12)]);
        w32(PAD + 0x1a4, k);
        w32(PAD + 0x1b4, coin() ? k : b[ri(0, 12)]);
    };
    auto slot = [](uint32_t k) { return ee_read<uint32_t>(ee_read<uint32_t>(MS + 4) + 0x44 + k * 4); };
    auto box = [](uint32_t w) {  // its box (menu_draw's, from its moby)
        w32(w + 0x20, u(ri(120, 300)));
        w32(w + 0x24, u(ri(-3, 200)));
    };

    // --- the cheats page (0x26): its switch list's focus in -------------------------------------------------------------
    check("cheats_list_focus_in", 0x00277e28, 120, Ret::Int32, [page, progress, slot] {
        page(P_CHEATS);
        progress();
        for (uint32_t k = 0; k < 4; ++k) w8(0x0015edc0 + 0x501a + k, 0);  // (rows 8..11's bytes: zero in every level's data)
        return Args{{slot(1)}, {}};
    }, [](A a) { return ri32(cheats_list_focus_in(a.i[0])); });
    // --- page 0x28: its planet list's focus in --------------------------------------------------------------------------
    check("level_movies_focus_in", 0x0027f970, 60, Ret::Int32, [page, slot] {
        page(P_28);
        w32(0x0015ed84, u(ri(0, 40)));
        return Args{{slot(1)}, {}};
    }, [](A a) { return ri32(level_movies_focus_in(a.i[0])); });
    // --- page 0x29: its selector (30 values) ----------------------------------------------------------------------------
    check("sketchbook_update", 0x0027f9b0, 200, Ret::Int32, [page, buttons, slot] {
        page(P_29);
        buttons();
        const uint32_t w = slot(3);
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        if (ri(0, 3) == 0) w32(ee_read<uint32_t>(MS + 4) + 0x38, 0);
        w32(w + 0x40, u(ri(0, 5) == 0 ? ri(-40, 40) : ri(0, 29)));
        if (coin()) w32(w + 0x14, 0);
        return Args{{w}, {}};
    }, [](A a) { return ri32(sketchbook_update(a.i[0])); });
    // --- page 0x2a: its selector (12 values) and its arrows -------------------------------------------------------------
    check("epilogue_update", 0x0027fa98, 200, Ret::Int32, [page, buttons, slot] {
        page(P_2A);
        buttons();
        const uint32_t w = slot(2);
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        if (ri(0, 3) == 0) w32(ee_read<uint32_t>(MS + 4) + 0x38, 0);
        w32(w + 0x54, u(ri(0, 5) == 0 ? ri(-30, 30) : ri(0, 11)));
        if (coin()) w32(w + 0x14, 0);
        return Args{{w}, {}};
    }, [](A a) { return ri32(epilogue_update(a.i[0])); });
    check("epilogue_arrows_draw", 0x0027fba0, 100, Ret::Int32, [page, slot, box] {
        page(P_2A);
        const uint32_t w = slot(coin() ? 0u : 2u);
        box(w);
        if (ri(0, 2) == 0) w32(w + 0x38, coin() ? 0u : slot(1));
        return Args{{w}, {}};
    }, [](A a) { return ri32(epilogue_arrows_draw(a.i[0])); });
    // --- page 0x24: its text panel (the selected entry of the focused list) ---------------------------------------------
    check("skill_points_text_draw", 0x0027fd38, 150, Ret::Int32, [page, slot, box] {
        page(P_24);
        const uint32_t list = slot(1);
        w32(ee_read<uint32_t>(MS + 4) + 0x40, list);
        int32_t n = 0;
        for (uint32_t e = ee_read<uint32_t>(list + 0x34); ee_read<int16_t>(e) != 0; e += 0xc) ++n;
        const int32_t sel = ri(0, n - 1);
        w32(list + 0x40, u(sel));
        if (ri(0, 3) == 0) w32(ee_read<uint32_t>(list + 0x34) + u(sel) * 0xc + 4, 0xffffffffu);
        const uint32_t w = slot(5);
        box(w);
        return Args{{w}, {}};
    }, [](A a) { return ri32(skill_points_text_draw(a.i[0])); });

    // --- the levels' own copies of two shared widget functions, the Veldin ports standing in -----------------------------
    for (const Copy& c : kCopies) {
        if (c.level != g_oracle_level) continue;
        if (c.veldin == 0x0027a6e0) {
            at_check("items_help_focus_in", c.at, 20, Ret::Int32, [level] {
                level();
                w8(0x0013d4c2, ri(0, 1));
                wrand(TEST_AREA, 0x20);
                return Args{{TEST_AREA}, {}};
            }, [](A a) { return ri32(items_help_focus_in(a.i[0])); });
        } else if (c.veldin == 0x002779c8) {
            at_check("menu_label_update", c.at, 120, Ret::Int32, [menu, buttons] {
                menu();
                buttons();
                w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
                const uint32_t p = ee_read<uint32_t>(MS + 4);
                const uint32_t w = ee_read<uint32_t>(p + 0x44 + u(ri(0, 1)) * 4);
                if (coin()) w32(p + 0x40, w);
                if (ri(0, 3) == 0) w32(p + 0x38, coin() ? 0u : OV(0x001b2588u));
                return Args{{w}, {}};
            }, [](A a) { return ri32(menu_label_update(a.i[0])); });
        }
    }

    //@@NEXT
    g_ignored_words = saved_ignored;
    g_ee_null_is_ram = null_ram;
    g_extra_noop_hooks.clear();
}

}  // namespace rac1::oracle
