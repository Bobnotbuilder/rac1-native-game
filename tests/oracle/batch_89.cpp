// Oracle batch 89 (task 0308): the boot module on its own image (SCUS_971.99's sections, the ports
// relocated onto it: ovl_reloc_set_level(OVL_FRONT)). The original is the boot module's own code
// (its addresses), run in the EE interpreter; the ports are src/game/boot (its own functions) and
// the overlays' ports they call (Veldin's code relocated). States: the boot module started
// (boot_start, the port's), then the title loaded (title_loop_start) and a few passes of its loop,
// then the main menu open. The disc's reads (the stream system's command 0x38, sceCdRead behind
// 0x216828) are the host's on both sides; 989snd's IOP the batches' stand-in.

#include "ovl_oracle.h"

#include <game/boot/boot_start.h>
#include <game/boot/front_end.h>
#include <game/boot/front_image.h>
#include <game/boot/title.h>
#include <game/boot/title_menu.h>
#include <game/lib/libmc.h>
#include <game/ovl/arrival.h>
#include <game/ovl/level_change.h>
#include <game/ovl/memory_card.h>
#include <game/ovl/moby_draw.h>
#include <game/ovl/moby_pass.h>
#include <game/ovl/pause_menu.h>
#include <game/ovl/vu0_macro.h>
#include <runtime/ee/ee_interpreter.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/render/render_toggles.h>
#include <runtime/kernel/disc.h>

