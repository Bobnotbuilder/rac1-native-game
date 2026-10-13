// Oracle batch 73: the full-screen menu (game mode 3: the pause menu), its goals, its update and
// its drawing, on Veldin's real level data (needs the ISO).

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/pause_menu.h>
#include <runtime/kernel/disc.h>

namespace rac1::oracle {

void batch_73() {
    if (g_only_batch != 73) return;
    auto boot = [] { veldin_boot(); };
    g_batch = 73;
    g_ignored_words = vcross_lanes();
    // the callees left TODO_PORT: the kernel's vsync wait, the frame capture, the planet map's
    // data, the packet kick, the help screens, the mode-3 screens, the VU moby draw
    g_extra_noop_hooks = {0x00122298, OVF(0x002a0f18), OVF(0x002a08c8), 
                          OVF(0x00299a68), OVF(0x0024fe50), OVF(0x0024ff08), OVF(0x0024ff88), OVF(0x0024fad0)};  // (the music update 0x265aa8 runs for real on both sides since task 0323: the screen 5's scene start calls it)
    // 989snd's IOP server as batch 80's: the replies a function of the command; the stream system's disc reads (command
    // 0x38) done into EE RAM as the IRX does them (the screen 5's scene start reads its blocks, task 0323)
    {
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
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    static const uint32_t MS = OV(0x001b9cf0), MOBYS = OV(0x001b9e90), PAD = 0x0013c940;
    static std::vector<uint8_t> snap, snap_menu;
    auto packet = [] {  // the frame packet: buffer 0, the cursor at a random point of it
        w32(OV(0x001611b8), ee_read<uint32_t>(OV(0x00173e08)));
        w32(OV(0x001611bc), ee_read<uint32_t>(OV(0x00173e08)) + 0x30000);
        w32(OV(0x001611d0), 0);
        w32(OV(0x001611c0), ee_read<uint32_t>(OV(0x001611b8)) + u(ri(0, 0x100)) * 0x10);
    };
    auto level = [boot, packet] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            boot();
            ee_write<uint16_t>(0x001518d0, 512);  // the display size (the host's, from the boot module)
            ee_write<uint16_t>(0x001518d2, 416);
            particle_table_init();
            level_setup_body(1, 0);
            hud_init();  // (the overlay's start: the menu's moby list 0x15fa70)
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            snap.assign(ram, ram + 0x02000000);
            // the class update functions not ported yet: no-ops on both sides
            for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); ee_read<uint8_t>(m + 0x20) != 0xff; m += 0x100) {
                const uint32_t fn = ee_read<uint32_t>(m + 0x74);
                if (fn != 0 && !ovl_moby_fn_ported(fn)) g_extra_noop_hooks.push_back(fn);
            }
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
        packet();
    };
    // the pause menu open on its main page (the port's frames from Start)
    auto menu = [level, packet] {
        uint8_t* ram = ee_get_ram_base();
        if (snap_menu.empty()) {
            level();
            w32(OV(0x0016c06c), 0);
            menu_open(0);
            for (int f = 0; f < 16; ++f) {
                w32(PAD + 0x1c4, 0);
                w32(PAD + 0x1b4, 0);
                menu_update();
            }
            snap_menu.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap_menu.data(), snap_menu.size());
        }
        packet();
    };
    // the save data the goals read: items, gold bolts, levels, moby flags
    auto progress = [] {
        for (uint32_t i = 0; i < 0x80; ++i) w8(0x0013d388 + i, ri(0, 2) == 0 ? 1u : 0u);
        for (uint32_t i = 0; i < 0x50; ++i) w8(0x0013d4c0 + i, ri(0, 2) == 0 ? 1u : 0u);
        for (uint32_t i = 0; i < 0x79; ++i) if (ri(0, 3) == 0) w32(0x0013d5b0 + i * 16 + 0xc, u(ri(0, 3)));
        for (uint32_t a : {0x0013d73cu, 0x0013d8acu, 0x0013d9dcu, 0x0013da1cu, 0x0013db2cu, 0x0013d6fcu, 0x0013d73cu, 0x0013d9bcu, 0x0013d71cu})
            w32(a, coin() ? 0u : u(ri(1, 3)));
        for (uint32_t i = 0; i < 0x40; ++i) w8(0x0013dd40 + i, ri(0, 2) == 0 ? 1u : 0u);
        for (uint32_t i = 0; i < 0x100; ++i) if (ri(0, 4) == 0) w8(0x0014bec0 + i, R());
        for (uint32_t i = 0; i < 0x20; ++i) w8(0x0013d408 + i, ri(0, 1));
        for (uint32_t i = 0; i < 0x25; ++i) w8(0x0013e520 + i, ri(0, 3) == 0 ? 1u : 0u);
    };
    static const uint32_t tests7[] = {OVF(0x0024dfe8), OVF(0x0024e010), OVF(0x0024e048), OVF(0x0024e078), OVF(0x0024e0a0), OVF(0x0024e0c8), OVF(0x0024e0f0)};
    check("goal_cond", 0x0024dda8, 200, Ret::Int32, [level, progress] {
        level();
        progress();
        const int32_t k = ri(-1, 10);
        uint32_t a = u(ri(0, 0x40));
        if (k == 4 || k == 5) a = u(ri(0, 0x80));
        if (k == 7) a = tests7[ri(0, 6)];
        if (k == 8) a = (u(ri(0, 8)) << 16) | u(ri(0, 0x20));
        return Args{{u(k), a}, {}};
    }, [](A a) { return ri32(goal_cond(static_cast<int32_t>(a.i[0]), a.i[1])); });
    check("goals_update", 0x0024dc08, 120, Ret::Int32, [level, progress] {
        level();
        progress();
        w32(OV(0x00184414), u(ri(0, 4) == 0 ? ri(0, 19) : ri(0, 2)));
        return Args{};
    }, [](A) { return ri32(goals_update()); });
    check("skill_points_count", 0x00264640, 30, Ret::Int32, [level, progress] { level(); progress(); return Args{}; }, [](A) { return ri32(skill_points_count()); });
    check("gold_bolts_count", 0x002645f8, 30, Ret::Int32, [level, progress] { level(); progress(); return Args{}; }, [](A) { return ri32(gold_bolts_count()); });
    check("cheats_menu_entries", 0x00284ca0, 40, Ret::None, [level, progress] {
        level();
        progress();
        if (coin()) for (uint32_t i = 0; i < 0x20; ++i) w8(0x0013d408 + i, 1);
        if (coin()) for (uint32_t i = 0; i < 0x25; ++i) w8(0x0013e520 + i, 1);
        return Args{};
    }, [](A) { cheats_menu_entries(); return uint64_t{0}; });
    check("menu_open", 0x00276190, 80, Ret::None, [level, progress] {
        level();
        progress();
        w32(OV(0x0016c06c), ri(0, 5) == 0 ? 1u : 0u);
        w32(0x00141660, coin() ? 0x24u : u(ri(0, 0x30)));
        w32(0x0015ed84, u(ri(0, 4) == 0 ? ri(0, 20) : 0));
        w8(0x0014161b, ri(0, 3) == 0 ? 1u : 0u);
        w32(0x0015eea0, ri(0, 3) == 0 ? 1u : 0u);
        w32(0x0015ee20, ri(0, 3) == 0 ? 1u : 0u);
        w32(MS + 0xf8, ri(0, 3) == 0 ? 1u : 0u);
        return Args{{coin() ? 0u : (coin() ? 0x23u : u(ri(0, 0x30)))}, {}};
    }, [](A a) { menu_open(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    check("menu_camera_set", 0x00276128, 5, Ret::None, [level] { level(); return Args{}; }, [](A) { menu_camera_set(); return uint64_t{0}; });
    check("menu_vram_layout", 0x00283908, 20, Ret::None, [level] {
        level();
        w32(MS + 0x108, R() & 0x1fffff0u);
        w32(MS + 0x10c, R() & 0x1fffff0u);
        return Args{{u(ri(0, 1))}, {}};
    }, [](A a) { menu_vram_layout(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    check("moby_lists_swap", 0x00235668, 20, Ret::None, [level] {
        level();
        w32(OV(0x0015f728), u(ri(0, 1)));
        wrand(OV(0x0015fa70), 4);
        return Args{{u(ri(0, 1))}, {}};
    }, [](A a) { moby_lists_swap(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    check("hud_tex_unrelocate", 0x00235310, 20, Ret::None, [level] { level(); return Args{{u(ri(0, 4))}, {}}; },
          [](A a) { hud_tex_unrelocate(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    check("menu_moby_spawn", 0x002830d8, 20, Ret::Int32, [level] {
        level();
        return Args{{ri(0, 3) == 0 ? u(ri(0, 0x500)) : 0x472u}, {}};
    }, [](A a) { return r32(menu_moby_spawn(a.i[0])); });
    check("menu_page_start", 0x00276368, 40, Ret::None, [level] {
        level();
        w32(OV(0x0016c06c), 0);
        static const int32_t states[] = {0, 0, 0, 0x2d, 0x23, 0x21, 0xa, 0xe};
        menu_open(states[ri(0, 7)]);
        if (ri(0, 4) == 0) { w32(MS + 0xf0, OV(0x001b2db8)); w32(MS + 0xf4, coin() ? 3u : 0xbu); w32(MS + 0xe4, u(ri(0, 3))); }
        return Args{};
    }, [](A) { menu_page_start(); return uint64_t{0}; });
    // the menu open: its mobys (their update: the panel corners), its widgets
    check("menu_moby_update", 0x002df638, 30, Ret::None, [menu] {
        menu();
        const uint32_t m = ee_read<uint32_t>(MOBYS + u(ri(0, 13)) * 4);
        if (coin()) w8(m + 0x70, R());
        wf(m + 0x58, rf(-1, 1));
        return Args{{m}, {}};
    }, [](A a) { if (a.i[0] != 0) menu_moby_update(a.i[0]); return uint64_t{0}; });
    check("menu_moby_free", 0x00283178, 20, Ret::Int32, [menu] {
        menu();
        w32(OV(0x0015f5cc), R());
        return Args{{coin() ? ee_read<uint32_t>(MOBYS + u(ri(0, 13)) * 4) : 0u}, {}};
    }, [](A a) { return ri32(menu_moby_free(a.i[0])); });
    check("menu_hud_tex_load", 0x00234fd0, 12, Ret::None, [menu] { menu(); return Args{{u(ri(0, 3))}, {}}; },
          [](A a) { menu_hud_tex_load(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    // buttons: their pad (+0x1c4 pressed, +0x1b4 repeated), the selection, the timers
    auto buttons = [] {
        static const uint32_t b[] = {0x1000, 0x4000, 0x40, 0x10, 0x800, 0x100, 0x400, 4, 8, 0x20, 0};
        const uint32_t k = b[ri(0, 10)] | (ri(0, 4) == 0 ? b[ri(0, 10)] : 0u);
        w32(PAD + 0x1c4, k);
        w32(PAD + 0x1b4, coin() ? k : b[ri(0, 10)]);
    };
    auto widget = [] {
        const uint32_t page = ee_read<uint32_t>(MS + 4);
        if (page == 0) return 0u;
        const uint32_t w = ee_read<uint32_t>(page + 0x44 + u(ri(0, 6)) * 4);
        if (w == 0) return 0u;
        if (ri(0, 2) != 0) w32(page + 0x40, w);
        for (uint32_t p = ee_read<uint32_t>(w + 0x34); ee_read<int16_t>(p) != 0; p += 0xc) w16(p + 0xa, u(ri(-3, 40)));
        if (ri(0, 3) == 0) w32(w + 0x30, ee_read<uint32_t>(w + 0x30) ^ (1u << ri(0, 16)));
        return w;
    };
    check("menu_button_update", 0x00278840, 200, Ret::Int32, [menu, buttons, widget] {
        menu();
        buttons();
        w32(0x0015eeb0, u(ri(0, 3) == 0 ? 0x10 : ri(0, 2)));
        w32(OV(0x00160274), u(ri(0, 30)));
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        return Args{{widget()}, {}};
    }, [](A a) { return ri32(a.i[0] != 0 ? menu_button_update(a.i[0]) : 0); });
    check("menu_save_entry_focus", 0x0027ba18, 10, Ret::Int32, [menu] {
        menu();
        w8(0x001413f4, u(ri(0, 2)));
        return Args{{OV(0x001b26b8)}, {}};
    }, [](A a) { return ri32(menu_save_entry_focus(a.i[0])); });
    // the cheat codes: a code's first 19 presses entered (or random ones), the 20th pressed
    check("cheat_code_input", 0x002831c0, 120, Ret::None, [menu, progress] {
        menu();
        progress();
        static const uint32_t SEQ = OV(0x001ba4a8), TAB = OV(0x001b9ba0);
        static const uint32_t keys[] = {0x1000, 0x4000, 0x8000, 0x2000, 0x80, 0x20};
        ee_write<uint64_t>(PAD + 0x1c0, (ee_read<uint64_t>(PAD + 0x1c0) & ~0xfull) | (ri(0, 5) == 0 ? u(ri(0, 15)) : 6u));
        const int32_t s = ri(2, 0x92);
        const bool real = ri(0, 2) != 0;
        for (uint32_t i = 0; i < 19; ++i) w16(SEQ + i * 2, real ? ee_read<uint8_t>(TAB + ((i + 1) * u(s) & 0xff)) : u(ri(0, 5)));
        w32(OV(0x001603f8), ri(0, 3) == 0 ? u(ri(0, 20)) : 19u);
        const uint32_t last = real ? ee_read<uint8_t>(TAB + (20 * u(s) & 0xff)) : u(ri(0, 5));
        w32(PAD + 0x1c4, last < 6 ? keys[last] : 0u);
        return Args{};
    }, [](A) { cheat_code_input(); return uint64_t{0}; });
    check("menu_update", 0x00276bd0, 160, Ret::None, [menu, buttons, widget] {
        menu();
        buttons();
        if (ri(0, 2) == 0) widget();
        w32(MS + 0x110, u(ri(0, 20)));
        const int32_t k = ri(0, 7);
        if (k == 0) { w32(MS, 0x14); w32(MS + 0x14, u(ri(0, 2))); w32(MS + 0xc, u(ri(0, 8))); w16(0x001516d8, ri(0, 4) == 0 ? 1u : 0u); w32(MS + 0xe4, u(ri(-2, 0x10))); static const uint32_t movie_kinds[] = {3u, 4u, 6u, 5u}; if (ri(0, 1) == 0) w32(MS + 0xc, movie_kinds[ri(0, 3)]);
            if (ee_read<uint32_t>(MS + 0xc) == 5) {  // screen 5: the scene +0xe4 started (0x298840, task 0323: batch 80's start setup)
                const uint32_t S = 0x001516d0, P = 0x0013f350;
                std::vector<int32_t> ks;
                for (int32_t s = 0; s < 0x20 && ee_read<int32_t>(0x00137b80 + 0x2afc + u(s) * 0x250) != 0; ++s) {  // the level's scenes with two blocks or more
                    const uint32_t row = 0x00137b80 + 0x2afc + u(s) * 0x250;
                    int32_t n = 0;
                    while (n < 0x93 && ee_read<int32_t>(row + u(n + 1) * 4) > ee_read<int32_t>(row + u(n) * 4)) ++n;
                    if (n >= 2) ks.push_back(s);
                }
                w32(MS + 0xe4, ri(0, 8) == 0 ? 0xffffffffu : u(ks.empty() ? 4 : ks[static_cast<size_t>(ri(0, static_cast<int32_t>(ks.size()) - 1))]));
                w16(S + 0x5a, 3);  // the scene's sound ready (the start's wait loop not entered)
                w32(S + 0x50, (R() & 0x7ffffffeu) | 1u);
                w16(S + 0x5c, 0);
                w32(S + 0x1c, 0xffffffffu);
                w32(P + 0x22a8, ri(0, 8) == 0 ? 0u : 1u);
                w32(OV(0x00179100) + 8, 0);
            }
            if (ee_read<uint32_t>(MS + 0xc) == 5) {  // the scene start reads its blocks through the stream system
                w32(0x0015ec8c, 1);  // up, the disc idle (batch 80's level); without it the original's sceCdRead waits for a CD the interpreter does not model
                w32(0x00137b00, 0);
                sub_00264bf8();
            }
            if (ee_read<uint32_t>(MS + 0xc) == 2) w32(MS + 0xc, 0);  // (the planet map: planet_travel's ship sequence / level change request need the CD clock and the ship's data the interpreter does not model: batch 84's)
        }  // (+0xe4: the movie / screen number of the kinds 3 / 4 / 6, task 0319)
        if (k == 1) { w32(MS, 1); w32(MS + 0x14, u(ri(0, 2))); w32(MS + 8, ee_read<uint32_t>(MS + 4)); w32(MS + 4, 0); }
        if (k == 2) w32(MS + 8, coin() ? ee_read<uint32_t>(MS + 4) : OV(0x001b2db8u));
        if (k == 3) w32(0x0015eeb4, R());
        w32(OV(0x0015f5c8), u(ri(0, 12)));
        return Args{};
    }, [](A) { menu_update(); return uint64_t{0}; });
    check("menu_close", 0x00276908, 30, Ret::None, [menu] {
        menu();
        w32(MS + 0x110, u(ri(0, 20)));
        for (uint32_t k = 0; k < 4; ++k) if (coin()) w32(0x00141660 + k * 4, u(ri(0, 0x30)));
        return Args{};
    }, [](A) { menu_close(); return uint64_t{0}; });
    // the drawing
    check("gs_set_def_draw_env", 0x00121fc8, 30, Ret::Int32, [level] {
        level();
        return Args{{OUT, u(ri(0, 2) == 0 ? 2 : 0), u(1 << ri(5, 9)), u(1 << ri(5, 9)), u(ri(0, 3)), u(ri(0, 3))}, {}};
    }, [](A a) { return ri32(gs_set_def_draw_env(a.i[0], static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]), static_cast<int32_t>(a.i[5]))); });
    check("gs_flat_rect", 0x0023caa8, 30, Ret::None, [level] {
        level();
        return Args{{u(ri(-20, 520)), u(ri(-20, 460)), u(ri(-20, 520)), u(ri(-20, 460)), R(), u(ri(0, 1))}, {}};
    }, [](A a) { gs_flat_rect(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<uint64_t>(sx(a.i[4])), static_cast<int32_t>(a.i[5])); return uint64_t{0}; });
    check("gs_flat_rect_z", 0x0023cc30, 30, Ret::None, [level] {
        level();
        return Args{{u(ri(-20, 520)), u(ri(-20, 460)), u(ri(-20, 520)), u(ri(-20, 460)), R(), R(), u(ri(0, 1))}, {}};
    }, [](A a) { gs_flat_rect_z(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<uint64_t>(sx(a.i[4])), static_cast<uint64_t>(sx(a.i[5])), static_cast<int32_t>(a.i[6])); return uint64_t{0}; });
    check("hud_sprite_xyz", 0x0023bd20, 30, Ret::None, [level] {
        level();
        const int32_t f = hud_frame(0xe99e, 7);
        return Args{{u(f != 0 && coin() ? f : ri(0, 40)), u(ri(-20, 520) << 4), u(ri(-20, 460) << 4), u(ri(0, 300) << 4), u(ri(0, 300) << 4), u(ri(0, 0x80))}, {}};
    }, [](A a) { hud_sprite_xyz(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]), static_cast<int32_t>(a.i[5])); return uint64_t{0}; });
    check("gs_upload_rows", 0x002a0fc8, 20, Ret::None, [level] {
        level();
        return Args{{0x01000000u + (R() & 0xfff00u), R() & 0x3fffu, u(ri(-10, 0x10000))}, {}};
    }, [](A a) { gs_upload_rows(a.i[0], static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2])); return uint64_t{0}; });
    check("render_target_set", 0x002010b0, 30, Ret::None, [level] {
        level();
        return Args{{u(ri(5, 9)), u(ri(5, 9)), (R() & 0x3fe000u)}, {}};
    }, [](A a) { render_target_set(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2])); return uint64_t{0}; });
    check("render_to_texture_begin", 0x001fc928, 30, Ret::None, [level] {
        level();
        return Args{{u(ri(7, 9)), u(ri(7, 9)), u(ri(0, 1))}, {rf(0.5f, 2.0f)}};
    }, [](A a) { render_to_texture_begin(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), a.f[0]); return uint64_t{0}; });
    check("render_to_texture_end", 0x001fca18, 10, Ret::None, [level] { level(); return Args{}; }, [](A) { render_to_texture_end(); return uint64_t{0}; });
    check("menu_highlight_colour", 0x00279320, 40, Ret::Int32, [level] {
        level();
        w32(OV(0x00160274), u(ri(0, 30)));
        return Args{{u(ri(-5, 40)), coin() ? 0xffffffffu : R(), coin() ? 0xffffffffu : R()}, {}};
    }, [](A a) { return r32(menu_highlight_colour(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]))); });
    check("menu_box_project", 0x00299f40, 30, Ret::None, [menu] {
        menu();
        const uint32_t d = ee_read<uint32_t>(ee_read<uint32_t>(MOBYS + u(ri(0, 13)) * 4) + 0x78);
        if (ri(0, 3) == 0) wfrand(d, 12, 200.0f, 300.0f);
        return Args{{d, d + 0x30, OUT, OUT + 4, OUT + 8, OUT + 12}, {}};
    }, [](A a) { menu_box_project(a.i[0], a.i[1], a.i[2], a.i[3], a.i[4], a.i[5]); return uint64_t{0}; });
    check("menu_button_draw", 0x00278e10, 60, Ret::Int32, [menu, widget] {
        menu();
        const uint32_t w = widget();
        if (w != 0) {
            w32(w + 0x20, u(ri(0, 300)));
            w32(w + 0x24, u(ri(0, 200)));
        }
        w32(OV(0x00160274), u(ri(0, 30)));
        return Args{{w}, {}};
    }, [](A a) { return ri32(a.i[0] != 0 ? menu_button_draw(a.i[0]) : 0); });
    check("menu_panel_draw", 0x00281a70, 40, Ret::None, [menu] {
        menu();
        const uint32_t m = ee_read<uint32_t>(MOBYS + u(ri(0, 13)) * 4);
        const uint32_t d = ee_read<uint32_t>(m + 0x78);
        w32(d + 0x50, u(ri(-100, 600))); w32(d + 0x54, u(ri(-100, 500))); w32(d + 0x58, u(ri(0, 200))); w32(d + 0x5c, u(ri(0, 200)));
        w32(d + 0x48, u(ri(0, 1))); w32(d + 0x4c, u(ri(0, 0x110)));
        w32(0x0015ed80, u(ri(0, 1)));
        seed_rand();
        return Args{{m}, {}};
    }, [](A a) { menu_panel_draw(a.i[0]); return uint64_t{0}; });
    // (task 0366c) the menu mobys drawn through the moby pass (0x24fe50 / 0x24ff08 / 0x24ff88 / 0x24fad0, ported): run on both sides here
    const std::vector<uint32_t> hooks_before = g_extra_noop_hooks;
    g_extra_noop_hooks = {0x00122298, OVF(0x002a0f18), OVF(0x002a08c8), OVF(0x00299a68)};
    check("menu_draw", 0x002772c0, 30, Ret::None, [menu] {
        menu();
        // (the moby groups drawn 0x19a600, -1 ended, as batch 131: a snapshot that has not drawn them holds zeros, an endless list for 0x24fad0)
        if (ee_read<uint32_t>(OV(0x0019a600)) == 0) w32(OV(0x0019a600), 0xffffffffu);
        seed_rand();
        if (ri(0, 4) == 0) w32(MS, 0x14);
        w32(MS + 0xd8, u(ri(0, 1)));
        return Args{{ri(0, 5) == 0 ? 1u : 0u}, {}};
    }, [](A a) { menu_draw(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    g_extra_noop_hooks = hooks_before;
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
