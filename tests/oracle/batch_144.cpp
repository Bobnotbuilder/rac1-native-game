// Oracle batch 144 (task 0408): the menus' item models, drawn by the one-moby draw 0x24ff08 (moby_draw_list, moby_pass.cpp) from
// the panels' draws, on RAM dumps of a headless run with the pause menu open (assets/dumps/oracle/menu_0408/, skipped without them):
//   weapons690.bin  the Weapons page (RAC1_GOTO=0@60 RAC1_GIVE_ITEMS=0xf,0x10,0x11 RAC1_MENU_AT=300 RAC1_SIM_INPUT=menu:x, frame 690),
//   gadgets690.bin  the Gadgets page (the same with RAC1_GIVE_ITEMS=0xf,0x10,0x11,0x1a,0x16 and menu:dx);
// on another level (ORACLE_LEVEL=n) weapons_L<n>.bin / gadgets_L<n>.bin of that level's pause menu (Novalis: RAC1_LEVEL=1
//   RAC1_GIVE_FRAME=1500 RAC1_MENU_AT=1900, the same items and keys, frame 2290).
// Checked: 0x24ff08 on the pages' mobys, the hero panel 0x27ba40, the item panel 0x27c250, the ammo panel 0x27bd58 and the menu's
// whole draw 0x2772c0 (its widgets drawn into textures, the item models with them) on the dumped page.
#include "ovl_oracle.h"

#include <game/ovl/moby_pass.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/pause_menu.h>
#include <game/ovl/pause_menu_pages.h>
#include <game/ovl/vu0_macro.h>
#include <runtime/ee/ee_interpreter.h>
#include <runtime/vu/vu0.h>

#include <cstdio>
#include <cstring>
#include <map>
#include <string>