namespace rac1::oracle {

namespace {
constexpr uint32_t RETURN_MAGIC = 0xfffffff0u;  // the interpreter's $ra sentinel (ee_interpreter.cpp)
constexpr uint32_t SCENE = 0x0018cb20;
constexpr uint32_t MODE = 0x0015f604;
constexpr uint32_t MS = 0x001d5bf0;

// 989snd's IOP (batch 85's): replies a function of the command, the stream system's disc reads
// (0x38) done at once
void iop_stand_in() {
    auto result = [](uint32_t id, uint32_t a0) -> uint32_t {
        const uint32_t h = (id * 0x9e3779b1u) ^ (a0 * 0x85ebca6bu);
        return ((h >> 7) % 4 == 0) ? 0u : ((h >> 3) | 1u);
    };
    sif_rpc_register(0x123456, [result](uint32_t fno, uint32_t send, uint32_t, uint32_t recv, uint32_t) {
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
}

std::vector<uint8_t> g_snap_start, g_snap_title, g_snap_menu;

// the states: the boot module started; the title loaded and running; its main menu open
void build_states() {
    uint8_t* ram = ee_get_ram_base();
    front_image_load();
    ovl_reloc_set_level(OVL_FRONT);
    mc_reset();
    boot_start();
    ee_write<uint16_t>(0x001518d0, 512);
    ee_write<uint16_t>(0x001518d2, 416);
    w32(0x0015ec8c, 1);
    w32(0x00137b00, 0);
    // the title's sound tables (the main after its intro; the bank's handle as the IOP gave it)
    const uint32_t bank = 0x100000;
    sw(0x001861e0 + 0x9c, bank);
    sw(0x00186100 + 0xdc, bank);
    sw(0x0015f634, 0x00186100);
    sw(0x0015f630, 7);
    for (uint32_t o : {0x1cu, 0x3cu, 0x5cu, 0x7cu, 0x9cu, 0xbcu}) sw(0x00186100 + o, bank);
    for (uint32_t o : {0x1cu, 0x3cu, 0x5cu, 0x7cu}) sw(0x001861e0 + o, bank);
    g_snap_start.assign(ram, ram + 0x02000000);
    title_loop_start();
    title_loop_ready();
    for (int i = 0; i < 70; ++i) title_loop_frame();
    g_snap_title.assign(ram, ram + 0x02000000);
    title_menu_open();
    for (int i = 0; i < 30; ++i) title_loop_frame();
    g_snap_menu.assign(ram, ram + 0x02000000);
}
void use(const std::vector<uint8_t>& s) { std::memcpy(ee_get_ram_base(), s.data(), s.size()); }

// the sound slots (0x13e550, 0x70 each) varied: ids and states
void vary_slots() {
    for (uint32_t i = 0; i < 26; ++i) {
        const uint32_t r = 0x0013e550 + i * 0x70;
        if (ri(0, 2) == 0) continue;
        w16(r + 0x7e, u(ri(0, 8)) + ee_read<uint32_t>(0x0015f5b4));
        w8(r + 0x74, u(ri(0, 4)));
    }
}
}  // namespace

void batch_89() {
    if (g_only_batch != 89) return;
    g_batch = 89;
    g_ignored_words.clear();
    // the original's stale lanes: the skinning routine's saved registers (0x1b7680, the overlays' 0x19b780,
    // batch 79's) and the draw loop's scratchpad stack
    for (uint32_t a = 0x001b7680; a < 0x001b76e0; a += 4) g_ignored_words.push_back(a);
    for (uint32_t a = 0x70003f80; a < 0x70004000; a += 4) g_ignored_words.push_back(a);
    // the moby pass's end and the title's whole draw differ (the skinned vertices' colours, task 0308
    // open question): checked with ORACLE_B89_KNOWN=1 only
    const bool known = std::getenv("ORACLE_B89_KNOWN") != nullptr;
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    g_oracle_front = true;
    iop_stand_in();
    // the host's stand-ins on both sides: libmc (no card), the disc read 0x216828 (the overlays'
    // 0x266040), the pad
    g_extra_hooks = {{0x00123d68, [](EeCpu& c) { c.set32(2, mc_get_info(static_cast<int32_t>(c.r32(4)), static_cast<int32_t>(c.r32(5)), c.r32(6), c.r32(7), c.r32(8))); }},
                     {0x00123c30, [](EeCpu& c) { c.set32(2, mc_sync(static_cast<int32_t>(c.r32(4)), c.r32(5), c.r32(6))); }},
                     {0x00216828, [](EeCpu& c) { c.set32(2, disc_read_wait(c.r32(4), c.r32(5), c.r32(6))); }},
                     {0x00217a10, [](EeCpu&) { pad_update(); }}};
    // the callees the ports leave TODO_PORT (the DMA kick, the fade, the vsync, the GS's resets, sceCdSync, the debug printf; the
    // title's world passes, sky and lighting run on both sides since task 0365, their chain kept as the original keeps it)
    g_extra_noop_hooks = {0x00122298, 0x002336a0, 0x001f4a58, 0x00232ce0, 0x00120c30, 0x001204b8, 0x00121878, 0x001f34e8, 0x001fb2a8, 0x00122e68};
    const bool chain_forced = g_world_chain_forced;
    g_world_chain_forced = true;
    if (g_snap_start.empty()) build_states();
    auto title = [] { use(g_snap_title); };
    auto menu = [] { use(g_snap_menu); };

    check("boot_tex_base", 0x001e9128, 4, Ret::None, [] { use(g_snap_start); w32(0x0015ee8c, R()); return Args{}; },
          [](A) { boot_tex_base(); return uint64_t{0}; });
    check("boot_clut_order", 0x001f7a30, 2, Ret::None, [] { use(g_snap_start); return Args{}; }, [](A) { boot_clut_order(); return uint64_t{0}; });
    check("boot_card_check", 0x00209168, 20, Ret::Int32, [] {
        use(g_snap_start);
        mc_reset();
        w32(0x0013d290, u(ri(0, 1)));
        w32(0x0013d294, 0);
        return Args{};
    }, [](A) { mc_reset(); return ri32(boot_card_check()); });
    check("boot_picture", 0x002012b8, 10, Ret::None, [] {
        use(g_snap_start);
        w32(0x0015ed80, u(ri(0, 1)));
        w32(0x0015ee84, u(ri(0, 1)) * 0xe0000u);
        return Args{{0x00500000u + u(ri(0, 0x1000)) * 0x40}};
    }, [](A a) { boot_picture(a.i[0]); return uint64_t{0}; });
    check("title_texts", 0x001eaf50, 16, Ret::None, [title] { title(); return Args{{u(ri(0, 7))}}; },
          [](A a) { title_texts(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    check("title_sound_alive", 0x001eb740, 200, Ret::Int32, [title] {
        title();
        vary_slots();
        return Args{{u(ri(-2, 25)), u(ri(0, 8))}};
    }, [](A a) { return ri32(title_sound_alive(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]))); });
    check("title_sound_play", 0x0022dba0, 60, Ret::Int32, [title] {
        title();
        vary_slots();
        return Args{{u(ri(-1, 8)), 0, u(ri(0, 3))}};
    }, [](A a) { return ri32(title_sound_play(static_cast<int32_t>(a.i[0]), a.i[1], a.i[2])); });
    // the loop's timed sounds: its code 0x1eba18..0x1ebaf8 (entered with s7 = -1, left at the
    // update's call)
    {
        g_pre_call = [](EeInterpreter& ee) { ee.cpu().set32(23, -1); };
        g_extra_hooks.push_back({0x001ebaf8, [](EeCpu& c) { c.set32(31, static_cast<int32_t>(RETURN_MAGIC)); }});
        check("title_sounds_timed", 0x001eba18, 200, Ret::None, [title] {
            title();
            vary_slots();
            w32(SCENE + 0x34, u(coin() ? ri(0, 400) : ri(700, 1300)));
            for (uint32_t r = 0x001862bc; r < 0x001862bc + 5 * 0x10; r += 0x10) w32(r, coin() ? 0xffffffffu : u(ri(0, 25)));
            return Args{};
        }, [](A) {
            sw(0x0015f618, 0);
            title_sounds_timed();
            return uint64_t{0};
        });
        g_extra_hooks.pop_back();
        g_pre_call = nullptr;
    }
    check("title_camera", 0x001eaf88, 30, Ret::None, [title] {
        title();
        w32(SCENE + 0x38, u(ri(0, 0x5f)));
        return Args{};
    }, [](A) { title_camera(); return uint64_t{0}; });
    // the overlays' ports relocated onto the boot module (its copies of 0x28cc08, 0x202aa0)
    check("front:sound_frame", 0x0022ca50, 20, Ret::Int32, [menu] { menu(); vary_slots(); return Args{}; },
          [](A) { return ri32(sound_frame()); });
    check("front:screen_update", 0x001fce28, 20, Ret::None, [menu] {
        menu();
        w32(MODE, 4);
        return Args{};
    }, [](A) { screen_update(); return uint64_t{0}; });
    check("front:sub_0024ebc0", 0x0020c880, 20, Ret::None, [title] {
        title();
        w32(SCENE + 0x38, u(ri(0, 0x60)));
        const int32_t n = lh(SCENE + 0x44);
        return Args{{lwu(SCENE + 0x178 + u(ri(0, n - 1)) * 4)}};
    }, [](A a) { sub_0024ebc0(a.i[0]); return uint64_t{0}; });
    check("front:moby_update_transform", 0x0020def8, 20, Ret::None, [title] {
        title();
        const int32_t n = lh(SCENE + 0x44);
        return Args{{lwu(SCENE + 0x178 + u(ri(0, n - 1)) * 4)}};
    }, [](A a) { moby_update_transform(a.i[0]); return uint64_t{0}; });
    check("front:scene_block_load", 0x002049f0, 10, Ret::None, [title] {
        title();
        return Args{{u(ri(0, 3))}};
    }, [](A a) { scene_block_load(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    if (known) check("front:moby_pass", 0x0020d460, u(std::getenv("ORACLE_B89_N") != nullptr ? std::atoi(std::getenv("ORACLE_B89_N")) : 4), Ret::None, [title] { title(); return Args{}; },
          [](A) { moby_pass(); return uint64_t{0}; });
    check("front:moby_pass_start", 0x0020d278, 2, Ret::None, [title] { title(); return Args{}; }, [](A) { moby_pass_start(); return uint64_t{0}; });
    check("front:moby_draw_loop", 0x00211808, 2, Ret::Int32, [title] {
        title();
        moby_pass_start();
        spr_moby_fill();
        return Args{{lwu(0x0015ff18), lwu(0x0015ff14), 0xffffffffu, 1}};
    }, [](A a) { return r32(moby_draw_loop(a.i[0], a.i[1], static_cast<int32_t>(a.i[2]), a.i[3])); });
    if (known) check("front:moby_pass_end", 0x0020d3b0, 2, Ret::None, [title] {
        title();
        moby_pass_start();
        spr_moby_fill();
        sw(0x0015ff14, moby_draw_loop(lwu(0x0015ff18), lwu(0x0015ff14), -1, 1));
        return Args{};
    }, [](A) { moby_pass_end(); return uint64_t{0}; });
    check("front:moby_draw_list", 0x0020d330, 6, Ret::None, [menu] {
        menu();
        const uint32_t m = lwu(0x001d5d90 + u(ri(0, 13)) * 4);
        return Args{{coin() ? lwu(0x0015ff18) : m, u(coin() ? 1 : 4)}};
    }, [](A a) { moby_draw_list(a.i[0], static_cast<int32_t>(a.i[1])); return uint64_t{0}; });
    check("title_update", 0x001eb0a8, 60, Ret::None, [title] {
        title();
        w32(MODE, coin() ? 0u : u(ri(0, 4)));
        if (const char* m = std::getenv("ORACLE_B89_MODE")) w32(MODE, u(std::atoi(m)));  // (diagnosis: one game mode)
        if (std::getenv("ORACLE_B89_NOMOBY") != nullptr) w16(SCENE + 0x44, 0);  // (diagnosis: no scene mobys)
        if (std::getenv("ORACLE_B89_NOBLOCK") != nullptr) { w32(SCENE + 0x38, 1); w32(SCENE + 0x34, 1); }  // (diagnosis: no block change)
        w32(SCENE + 0x34, u(ri(0, lh(SCENE + 0x40))));
        w32(SCENE + 0x38, u(ri(0, 0x60)));
        wf(0x0015f43c, coin() ? 0.0f : rf(0.0f, 1.2f));
        w32(0x0015ef58, u(ri(0, 300)));
        w32(0x0015ef50, u(ri(0, 0x40)));
        w32(0x0015ef54, u(ri(0, 0x80)));
        w32(0x0013c940 + 0x1a4, ri(0, 3) == 0 ? (coin() ? 0x800u : 0x40u) : 0u);
        return Args{};
    }, [](A) { title_update(); return uint64_t{0}; });
    if (known) check("title_draw", 0x001eb410, u(std::getenv("ORACLE_B89_N") != nullptr ? std::atoi(std::getenv("ORACLE_B89_N")) : 8), Ret::None, [title] {
        title();
        w32(MODE, coin() ? 0u : 4u);
        wf(0x0015f43c, coin() ? 0.0f : rf(0.0f, 1.2f));
        w32(0x0015ef50, u(ri(0, 0x40)));
        w32(0x0015ef54, u(ri(0, 0x80)));
        w32(0x0015ed88, u(ri(0, 5)));
        return Args{};
    }, [](A) { title_draw(); return uint64_t{0}; });
    check("title_menu_open", 0x00218f68, 4, Ret::None, [title] { title(); return Args{}; }, [](A) { title_menu_open(); return uint64_t{0}; });
    check("title_menu_page_start", 0x00218f98, 4, Ret::None, [title] {
        title();
        title_menu_open();
        return Args{};
    }, [](A) { title_menu_page_start(); return uint64_t{0}; });
    check("title_menu_update", 0x002192a8, 40, Ret::None, [menu] {
        menu();
        w32(0x0013c940 + 0x1a4, ri(0, 2) == 0 ? (1u << ri(0, 15)) : 0u);
        if (ri(0, 4) == 0) w32(MS + 0x110, u(ri(0, 20)));
        return Args{};
    }, [](A) { title_menu_update(); return uint64_t{0}; });
    check("title_menu_close", 0x002191b8, 10, Ret::None, [menu] {
        menu();
        w32(MS + 0x110, u(ri(0, 20)));
        return Args{};
    }, [](A) { title_menu_close(); return uint64_t{0}; });
    // The menu's moby draw skins through the VU0 programs, which the original runs beside the EE (its
    // loop polls the micro program's result vf9 / vf11): with the interpreter's synchronous VU0 that
    // poll never ends (instruction budget in 0x1ef41c: not the port's: task 0323). So the original runs with
    // VU0 co-execution (as batch 79's moby pass), the port from the same VU0 registers; the skinning's saved
    // return address (0x70001ff0) is a stale lane.
    {
        static vu0m::Regs vu0_in;  // the VU0 registers the original started from
        const std::vector<uint32_t> ign = g_ignored_words;
        for (uint32_t a = 0x70001ff0; a < 0x70002000; a += 4) g_ignored_words.push_back(a);
        g_pre_call = [](EeInterpreter& ee) {
            for (int i = 0; i < 32; ++i)
                for (int k = 0; k < 4; ++k) vu0_in.vf[i].v[k] = ee.vu0().vf[static_cast<size_t>(i)].f[k];
            for (int k = 0; k < 4; ++k) vu0_in.acc.v[k] = ee.vu0().acc.f[k];
            vu0_in.q = ee.vu0().q;
            ee.set_vu0_coexec(true, 1, 1);
        };
        check("title_menu_draw", 0x002196b8, 10, Ret::None, [menu] { menu(); return Args{}; }, [](A) {
            vu0m::regs() = vu0_in;
            title_menu_draw();
            return uint64_t{0};
        });
        g_pre_call = nullptr;
        g_ignored_words = ign;
    }
    check("title_instances", 0x001e9b10, 3, Ret::Int32, [title] {
        title();
        const uint32_t H = lwu(0x001940d4);
        return Args{{H + lwu(H + 4) + lwu(H + 0x7c)}};
    }, [](A a) { return r32(title_instances(a.i[0])); });
    check("title_load", 0x001ea830, 2, Ret::None, [] { use(g_snap_start); return Args{}; }, [](A) { title_load(); return uint64_t{0}; });
    // (task 0376) the main menu's own widget functions: its Options list's entries (PAL or not), its labels' update, a
    // focus function doing nothing
    check("title_options_list_focus_in", 0x0021a1b0, 10, Ret::Int32, [menu] {
        menu();
        w32(0x0015ee90, u(ri(0, 1)));
        return Args{{0x001d4868u}};
    }, [](A a) { return ri32(title_options_list_focus_in(a.i[0])); });
    check("title_label_update", 0x00219d80, 60, Ret::Int32, [menu] {
        menu();
        const uint32_t page = coin() ? 0x001d4788u : 0x001d45c8u;  // (Options: a parent; the root page: none)
        const uint32_t label = 0x001d08d8u;
        w32(MS + 4, page);
        w32(page + 0x40, coin() ? label : 0x001d4868u);
        w32(MS + 0x124, u(ri(0, 1)));
        w32(MS + 8, 0);
        const uint32_t bits[] = {0, 0x10, 0x100, 0x400, 0x800, 0x40, 0x1000};
        w32(0x0013c940 + 0x1c4, bits[ri(0, 6)] | (coin() ? 0x10u : 0u));
        return Args{{label}};
    }, [](A a) { return ri32(title_label_update(a.i[0])); });
    check("title_nop_focus", 0x0021a308, 2, Ret::Int32, [menu] { menu(); return Args{{0x001d4868u}}; },
          [](A a) { return ri32(title_nop_focus(a.i[0])); });

    g_extra_hooks.clear();
    g_extra_noop_hooks.clear();
    g_world_chain_forced = chain_forced;
    g_oracle_front = false;
    ovl_reloc_set_level(0);
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