namespace rac1::oracle {

namespace {
// a RAM dump of a headless run (32 MB + the scratchpad), empty when the file is missing (as batch 142's)
const std::vector<uint8_t>& menu_dump(const std::string& path) {
    static std::map<std::string, std::vector<uint8_t>> cache;
    std::vector<uint8_t>& d = cache[path];
    if (d.empty()) {
        if (FILE* f = std::fopen(path.c_str(), "rb")) {
            d.resize(0x02000000 + 0x4000);
            if (std::fread(d.data(), 1, d.size(), f) != d.size()) d.clear();
            std::fclose(f);
        }
    }
    return d;
}
}  // namespace

void batch_144() {
    if (g_only_batch != 144) return;
    g_batch = 144;
    const std::vector<uint32_t> saved_ignored = g_ignored_words;
    g_ignored_words = vcross_lanes();
    // the callees left TODO_PORT (as batch 73's menu_draw check): the vsync wait, the frame capture, the packet kick, the mode-3 screens
    g_extra_noop_hooks = {0x00122298, OVF(0x002a0f18), OVF(0x002a08c8), OVF(0x00299a68)};
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    const std::string sfx = g_oracle_level == 0 ? std::string("690.bin") : "_L" + std::to_string(g_oracle_level) + ".bin";
    const std::vector<uint8_t>* dumps[2] = {&menu_dump(dump_path("menu_0408/weapons") + sfx), &menu_dump(dump_path("menu_0408/gadgets") + sfx)};
    if (dumps[0]->empty() || dumps[1]->empty()) {
        for (const char* n : {"moby_draw_list", "menu_hero_draw", "menu_model_draw", "menu_ammo_draw", "menu_draw"})
            skip_check(n, 0x0024ff08, "no pause menu RAM dumps (assets/dumps/oracle/menu_0408)");
        g_ignored_words = saved_ignored;
        g_ee_null_is_ram = null_ram;
        g_extra_noop_hooks.clear();
        return;
    }
    // one of the dumps (0 Weapons, 1 Gadgets, -1 either), the frame packet's cursor moved on a little (a draw later in the frame)
    auto load = [&dumps](int which) {
        const std::vector<uint8_t>& d = *dumps[which < 0 ? ri(0, 1) : which];
        std::memcpy(ee_get_ram_base(), d.data(), 0x02000000);
        std::memcpy(ee_ram_ptr(0x70000000u), d.data() + 0x02000000, 0x4000);
        w32(OV(0x001611c0), ee_read<uint32_t>(OV(0x001611c0)) + u(ri(0, 0x40)) * 0x10);
    };
    // the shown page's widget of slot `slot` (page + 0x44 + 4 slot), 0 when none
    auto widget = [](uint32_t slot) {
        const uint32_t page = ee_read<uint32_t>(OV(0x001b9cf0) + 4);
        return page != 0 ? ee_read<uint32_t>(page + 0x44 + slot * 4) : 0u;
    };
    // the page's widget whose draw function is `fn` (relocated), 0 when none
    auto widget_of = [widget](uint32_t fn) {
        for (uint32_t s = 0; s < 0xe; ++s) {
            const uint32_t w = widget(s);
            if (w != 0 && ee_read<uint32_t>(w + 4) == fn) return w;
        }
        return 0u;
    };
    // a moby the page shows: a panel's model (+0x44 / +0x48, the hero panel's 24) or one of the menu's 14 frames
    auto menu_moby = [widget]() {
        std::vector<uint32_t> v;
        for (uint32_t s = 0; s < 0xe; ++s) {
            const uint32_t w = widget(s);
            if (w == 0) continue;
            const uint32_t n = ee_read<uint32_t>(w + 4) == OVF(0x0027ba40) ? 0x18u : 2u;
            for (uint32_t i = 0; i < n; ++i)
                if (const uint32_t m = ee_read<uint32_t>(w + 0x44 + i * 4); m != 0) v.push_back(m);
        }
        for (uint32_t i = 0; i < 0xe; ++i)
            if (const uint32_t m = ee_read<uint32_t>(OV(0x001b9e90) + i * 4); m != 0) v.push_back(m);
        return v.empty() ? 0u : v[static_cast<size_t>(ri(0, static_cast<int>(v.size()) - 1))];
    };
    // a model turned / moved a little (its place in front of the menu camera)
    auto nudge = [](uint32_t m) {
        if (m == 0) return;
        wf(m + 0x48, ee_read<float>(m + 0x48) + rf(-0.5f, 0.5f));
        for (uint32_t k = 0; k < 3; ++k) wf(m + 0x10 + 4 * k, ee_read<float>(m + 0x10 + 4 * k) + rf(-0.2f, 0.2f));
    };
    check("moby_draw_list", 0x0024ff08, 40, Ret::None, [load, menu_moby, nudge] {
        load(-1);
        const uint32_t m = menu_moby();
        if (coin()) nudge(m);
        return Args{{m, 1}, {}};
    }, [](A a) { if (a.i[0] != 0) moby_draw_list(a.i[0], static_cast<int32_t>(a.i[1])); return uint64_t{0}; });
    check("menu_hero_draw", 0x0027ba40, 20, Ret::Int32, [load, widget_of] {
        load(-1);
        const uint32_t w = widget_of(OVF(0x0027ba40));
        if (coin()) w8(w + 0xa4 + u(ri(0, 0x17)), 1);
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_hero_draw(a.i[0])); });
    check("menu_model_draw", 0x0027c250, 20, Ret::Int32, [load, widget_of, nudge] {
        load(-1);
        const uint32_t w = widget_of(OVF(0x0027c250));
        nudge(ee_read<uint32_t>(w + 0x44));
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_model_draw(a.i[0])); });
    check("menu_ammo_draw", 0x0027bd58, 20, Ret::Int32, [load, widget_of, nudge] {
        load(0);
        const uint32_t w = widget_of(OVF(0x0027bd58));
        nudge(ee_read<uint32_t>(w + 0x44));
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_ammo_draw(a.i[0])); });
    // the menu's whole draw: the panels' frames, the widgets drawn into textures (the models with them), the panels. Its moby pass
    // skins through the VU0 programs, whose results the original's loop polls: run with VU0 co-execution, the port from the same VU0
    // state, the skinning's saved return address (0x70001ff0) a stale lane (as batch 79's moby_skin_all)
    {
        const std::vector<uint32_t> ign = g_ignored_words;
        for (uint32_t a = 0x70001ff0; a < 0x70002000; a += 4) g_ignored_words.push_back(a);
        for (uint32_t a = OV(0x0019b780); a < OV(0x0019b7e0); a += 4) g_ignored_words.push_back(a);  // 0x2558e0's saved registers (batch 79's)
        // the 4th (w) lanes of the skinning records' rows (the ring 0x15f5f8 on) and of their copy on the scratchpad (0x1c00..): stale
        // VU0 lanes of the original (the menu's code before the moby pass leaves other values in the VU0 registers' w lanes than the
        // port's); the only words that differ: every joint matrix, skinned vertex and packet word agrees
        load(0);
        const uint32_t ring = ee_read<uint32_t>(OV(0x0015f5f8));
        for (uint32_t a = ring + 0xc; a < ring + 0x20000; a += 0x10) g_ignored_words.push_back(a);
        for (uint32_t a = 0x70001c0c; a < 0x70002c00; a += 0x10) g_ignored_words.push_back(a);
        g_pre_call = [](EeInterpreter& ee) {  // the whole VU0 the port starts from (registers, data memory), as batch 79's moby_skin_all
            const VUState& p = vu0::global().st;
            ee.vu0().vf = p.vf;
            ee.vu0().vi = p.vi;
            ee.vu0().acc = p.acc;
            ee.vu0().q = p.q;
            ee.vu0().i = p.i;
            ee.vu0().mem = p.mem;
            ee.set_vu0_coexec(true, 1, 1);
        };
        check("menu_draw", 0x002772c0, 10, Ret::None, [load] {
            load(-1);
            // (the moby groups drawn 0x19a600, -1 ended, as batch 73 / 131)
            if (ee_read<uint32_t>(OV(0x0019a600)) == 0) w32(OV(0x0019a600), 0xffffffffu);
            seed_rand();
            return Args{{0}, {}};
        }, [](A a) {
            menu_draw(static_cast<int32_t>(a.i[0]));
            return uint64_t{0};
        });
        g_pre_call = nullptr;
        g_ignored_words = ign;
    }
    g_ignored_words = saved_ignored;
    g_ee_null_is_ram = null_ram;
    g_extra_noop_hooks.clear();
}

}  // namespace rac1::oracle
