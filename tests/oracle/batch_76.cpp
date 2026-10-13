// Oracle batch 76: the full-screen menu's other pages (Options and its sub-pages, Weapons,
// Gadgets, Items, Quick Select): their widget functions, on Veldin's real level data with the
// menu open on the page (needs the ISO).

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/pause_menu.h>
#include <game/ovl/pause_menu_pages.h>
#include <game/ovl/planet_map.h>

#include <map>

namespace rac1::oracle {

namespace {
constexpr uint32_t PAD = 0x0013c940, SPR_ADDR = 0x70000000;
constexpr OvConst<0x001b9cf0u> MS{};
constexpr OvConst<0x001ba1b8u> BUFS{};
// the pages (overlay data, menu.md §13.6)
constexpr OvConst<0x001b4950u> P_OPTIONS{};
constexpr OvConst<0x001b5598u> P_SOUND{};
constexpr OvConst<0x001b5a60u> P_CAMERA{};
constexpr OvConst<0x001b5308u> P_SUBTITLES{};
constexpr OvConst<0x001b4bc0u> P_HELPDESK{};
constexpr OvConst<0x001b5be0u> P_QUIT{};
constexpr OvConst<0x001b2db8u> P_WEAPONS{};
constexpr OvConst<0x001b28e8u> P_GADGETS{};
constexpr OvConst<0x001b32b8u> P_QUICK{};
constexpr OvConst<0x001b5cb8u> P_ITEMS{};
}  // namespace

void batch_76() {
    if (g_only_batch != 76) return;
    auto boot = [] { veldin_boot(); };
    g_batch = 76;
    g_ignored_words = vcross_lanes();
    // the callees left TODO_PORT (as in batch 73); (task 0408) the one-moby draw 0x24ff08 the panels' draws call runs on both sides
    g_extra_noop_hooks = {0x00122298, OVF(0x002a0f18), OVF(0x002a08c8), OVF(0x0028ed58),
                          OVF(0x00299a68), OVF(0x0024fe50), OVF(0x0024ff88), OVF(0x0024fad0),
                          OVF(0x00265aa8)};
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    static std::vector<uint8_t> snap, snap_menu;
    static std::map<uint32_t, std::vector<uint8_t>> snap_page;
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
            ee_write<uint16_t>(0x001518d0, 512);
            ee_write<uint16_t>(0x001518d2, 416);
            particle_table_init();
            level_setup_body(1, 0);
            hud_init();
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            w32(0x0015ec8c, 1);  // the stream system up (the menu's pictures are read through it)
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
    auto menu = [level, packet, frames] {
        uint8_t* ram = ee_get_ram_base();
        if (snap_menu.empty()) {
            level();
            w32(OV(0x0016c06c), 0);
            menu_open(0);
            frames(16);
            snap_menu.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap_menu.data(), snap_menu.size());
        }
        packet();
    };
    // the menu open on page p (the port's frames from the pause page)
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
    // the save data the pages read: items, gold bolts, skill points, cheats
    auto progress = [] {
        for (uint32_t i = 0; i < 0x80; ++i) w8(0x0013d388 + i, ri(0, 2) == 0 ? 1u : 0u);
        for (uint32_t i = 0; i < 0x50; ++i) w8(0x0013d4c0 + i, ri(0, 2) == 0 ? 1u : 0u);
        for (uint32_t i = 0; i < 0x20; ++i) w8(0x0013d408 + i, ri(0, 1));
        for (uint32_t i = 0; i < 0x25; ++i) w8(0x0013e520 + i, ri(0, 3) == 0 ? 1u : 0u);
    };
    auto buttons = [] {
        static const uint32_t b[] = {0x1000, 0x4000, 0x40, 0x10, 0x800, 0x100, 0x400, 4, 8, 0x20, 0x2000, 0x8000, 0};
        const uint32_t k = b[ri(0, 12)] | (ri(0, 4) == 0 ? b[ri(0, 12)] : 0u);
        w32(PAD + 0x1c4, k);
        w32(PAD + 0x1b4, coin() ? k : b[ri(0, 12)]);
    };
    // a widget of the page shown (slot k), maybe focused
    auto slot = [](uint32_t k) { return ee_read<uint32_t>(ee_read<uint32_t>(MS + 4) + 0x44 + k * 4); };
    auto focus_rand = [] {  // the page's focused list: one of its entries selected
        const uint32_t p = ee_read<uint32_t>(MS + 4);
        const uint32_t f = ee_read<uint32_t>(p + 0x40);
        if (f != 0 && ee_read<uint32_t>(f) == OVF(0x00278840)) {  // a list: one of its entries selected
            int32_t n = 0;
            for (uint32_t e = ee_read<uint32_t>(f + 0x34); ee_read<int16_t>(e) != 0; e += 0xc) ++n;
            w32(f + 0x40, u(ri(0, n - 1)));
        }
    };

    // --- Options (state 8): its title, its help label, its picture panel -----------------------
    check("menu_label_update", 0x002779c8, 60, Ret::Int32, [page, buttons] {
        page(P_OPTIONS);
        buttons();
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        const uint32_t p = ee_read<uint32_t>(MS + 4);
        const uint32_t w = ee_read<uint32_t>(p + 0x44 + u(ri(0, 1)) * 4);
        if (coin()) w32(p + 0x40, w);
        if (ri(0, 3) == 0) w32(p + 0x38, 0);
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_label_update(a.i[0])); });
    check("menu_label_focus", 0x00277f60, 5, Ret::Int32, [page, slot] { page(P_OPTIONS); return Args{{slot(1)}, {}}; },
          [](A a) { return ri32(menu_label_focus(a.i[0])); });
    check("menu_buffers_full", 0x0027a728, 5, Ret::Int32, [page] { page(P_OPTIONS); return Args{{0}, {}}; },
          [](A a) { return ri32(menu_buffers_full(a.i[0])); });
    check("menu_label_draw", 0x00277f70, 200, Ret::Int32, [page, progress, focus_rand, slot] {
        page(P_OPTIONS);
        progress();
        focus_rand();
        const uint32_t w = slot(u(ri(0, 1)));
        static const int32_t t[] = {-1, 0, 1, 2, 5, 20, 40};
        w32(w + 0x44, u(t[ri(0, 6)]));
        w32(w + 0x48, u(ri(-1, 6)));
        w32(w + 0x4c, u(ri(0, 1)));
        w32(w + 0x20, u(ri(100, 300)));  // its box (menu_draw's, from its moby)
        w32(w + 0x24, u(ri(60, 200)));
        if (coin()) w32(w + 0x30, ee_read<uint32_t>(w + 0x30) ^ 0x400);
        if (coin()) w32(w + 0x3c, u(ri(-2000, 200)));
        // flags that change the layout only (others pick texts the page does not have)
        static const uint32_t fl[] = {1, 2, 8, 0x10, 0x400, 0x2000, 0x4000, 0x10000};
        if (ri(0, 2) == 0) w32(w + 0x30, ee_read<uint32_t>(w + 0x30) ^ fl[ri(0, 7)]);
        w32(OV(0x00160274), u(ri(0, 30)));
        w32(0x0015ed84, u(ri(0, 20)));
        w32(OV(0x00184414), u(ri(0, 20)));
        w32(0x0015ed80, u(ri(0, 1)));
        w32(0x0013cae0, R());
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_label_draw(a.i[0])); });
    // the menu's buffers: random states
    auto bufs = [] {
        for (uint32_t i = 0; i < 5; ++i) if (ri(0, 2) == 0) w32(BUFS + i * 8 + 4, u(ri(0, 7)));
        if (ri(0, 4) == 0) w32(BUFS + u(ri(0, 4)) * 8, 0);
        w8(MS + 0xcb, u(ri(0, 1)));
        w16(0x001516d8, u(ri(0, 2) == 0 ? 1 : 0));
    };
    auto buf_any = [] { return coin() ? ee_read<uint32_t>(BUFS + u(ri(0, 4)) * 8) : R() & 0x1fffff0u; };
    check("menu_buf_alloc", 0x00283a60, 40, Ret::Int32, [page, bufs] { page(P_OPTIONS); bufs(); return Args{{u(ri(0, 1))}, {}}; },
          [](A a) { return r32(menu_buf_alloc(static_cast<int32_t>(a.i[0]))); });
    check("menu_buf_free", 0x00283b20, 40, Ret::Int32, [page, bufs, buf_any] { page(P_OPTIONS); bufs(); return Args{{buf_any()}, {}}; },
          [](A a) { return ri32(menu_buf_free(a.i[0])); });
    check("menu_buf_size", 0x00283bd0, 20, Ret::Int32, [page, bufs, buf_any] { page(P_OPTIONS); bufs(); return Args{{buf_any()}, {}}; },
          [](A a) { return ri32(menu_buf_size(a.i[0])); });
    check("menu_buf_busy", 0x00283c20, 20, Ret::Int32, [page, bufs, buf_any] { page(P_OPTIONS); bufs(); return Args{{buf_any()}, {}}; },
          [](A a) { return ri32(menu_buf_busy(a.i[0])); });
    check("menu_buf_ready", 0x00283c68, 20, Ret::Int32, [page, bufs, buf_any] { page(P_OPTIONS); bufs(); return Args{{buf_any()}, {}}; },
          [](A a) { return ri32(menu_buf_ready(a.i[0])); });
    check("disc_read_break", 0x00265f00, 10, Ret::None, [page] { page(P_OPTIONS); w16(0x001516d8, u(ri(0, 1))); return Args{}; },
          [](A) { disc_read_break(); return uint64_t{0}; });
    check("image_upload", 0x00245a20, 30, Ret::Int64, [page] {
        page(P_OPTIONS);
        const uint32_t img = 0x01000000u + (R() & 0xfff0u);
        w32(img + 8, 1u << ri(4, 9));
        w32(img + 0xc, 1u << ri(4, 9));
        w32(0x0015ee74, R() & 0x3fff00u);
        w32(OV(0x0015f418), u(ri(0, 3) == 0 ? 0x40 : ri(0, 0x3f)));
        return Args{{img}, {}};
    }, [](A a) { return image_upload(a.i[0]); });
    // the picture panel (slot 2): its states, its buffers, the image it wants
    auto picture = [page, progress, focus_rand, slot] {
        page(P_OPTIONS);
        progress();
        focus_rand();
        const uint32_t w = slot(2);
        w32(w + 0x44, u(ri(-1, 7)));
        w32(w + 0x50, u(ri(-1, 6)));
        w32(w + 0x54, u(ri(-1, 6)));
        w32(w + 0x5c, u(ri(0, 2000)));
        w32(w + 0x60, u(ri(0, 0x100)) * 0x100);
        static const uint32_t fl[] = {0x10, 0x20, 0x200, 0x2000};
        if (ri(0, 2) == 0) w32(w + 0x34, ee_read<uint32_t>(w + 0x34) ^ fl[ri(0, 3)]);
        const int32_t st = ee_read<int32_t>(w + 0x44);
        if (st != 1 && st != 3 && st != 5) {  // (an unpack has its buffer)
            if (ri(0, 4) == 0) w32(w + 0x48, 0);
            if (ri(0, 4) == 0) w32(w + 0x4c, 0);
        }
        for (uint32_t k : {0x48u, 0x4cu}) {  // packed data read (an empty WAD stream) where an unpack finds it
            const uint32_t b = ee_read<uint32_t>(w + k);
            if (b == 0) continue;
            const uint32_t src = b + ee_read<uint32_t>(w + 0x60);
            for (uint32_t o = 0; o < 0x40; o += 4) w32(src + o, 0);
            w32(src, 0x10444157u);  // "WAD", size 0x10
        }
        w16(0x001516d8, u(ri(0, 3) == 0 ? 1 : 0));
        if (ee_read<int16_t>(0x001516d8) == 0) w32(0x00137b00, 0);  // no read going: the stream system idle
        w32(0x0013d290 + 8, u(ri(0, 3)));
        w32(0x0013d290 + 0xd4, u(ri(0, 4)));
        w32(0x0013d290 + 0xdc, u(ri(-2, 2)));
        w32(0x0015ed88, u(ri(0, 5)));
        w32(OV(0x00184414), u(ri(0, 18)));
        return w;
    };
    check("menu_picture_update", 0x0027da10, 200, Ret::Int32, [picture] { return Args{{picture()}, {}}; },
          [](A a) { return ri32(menu_picture_update(a.i[0])); });
    check("menu_picture_draw", 0x0027df90, 100, Ret::Int32, [picture] {
        const uint32_t w = picture();
        const uint32_t b = ee_read<uint32_t>(w + (coin() ? 0x48 : 0x4c));
        if (b != 0) {  // an image in the buffer
            w32(b + 8, 1u << ri(5, 8));
            w32(b + 0xc, 1u << ri(5, 8));
        }
        w32(MS + 0x128, u(ri(0, 1)));
        w32(MS + 0x12c, 0x4fb9);
        w32(OV(0x0015f418), u(ri(0, 0x3f)));
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_picture_draw(a.i[0])); });
    check("menu_picture_focus_in", 0x0027d928, 30, Ret::Int32, [picture, bufs] {
        const uint32_t w = picture();
        bufs();
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_picture_focus_in(a.i[0])); });
    check("menu_picture_focus_out", 0x0027d9c0, 30, Ret::Int32, [picture, bufs] {
        const uint32_t w = picture();
        bufs();
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_picture_focus_out(a.i[0])); });
    // --- Options' sub-pages: Sound, Camera, Subtitles, Help Desk, Quit -------------------------
    auto box = [](uint32_t w) {  // its box (menu_draw's, from its moby)
        w32(w + 0x20, u(ri(120, 300)));
        w32(w + 0x24, u(ri(80, 200)));
    };
    check("text_small", 0x001fb370, 20, Ret::None, [level] {
        level();
        return Args{{u(ri(-20, 500)), u(ri(-20, 400)), R(), coin() ? OV(0x00160330u) : OV(0x001b2610u), u(ri(-1, 5))}, {}};
    }, [](A a) { text_small(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<uint64_t>(sx(a.i[2])), a.i[3], static_cast<int32_t>(a.i[4])); return uint64_t{0}; });
    check("hud_sprite_uv", 0x0023c5f8, 30, Ret::None, [level] {
        level();
        const int32_t f = hud_frame(0xe99e, ri(0, 9));
        return Args{{u(f), u(ri(-20, 520) << 4), u(ri(-20, 460) << 4), u(ri(-20, 520) << 4), u(ri(-20, 460) << 4), u(ri(0, 0x200)), u(ri(0, 0x200)),
                     u(ri(0, 0x200)), u(ri(0, 0x200)), u(ri(0, 0x80))}, {}};
    }, [](A a) {
        hud_sprite_uv(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]),
                      static_cast<int32_t>(a.i[5]), static_cast<int32_t>(a.i[6]), static_cast<int32_t>(a.i[7]), static_cast<int32_t>(a.i[8]), static_cast<int32_t>(a.i[9]));
        return uint64_t{0};
    });
    auto sound = [page, buttons, slot] {
        page(P_SOUND);
        buttons();
        if (coin()) w32(PAD + 0x1c0, ee_read<uint32_t>(PAD + 0x1c0) | (coin() ? 0x2000u : 0x8000u));
        const uint32_t w = slot(3);
        w32(w + 0x40, u(ri(0, 2)));
        w32(0x0015edf0, u(ri(0, 0x400)));
        w32(0x0015edec, u(ri(0, 0x400)));
        w32(0x0015ede8, u(ri(0, 1)));
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        if (coin()) w32(ee_read<uint32_t>(MS + 4) + 0x80, 0);
        return w;
    };
    check("menu_sound_update", 0x0027a778, 150, Ret::Int32, [sound] { return Args{{sound()}, {}}; },
          [](A a) { return ri32(menu_sound_update(a.i[0])); });
    check("menu_sound_leave", 0x0027a748, 10, Ret::Int32, [sound] { return Args{{sound()}, {}}; },
          [](A a) { return ri32(menu_sound_leave(a.i[0])); });
    check("menu_sound_draw", 0x0027aa48, 60, Ret::Int32, [sound, box] { const uint32_t w = sound(); box(w); return Args{{w}, {}}; },
          [](A a) { return ri32(menu_sound_draw(a.i[0])); });
    // a list of settings / switches: the selection, the bytes they set
    auto settings = [page, buttons, slot](uint32_t p, uint32_t stride, bool cycle) {
        page(p);
        buttons();
        const uint32_t w = slot(3);
        int32_t n = 0;
        for (uint32_t e = ee_read<uint32_t>(w + 0x34); ee_read<uint32_t>(e) != 0; e += stride) {
            const uint32_t b = ee_read<uint32_t>(e + 4);
            if (b != 0) {
                int32_t k = 0;  // a setting: one of its values
                while (cycle && k < 4 && ee_read<uint32_t>(e + 8 + u(k) * 4) != 0) ++k;
                w8(b, cycle ? u(ri(0, k > 0 ? k - 1 : 0)) : u(ri(0, 1)));
            }
            ++n;
        }
        w32(w + 0x38, u(ri(0, n > 0 ? n - 1 : 0)));
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        if (ri(0, 4) == 0) w32(ee_read<uint32_t>(MS + 4) + 0x40, 0);
        return w;
    };
    check("menu_settings_update", 0x0027ef00, 120, Ret::Int32, [settings] { return Args{{settings(P_CAMERA, 0x18, true)}, {}}; },
          [](A a) { return ri32(menu_settings_update(a.i[0])); });
    check("menu_settings_draw", 0x0027f0a8, 40, Ret::Int32, [settings, box] {
        const uint32_t w = settings(P_CAMERA, 0x18, true);
        box(w);
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_settings_draw(a.i[0])); });
    auto switches = [settings] {
        const uint32_t w = settings(coin() ? P_SUBTITLES : P_HELPDESK, 0x14, false);
        w32(w + 0x3c, ri(0, 2) == 0 ? u(ri(0, 8)) : 0u);
        w8(OV(0x0016040c), u(ri(0, 1)));
        return w;
    };
    check("menu_switches_update", 0x0027ea70, 120, Ret::Int32, [switches] { return Args{{switches()}, {}}; },
          [](A a) { return ri32(menu_switches_update(a.i[0])); });
    check("menu_switches_draw", 0x0027ec78, 40, Ret::Int32, [switches, box] {
        const uint32_t w = switches();
        box(w);
        if (ri(0, 3) == 0) w32(w + 0x30, ee_read<uint32_t>(w + 0x30) | 1);
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_switches_draw(a.i[0])); });
    check("menu_switches_draw (none)", 0x0027ec78, 10, Ret::None, [switches, box] {
        const uint32_t w = switches();
        box(w);
        w32(w + 0x30, ee_read<uint32_t>(w + 0x30) | 1);
        const uint32_t e = ee_read<uint32_t>(w + 0x34);
        w32(e, 0);  // (no switches: the page's text in its box)
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_switches_draw(a.i[0])); });
    check("menu_quit_update", 0x0027c410, 60, Ret::Int32, [page, buttons, slot] {
        page(P_QUIT);
        buttons();
        const uint32_t w = slot(3);
        if (coin()) w32(w + 0x30, ee_read<uint32_t>(w + 0x30) ^ 0x20);
        w32(0x0015ed84, u(ri(0, 18)));
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_quit_update(a.i[0])); });
    check("menu_quit_draw", 0x0027c4d8, 30, Ret::Int32, [page, slot, box] {
        page(P_QUIT);
        const uint32_t w = slot(3);
        box(w);
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_quit_draw(a.i[0])); });
    // --- the item grids: Weapons, Gadgets, Quick Select, Items ----------------------------------
    auto grid = [page, buttons, progress, box](bool focus) {
        static const uint32_t pages[] = {P_WEAPONS, P_GADGETS, P_QUICK, P_ITEMS};
        static const uint32_t grids[][4] = {{OV(0x001b2f58), 0, 0, 0}, {OV(0x001b2b80), OV(0x001b2be8), OV(0x001b2c50), OV(0x001b2cb8)}, {OV(0x001b3398), OV(0x001b3400), 0, 0}, {OV(0x001b5e60), 0, 0, 0}};
        const int32_t k = ri(0, 3);
        page(pages[k]);
        progress();
        buttons();
        uint32_t w = 0;
        while (w == 0) w = grids[k][ri(0, 3)];
        if (focus) w32(ee_read<uint32_t>(MS + 4) + 0x40, w);
        w32(w + 0x3c, u(ri(0, ee_read<int32_t>(w + 0x40) * ee_read<int32_t>(w + 0x44) - 1)));
        for (uint32_t s = 0; s < 4; ++s) w32(MS + 0x30 + s * 4, u(ri(0, 3) == 0 ? 0 : ri(0, 0x30)));
        w32(MS + 0x134, u(ri(0, 1)));
        w32(MS + 0x138, u(ri(0, 1)));
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        w32(MS, coin() ? 3u : u(ri(0, 5)));
        w8(0x00141347, u(ri(0, 6)));
        static const uint32_t fl[] = {1, 2, 0x20, 0x8000};
        if (ri(0, 3) == 0) w32(w + 0x30, ee_read<uint32_t>(w + 0x30) ^ fl[ri(0, 3)]);
        box(w);
        w32(OV(0x0015f3f8), R());
        return w;
    };
    check("menu_grid_update", 0x002794a0, 300, Ret::Int32, [grid] { return Args{{grid(ri(0, 5) != 0)}, {}}; },
          [](A a) { return ri32(menu_grid_update(a.i[0])); });
    check("menu_grid_focus_in", 0x002793f0, 60, Ret::Int32, [grid] { return Args{{grid(false), u(ri(0, 3) == 0 ? 1 : 0)}, {}}; },
          [](A a) { return ri32(menu_grid_focus_in(a.i[0], static_cast<int32_t>(a.i[1]))); });
    check("menu_grid_draw", 0x0027b590, 120, Ret::Int32, [grid] { return Args{{grid(coin())}, {}}; },
          [](A a) { return ri32(menu_grid_draw(a.i[0])); });
    // --- the 3D item panels (Weapons / Gadgets) -------------------------------------------------
    // the page with its grid focused on an item (owned or not), the panels' state
    auto panel = [page, progress, box](uint32_t pw) {
        const bool gadgets = coin();
        page(gadgets ? P_GADGETS : P_WEAPONS);
        progress();
        const uint32_t p = ee_read<uint32_t>(MS + 4);
        static const uint32_t gg[] = {OV(0x001b2b80), OV(0x001b2be8), OV(0x001b2c50), OV(0x001b2cb8)};
        const uint32_t g = gadgets ? gg[ri(0, 3)] : OV(0x001b2f58);
        w32(p + 0x40, g);
        w32(g + 0x3c, u(ri(0, ee_read<int32_t>(g + 0x40) * ee_read<int32_t>(g + 0x44) - 1)));
        for (uint32_t s = 0; s < 4; ++s) w32(MS + 0x30 + s * 4, u(ri(0, 3) == 0 ? 0 : ri(0, 0x30)));
        if (coin()) w8(ee_read<uint32_t>(ee_read<uint32_t>(g + 0x48) + ee_read<uint32_t>(g + 0x3c) * 10 + 6) + 0x0013d4c0, 1);
        const uint32_t w = pw;
        box(w);
        w32(0x0013d428 + u(ri(0, 0x40)) * 4, u(ri(0, 3000)));
        return w;
    };
    check("menu_model_update", 0x0027be78, 120, Ret::Int32, [panel] {
        const uint32_t w = panel(OV(0x001b4f28));
        w32(w + 0x34, u(ri(-1, 4)));
        if (coin()) w32(w + 0x30, ee_read<uint32_t>(w + 0x30) ^ 1);
        w32(0x00140408, coin() ? 0u : u(ri(0, 0x300)));
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_model_update(a.i[0])); });
    // a panel showing its models: the port's frames from the focus in
    auto shown = [panel](uint32_t pw) {
        const uint32_t w = panel(pw);
        menu_model_focus_in(w);
        for (int f = 0; f < 4; ++f) (pw == OV(0x001b3080) ? menu_ammo_update(w) : menu_model_update(w));
        return w;
    };
    check("menu_model_focus_in", 0x0027bb78, 5, Ret::Int32, [panel] { return Args{{panel(OV(0x001b4f28))}, {}}; },
          [](A a) { return ri32(menu_model_focus_in(a.i[0])); });
    check("menu_model_focus_out", 0x0027bba0, 20, Ret::Int32, [shown] { return Args{{shown(OV(0x001b4f28))}, {}}; },
          [](A a) { return ri32(menu_model_focus_out(a.i[0])); });
    check("menu_model_draw", 0x0027c250, 20, Ret::Int32, [shown] { return Args{{shown(OV(0x001b4f28))}, {}}; },
          [](A a) { return ri32(menu_model_draw(a.i[0])); });
    check("menu_item_moby_update", 0x0027c2e0, 40, Ret::None, [shown] {
        const uint32_t w = shown(OV(0x001b4f28));
        wf(w + 0x38, rf(-3.1f, 3.1f));
        return Args{{ee_read<uint32_t>(w + 0x44)}, {}};
    }, [](A a) { if (a.i[0] != 0) menu_item_moby_update(a.i[0]); return uint64_t{0}; });
    check("menu_ammo_update", 0x0027bbe0, 80, Ret::Int32, [shown] {
        const uint32_t w = shown(OV(0x001b3080));
        if (coin()) {  // another item focused
            const uint32_t g = ee_read<uint32_t>(ee_read<uint32_t>(MS + 4) + 0x40);
            w32(g + 0x3c, u(ri(0, ee_read<int32_t>(g + 0x40) * ee_read<int32_t>(g + 0x44) - 1)));
        }
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_ammo_update(a.i[0])); });
    check("menu_ammo_moby_update", 0x0027be40, 10, Ret::None, [shown] { return Args{{ee_read<uint32_t>(shown(OV(0x001b3080)) + 0x44)}, {}}; },
          [](A a) { menu_ammo_moby_update(a.i[0]); return uint64_t{0}; });
    check("menu_ammo_draw", 0x0027bd58, 40, Ret::Int32, [shown, panel] {
        const uint32_t w = coin() ? shown(OV(0x001b3080)) : panel(OV(0x001b3080));
        if (!coin()) w32(w + 0x44, 0);
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_ammo_draw(a.i[0])); });
    check("menu_ammo_count_draw", 0x0027cda0, 60, Ret::Int32, [panel] {
        const uint32_t w = panel(OV(0x001b3140));
        for (uint32_t i = 0; i < 0x40; ++i) w32(0x0013d428 + i * 4, u(ri(0, 3) == 0 ? ri(1000, 20000) : ri(0, 999)));
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_ammo_count_draw(a.i[0])); });
    check("menu_hero_draw", 0x0027ba40, 20, Ret::Int32, [panel] {
        const uint32_t w = panel(OV(0x001b2e98));
        for (uint32_t i = 0; i < 0x18; ++i) {
            w32(w + 0x44 + i * 4, coin() ? ee_read<uint32_t>(OV(0x001b9e90) + u(ri(0, 13)) * 4) : 0u);
            w8(w + 0xa4 + i, ri(0, 3) == 0 ? 1u : 0u);
            w32(OV(0x001b9120) + i * 4, u(ri(0, 1)));
        }
        w32(w + 0x60, ee_read<uint32_t>(w + 0x44 + u(ri(0, 3)) * 4));
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_hero_draw(a.i[0])); });
    // --- Quick Select: the ring (slot 1 of its page) -------------------------------------------
    auto ring = [page, progress, buttons, box] {
        page(P_QUICK);
        progress();
        buttons();
        const uint32_t p = ee_read<uint32_t>(MS + 4);
        const uint32_t g = coin() ? OV(0x001b3398u) : OV(0x001b3400u);
        w32(p + 0x40, g);
        w32(g + 0x3c, u(ri(0, ee_read<int32_t>(g + 0x40) * ee_read<int32_t>(g + 0x44) - 1)));
        const uint32_t w = OV(0x001b3468);
        for (uint32_t i = 0; i < 8; ++i) {
            w32(w + 0x30 + i * 4, u(ri(0, 2) == 0 ? 0 : ri(1, 0x30)));
            w32(0x00141ea0 + i * 4, u(ri(0, 2) == 0 ? 0 : ri(1, 0x30)));
        }
        w32(w + 0x50, u(ri(0, 7)));
        w16(0x001418f0, coin() ? 0xffffu : u(ri(0, 100)));
        w32(0x0015eea4, u(ri(0, 1000000)));
        w32(0x0015ed84, u(ri(0, 18)));
        box(w);
        w32(OV(0x0015f3f8), R());
        return w;
    };
    check("menu_quick_update", 0x0027a3e8, 150, Ret::Int32, [ring] { return Args{{ring()}, {}}; },
          [](A a) { return ri32(menu_quick_update(a.i[0])); });
    check("menu_quick_focus_in", 0x0027a610, 30, Ret::Int32, [ring] { return Args{{ring()}, {}}; },
          [](A a) { return ri32(menu_quick_focus_in(a.i[0])); });
    check("menu_quick_focus_out", 0x0027a6a8, 10, Ret::Int32, [ring] { return Args{{ring()}, {}}; },
          [](A a) { return ri32(menu_quick_focus_out(a.i[0])); });
    check("menu_quick_draw", 0x0027e768, 40, Ret::Int32, [ring] { return Args{{ring()}, {}}; },
          [](A a) { return ri32(menu_quick_draw(a.i[0])); });
    // --- Items: the statistics panel (slot 2 of its page) -------------------------------------
    auto stats = [page, progress, box] {
        page(P_ITEMS);
        progress();
        const uint32_t w = OV(0x001b5ec8);
        box(w);
        for (uint32_t i = 0; i < 0x50; ++i) w8(0x0014bec0 + i, ri(0, 2) == 0 ? 1u : 0u);
        w32(0x0015ed88, u(coin() ? 3 : ri(0, 5)));
        w32(0x0013e504, u(ri(0, 3000)));
        return w;
    };
    check("bytes_14bec0_count", 0x00264588, 20, Ret::Int32, [stats] { stats(); return Args{}; }, [](A) { return ri32(bytes_14bec0_count()); });
    check("bytes_14bec0_left", 0x00264540, 20, Ret::Int32, [stats] { stats(); return Args{}; }, [](A) { return ri32(bytes_14bec0_left()); });
    check("menu_stats_draw", 0x0027c768, 40, Ret::Int32, [stats] { return Args{{stats()}, {}}; },
          [](A a) { return ri32(menu_stats_draw(a.i[0])); });
    check("menu_stats_focus_in", 0x0027c690, 10, Ret::Int32, [stats] {
        const uint32_t w = stats();
        menu_stats_focus_out(w);
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_stats_focus_in(a.i[0])); });
    check("menu_stats_focus_out", 0x0027c738, 10, Ret::Int32, [stats] { return Args{{stats()}, {}}; },
          [](A a) { return ri32(menu_stats_focus_out(a.i[0])); });
    check("menu_stats_moby_update", 0x0027cd68, 10, Ret::None, [stats] {
        const uint32_t m = ee_read<uint32_t>(stats() + 0x44);
        if (m != 0) wf(m + 0x40, rf(-3.1f, 3.1f));
        return Args{{m}, {}};
    }, [](A a) { menu_stats_moby_update(a.i[0]); return uint64_t{0}; });
    // --- the planet map (state 0xb) ---------------------------------------------------------------
    static const uint32_t MAP = OV(0x001841f0), FREE = 0x01c00000;  // (FREE: unused RAM for test buffers)
    static std::vector<uint8_t> snap_map;
    auto map = [level, packet, frames] {  // the map set up (the port's 0x2464a8) and its page open
        uint8_t* ram = ee_get_ram_base();
        if (snap_map.empty()) {
            level();
            planet_map_init();
            w32(OV(0x0016c06c), 0);
            menu_open(0xa);
            frames(16);
            snap_map.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap_map.data(), snap_map.size());
        }
        packet();
    };
    check("map_bits_widen", 0x0024a1f0, 10, Ret::None, [map] {
        map();
        wrand(FREE, 0x200);
        return Args{{FREE + 0x1000, FREE}, {}};
    }, [](A a) { map_bits_widen(a.i[0], a.i[1]); return uint64_t{0}; });
    auto bits = [map] {  // run-length data for 0x40000 pixels
        map();
        uint32_t runs = FREE, spans = FREE + 0x8000;
        w8(runs++, u(ri(0, 255)));
        for (int i = 0; i < 0x7000; ++i) w8(runs++, u(ri(0, 6) == 0 ? 0 : ri(1, 255)));
        for (int i = 0; i < 0x3800; ++i) { w8(spans++, u(ri(0, 12))); w8(spans++, u(ri(0, 255))); }
        return Args{{FREE + 0x20000, FREE, FREE + 0x8000}, {}};
    };
    check("map_bits_unpack", 0x00249fc0, 4, Ret::None, bits, [](A a) { map_bits_unpack(a.i[0], a.i[1], a.i[2]); return uint64_t{0}; });
    check("map_mask_unpack", 0x00249f48, 6, Ret::None, [bits] {
        Args a = bits();
        w8(FREE, coin() ? 1u : 0u);
        w32(MAP + 0x28, u(ri(0, 3) != 0 ? 1 : 0));
        return a;
    }, [](A a) { map_mask_unpack(a.i[0], a.i[1], a.i[2]); return uint64_t{0}; });
    check("map_mask_from_tiles", 0x00248278, 3, Ret::None, [map] {
        map();
        return Args{{FREE + 0x20000, ee_read<uint32_t>(MAP), ee_read<uint32_t>(MAP + 4)}, {}};
    }, [](A a) { map_mask_from_tiles(a.i[0], a.i[1], a.i[2]); return uint64_t{0}; });
    check("image_upload_at", 0x00245b60, 20, Ret::Int64, [map] {
        map();
        w32(OV(0x0015f418), u(ri(0, 3) == 0 ? 0x40 : ri(0, 0x3f)));
        return Args{{u(ri(4, 9)), u(ri(4, 9)), R() & 0x1fffff0u, R() & 0x1fffff0u, R() & 0x3fff00u, R() & 0x3fff00u}, {}};
    }, [](A a) { return image_upload_at(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), a.i[2], a.i[3], static_cast<int32_t>(a.i[4]), static_cast<int32_t>(a.i[5])); });
    check("map_bytes_select", 0x00246f40, 3, Ret::None, [map] {
        map();
        wrand(FREE, 0x2000);
        wrand(FREE + 0x40000, 0x2000);
        return Args{{FREE + 0x80000, FREE, FREE + 0x40000, FREE + 0x20000}, {}};
    }, [](A a) { map_bytes_select(a.i[0], a.i[1], a.i[2], a.i[3]); return uint64_t{0}; });
    // the planet buffers: random contents, planets, sizes
    auto bufs5 = [map] {
        map();
        for (uint32_t k = 0; k < 5; ++k) {
            w32(MAP + 0x278 + k * 4, ri(0, 4) == 0 ? 0u : FREE + k * 0x1000);
            const int32_t p = ri(0, 3) == 0 ? -1 : ri(0, 0x12) + (coin() ? 0x100 : 0) + (ri(0, 3) == 0 ? 0x1000 : 0);
            w32(MAP + 0x28c + k * 4, u(p));
            w32(MAP + 0x2a4 + k * 4, u(ri(1, 0x80)));
        }
        w32(MAP + 0x2a0, u(ri(-1, 4)));
        w32(MAP + 0x224, u(ri(0, 18)));
        w8(0x0013d4e1, u(ri(0, 1)));
    };
    check("map_buffer_free", 0x00246a60, 30, Ret::Int32, [bufs5] { bufs5(); return Args{{u(ri(0, 1))}, {}}; },
          [](A a) { return ri32(map_buffer_free(static_cast<int32_t>(a.i[0]))); });
    check("map_buffer_move", 0x00246b68, 20, Ret::None, [bufs5] {
        bufs5();
        for (uint32_t k = 0; k < 5; ++k) w32(MAP + 0x278 + k * 4, FREE + k * 0x1000);
        return Args{{u(ri(0, 4)), u(ri(0, 4))}, {}};
    }, [](A a) { map_buffer_move(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1])); return uint64_t{0}; });
    check("map_buffer_for_read", 0x00246ac8, 30, Ret::Int32, [bufs5] {
        bufs5();
        for (uint32_t k = 0; k < 5; ++k) if (ee_read<uint32_t>(MAP + 0x278 + k * 4) == 0) w32(MAP + 0x2a4 + k * 4, 0);
        return Args{};
    }, [](A) { return ri32(map_buffer_for_read()); });
    check("map_buffer_of", 0x00246c08, 30, Ret::Int32, [bufs5] { bufs5(); return Args{{u(ri(0, 0x12) + (coin() ? 0x100 : 0))}, {}}; },
          [](A a) { return ri32(map_buffer_of(static_cast<int32_t>(a.i[0]))); });
    check("map_planet_index", 0x00246d88, 30, Ret::Int32, [map] { map(); return Args{{u(ri(-1, 20))}, {}}; },
          [](A a) { return ri32(map_planet_index(static_cast<int32_t>(a.i[0]))); });
    check("map_planet_to_read", 0x00246c50, 40, Ret::Int32, [bufs5] {
        bufs5();
        // (the shown planet is one of the list's)
        w32(MAP + 0x224, ee_read<uint32_t>(ee_read<uint32_t>(OV(0x001602a0)) + u(ri(0, 5)) * 4));
        return Args{};
    }, [](A) { return ri32(map_planet_to_read()); });
    check("map_buffer_to_reuse", 0x00246de0, 40, Ret::Int32, [bufs5] {
        bufs5();
        if (coin()) w32(MAP + 0x224, ee_read<uint32_t>(ee_read<uint32_t>(OV(0x001602a0)) + u(ri(0, 5)) * 4));
        if (ri(0, 4) == 0) w32(MAP + 0x224, 0);
        return Args{};
    }, [](A) { return ri32(map_buffer_to_reuse()); });
    check("map_zoom_scroll", 0x00246fa8, 60, Ret::Int32, [map] {
        map();
        w32(PAD + 0x1a4, ri(0, 4) == 0 ? 0x100u : 0u);
        w32(MAP + 0x24, u(ri(0, 4) != 0 ? 1 : 0));
        w32(MAP + 0x228, u(ri(-1, 18)));
        wf(PAD + 0x104, rf(-1, 1));
        wf(PAD + 0x108, rf(-1, 1));
        wf(PAD + 0x10c, rf(-1, 1));
        for (uint32_t i = 0; i < 0x14; ++i) {
            wf(MAP + 0xb4 + i * 4, rf(0.5f, 4.5f));
            w32(MAP + 0x104 + i * 4, R() & 0x1fffffffu);
            w32(MAP + 0x154 + i * 4, R() & 0x1fffffffu);
        }
        return Args{};
    }, [](A) { return ri32(map_zoom_scroll()); });
    check("disc_read_sync", 0x002660c0, 10, Ret::Int32, [map] {
        map();
        w16(0x001516d8, 0);
        return Args{{u(ri(0, 1))}, {}};
    }, [](A a) { return ri32(disc_read_sync(static_cast<int32_t>(a.i[0]))); });
    check("map_marker_text", 0x0024a618, 30, Ret::None, [map] {
        map();
        const uint32_t r = ee_read<uint32_t>(MAP + 0x20);
        int32_t n = 0;
        if (r != 0) while ((ee_read<uint16_t>(r + static_cast<uint32_t>(n) * 0x28 + 4) & 4) == 0) ++n;
        return Args{{u(ri(0, n > 0 ? n - 1 : 0)), FREE}, {}};
    }, [](A a) { map_marker_text(static_cast<int32_t>(a.i[0]), a.i[1]); return uint64_t{0}; });
    check("map_markers_setup", 0x0024e248, 40, Ret::None, [map] {
        map();
        for (uint32_t i = 0; i < 0x13; ++i) w8(0x0013dd58 + i, ri(0, 2) != 0 ? 1u : 0u);
        wf(0x0013f3d8, coin() ? 0.0f : 1.0f);
        w32(OV(0x0015fdf8), u(ri(0, 1)));
        return Args{{u(coin() ? 0 : ri(0, 0x14)), u(ri(0, 1))}, {}};
    }, [](A a) { map_markers_setup(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1])); return uint64_t{0}; });
    check("planet_map_init (no map)", 0x002464a8, 10, Ret::None, [map] {
        map();
        w8(0x0013d4e1, u(ri(0, 1)));
        w32(0x0015ed84, u(ri(0, 20)));
        const int32_t lv = ee_read<int32_t>(0x0015ed84);
        const uint32_t t = ee_read<uint8_t>(0x0013d4e1) != 0 ? 0x00138438u : 0x001383a0u;
        w32(t + u(0x12 < lv ? 0 : lv) * 8 + 4, 0);  // (no map: the disc read and the stash left out)
        return Args{};
    }, [](A) { planet_map_init(); return uint64_t{0}; });
    check("planet_map_page_start", 0x002468b8, 10, Ret::None, [map] {
        map();
        w32(MAP + 0x230, 1);  // (the map-o-matic's disc read left out)
        if (ri(0, 3) == 0) w32(MAP + 0x22c, 0xffffffffu);
        return Args{};
    }, [](A) { planet_map_page_start(); return uint64_t{0}; });
    auto mapw = [map, buttons] {
        map();
        buttons();
        const uint32_t w = OV(0x001b3650);
        if (ri(0, 4) == 0) w32(w + 0x34, ee_read<uint32_t>(w + 0x34) ^ (coin() ? 0x40u : 0x80u));
        w16(0x001516d8, u(ri(0, 3) == 0 ? 1 : 0));
        if (ee_read<int16_t>(0x001516d8) == 0) w32(0x00137b00, 0);
        w32(MAP + 0x2a0, u(ri(0, 2) == 0 ? ri(0, 4) : -1));
        w8(MS + 0xcb, u(ri(0, 1)));
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        if (coin()) w32(MAP + 0x228, 0xffffffffu);
        return w;
    };
    check("planet_map_update", 0x00279aa8, 60, Ret::Int32, [mapw] { return Args{{mapw()}, {}}; },
          [](A a) { return ri32(planet_map_update(a.i[0])); });
    check("planet_map_leave", 0x002799e8, 30, Ret::Int32, [mapw] { return Args{{mapw()}, {}}; },
          [](A a) { return ri32(planet_map_leave(a.i[0])); });
    check("planet_map_enter", 0x0027a710, 5, Ret::Int32, [mapw] { return Args{{mapw()}, {}}; },
          [](A a) { return ri32(planet_map_enter(a.i[0])); });
    // the map's drawing: the planet shown, its zoom / scroll, its markers
    auto mapd = [map] {
        map();
        static const int32_t pl[] = {0, 0, 0, 1, 2, 6, 0xd, 0x11, 5};
        const int32_t p = pl[ri(0, 8)];
        w32(MAP + 0x228, u(p));
        w32(MAP + 0x24, ri(0, 6) == 0 ? 0u : 1u);
        wf(MAP + 0xb4 + u(p) * 4, rf(0.65f, 4.0f));
        w32(MAP + 0x104 + u(p) * 4, R() & 0x0fffffffu);
        w32(MAP + 0x154 + u(p) * 4, R() & 0x0fffffffu);
        w8(0x0015edb4, u(ri(0, 3) == 0 ? 1 : 0));
        w32(OV(0x0015fdf8), u(ri(0, 1)));
        w32(MAP + 0x2c, u(ri(0, 1)));
        for (uint32_t k = 0; k < 8; ++k) w32(MAP + 0x30 + k * 0x10, u(ri(-1, 255)));
        w32(MAP + 0x1c, coin() ? 0u : FREE);
        w32(MAP + 0xb0, u(ri(0, 6)));
        for (uint32_t k = 0; k < 6; ++k) {
            wf(FREE + k * 16, rf(100, 400));
            wf(FREE + k * 16 + 4, rf(100, 400));
            w32(FREE + k * 16 + 8, coin() ? 0xffffffffu : 0xe99au);
            w32(FREE + k * 16 + 0xc, u(ri(0, 5)));
        }
        if (coin()) {  // the planet's markers (its map known)
            w8(0x0013dd58 + u(p), 1);
            map_markers_setup(p, 0);
        }
        const uint32_t r = ee_read<uint32_t>(MAP + 0x20);
        if (r != 0) {
            for (uint32_t o = 0; (ee_read<uint16_t>(r + o + 4) & 4) == 0; o += 0x28) {
                static const uint32_t fl[] = {0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x80, 1, 2};  // (0x10, the text box, only where set up)
                if (coin()) w16(r + o + 4, ee_read<uint16_t>(r + o + 4) ^ fl[ri(0, 9)]);
                w32(r + o + 0x24, u(ri(0, 3) != 0 ? 1 : 0));
                wf(r + o + 0x18, rf(0, 1));
                wf(r + o + 0x1c, rf(0, 1));
                wf(r + o + 0x20, rf(-3, 3));
            }
        }
        w32(OV(0x0015f3f8), R());
        w32(OV(0x0015efc4), u(ri(0, 10)));
        w32(OV(0x0015efc0), FREE + 0x10000);
    };
    check("planet_map_draw", 0x002471a8, 80, Ret::None, [mapd] { mapd(); return Args{}; }, [](A) { planet_map_draw(); return uint64_t{0}; });
    check("planet_map_widget_draw", 0x0027cf78, 10, Ret::Int32, [mapd] { mapd(); return Args{{OV(0x001b3650)}, {}}; },
          [](A a) { return ri32(planet_map_widget_draw(a.i[0])); });
    check("planet_picture_draw", 0x0027e498, 40, Ret::Int32, [mapd] {
        mapd();
        if (ri(0, 4) == 0) w32(MAP + 0x228, 0xffffffffu);
        if (ri(0, 2) == 0) w32(MAP + 0x228, u(ri(0, 18)));
        return Args{{OV(0x001b36b8)}, {}};
    }, [](A a) { return ri32(planet_picture_draw(a.i[0])); });
    check("planet_map_legend_draw", 0x0027cfb0, 20, Ret::Int32, [mapd, box] {
        mapd();
        box(OV(0x001b3720));
        w32(0x0015ed84, u(ri(0, 3) == 0 ? 0 : ri(1, 18)));
        return Args{{OV(0x001b3720)}, {}};
    }, [](A a) { return ri32(planet_map_legend_draw(a.i[0])); });
    check("map_slots_draw", 0x0024a8e8, 30, Ret::None, [mapd] {
        mapd();
        w32(MAP + 0x1c, FREE);
        return Args{{u(ri(-500, 2000)), u(ri(-500, 2000)), u(ri(3000, 9000)), u(ri(3000, 9000))}, {}};
    }, [](A a) { map_slots_draw(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3])); return uint64_t{0}; });
    check("debug_text_centred", 0x001f39e0, 10, Ret::Int32, [mapd] {
        mapd();
        return Args{{u(ri(0, 500)), u(ri(0, 400)), R(), OV(0x001e8b18)}, {}};
    }, [](A a) { return ri32(debug_text_centred(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), a.i[2], a.i[3])); });
    // --- the missions page (0x1b3778, cross on the map) -------------------------------------------
    static std::vector<uint8_t> snap_mis;
    auto mis = [map, packet, frames, progress] {
        uint8_t* ram = ee_get_ram_base();
        if (snap_mis.empty()) {
            map();
            w32(MS + 8, OV(0x001b3778));
            frames(16);
            snap_mis.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap_mis.data(), snap_mis.size());
        }
        packet();
        progress();
        // the goals' states and steps
        const uint32_t g = ee_read<uint32_t>(OV(0x00186d20));
        if (g != 0)
            for (uint32_t r = g; ee_read<int16_t>(r) != 0; r += 0x28) {
                w16(r + 0x24, u(ri(0, 2)));
                if (ri(0, 3) == 0) w16(r + 0x10, ee_read<uint16_t>(r + 0x10) ^ (coin() ? 1u : 2u));
            }
        w32(MAP + 0x224, u(ri(0, 3) == 0 ? ri(0, 19) : 0));
        for (uint32_t i = 0; i < 0x14; ++i) w8(0x0013dd40 + i, ri(0, 2) == 0 ? 1u : 0u);
        return OV(0x001b38c0u);
    };
    check("missions_list", 0x0024deb8, 40, Ret::Int32, [mis] {
        mis();
        return Args{{SPR_ADDR, coin() ? FREE : 0u, coin() ? SPR_ADDR + 0x100 : 0u, u(ri(0, 1))}, {}};
    }, [](A a) { return ri32(missions_list(a.i[0], a.i[1], a.i[2], static_cast<int32_t>(a.i[3]))); });
    check("missions_focus_in", 0x0027a068, 20, Ret::Int32, [mis] {
        const uint32_t w = mis();
        for (uint32_t i = 0; i < 0x14; ++i) w32(w + 0x30 + i * 4, u(ri(-1, 3)));
        return Args{{w}, {}};
    }, [](A a) { return ri32(missions_focus_in(a.i[0])); });
    check("missions_update", 0x0027a108, 120, Ret::Int32, [mis, buttons] {
        const uint32_t w = mis();
        buttons();
        if (ri(0, 4) == 0) w32(ee_read<uint32_t>(MS + 4) + 0x40, OV(0x001b3800));
        missions_focus_in(w);
        for (uint32_t i = 0; i < 0x14; ++i) w32(w + 0x30 + i * 4, u(ri(0, 2)));
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        return Args{{w}, {}};
    }, [](A a) { return ri32(missions_update(a.i[0])); });
    check("missions_draw", 0x0027d2d0, 40, Ret::Int32, [mis, box] {
        const uint32_t w = mis();
        box(w);
        missions_focus_in(w);
        w32(w + 0x30 + ee_read<uint32_t>(MAP + 0x224) * 4, u(ri(0, 3)));
        return Args{{w}, {}};
    }, [](A a) { return ri32(missions_draw(a.i[0])); });
    check("text_wrapped_med", 0x001fbe88, 40, Ret::Int32, [mis] {
        mis();
        static const uint32_t ids[] = {0x4f59, 0x523d, 0x4f1e, 0x4ef0, 0x4fd5};
        w32(OV(0x0015f45c), u(ri(0, 1)));
        return Args{{u(ri(0, 50)), u(ri(0, 100)), u(ri(40, 400)), u(ri(16, 300)), R(), sub_00203ad8(static_cast<int32_t>(ids[ri(0, 4)])), u(ri(0, 3) == 0 ? ri(0, 20) : -1)}, {}};
    }, [](A a) { return ri32(text_wrapped_med(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<uint64_t>(sx(a.i[4])), a.i[5], static_cast<int32_t>(a.i[6]))); });
    check("mission_box", 0x0027d530, 10, Ret::None, [mis] { mis(); return Args{{u(ri(0, 300)), u(ri(0, 300)), u(ri(0, 1))}, {}}; },
          [](A a) { mission_box(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2])); return uint64_t{0}; });
    auto mpic = [mis] {
        mis();
        const uint32_t w = OV(0x001b3800);
        w32(w + 0x44, u(ri(-1, 4)));
        w32(w + 0x50, u(ri(-1, 6)));
        w32(w + 0x58, u(ri(-1, 6)));
        w16(0x001516d8, u(ri(0, 3) == 0 ? 1 : 0));
        if (ee_read<int16_t>(0x001516d8) == 0) w32(0x00137b00, 0);
        const uint32_t b = ee_read<uint32_t>(w + 0x48);
        if (b != 0) {  // an image (unpacked: an empty WAD where it is read)
            const uint32_t src = b + ee_read<uint32_t>(w + 0x60);
            for (uint32_t o = 0; o < 0x40; o += 4) w32(src + o, 0);
            w32(src, 0x10444157u);
            w32(b + 8, 1u << ri(5, 8));
            w32(b + 0xc, 1u << ri(5, 8));
        }
        w32(OV(0x0015f418), u(ri(0, 0x3f)));
        return w;
    };
    check("mission_picture_update", 0x0027d5d8, 80, Ret::Int32, [mpic] { return Args{{mpic()}, {}}; },
          [](A a) { return ri32(mission_picture_update(a.i[0])); });
    check("mission_picture_draw", 0x0027d8b0, 20, Ret::Int32, [mpic] { return Args{{mpic()}, {}}; },
          [](A a) { return ri32(mission_picture_draw(a.i[0])); });
    // --- the Help page's sub-pages ------------------------------------------------------------------
    auto help = [page, progress, buttons, box](uint32_t p, uint32_t w) {
        page(p);
        progress();
        buttons();
        box(w);
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        w16(0x001516d8, u(ri(0, 3) == 0 ? 1 : 0));
        if (ee_read<int16_t>(0x001516d8) == 0) w32(0x00137b00, 0);
        w32(0x0015ed88, u(ri(0, 5)));
        w32(OV(0x0015f3f8), R());
        return w;
    };
    check("help_log_list_focus_in", 0x0027adb0, 20, Ret::Int32, [help] {
        const uint32_t w = help(OV(0x001b3c60), OV(0x001b4458));
        w32(0x0015ee30, u(ri(0, 12)));
        for (uint32_t i = 0; i < 16; ++i) w8(0x00141e08 + i, u(ri(0, 40)));
        return Args{{w}, {}};
    }, [](A a) { return ri32(help_log_list_focus_in(a.i[0])); });
    check("text_list_draw", 0x0027b0f0, 80, Ret::Int32, [help] {
        static const uint32_t ws[][2] = {{OV(0x001b3c60), OV(0x001b4458)}, {OV(0x001b6d40), OV(0x001b71a0)}, {OV(0x001b7dd0), OV(0x001b8090)}};
        const int k = ri(0, 2);
        const uint32_t w = help(ws[k][0], ws[k][1]);
        if (k == 0) { w32(0x0015ee30, u(ri(0, 12))); help_log_list_focus_in(w); }
        int32_t n = 0;
        for (uint32_t e = ee_read<uint32_t>(w + 0x34); ee_read<int16_t>(e) != 0; e += 0xc) ++n;
        w32(w + 0x40, u(ri(0, n > 0 ? n - 1 : 0)));
        w32(w + 0x44, u(ri(-40, 40)));
        if (coin()) w32(ee_read<uint32_t>(MS + 4) + 0x40, w);
        static const uint32_t fl[] = {2, 0x10, 0x200, 0x400, 0x800, 0x8000, 0x10000};
        if (ri(0, 2) == 0) w32(w + 0x30, ee_read<uint32_t>(w + 0x30) ^ fl[ri(0, 6)]);
        return Args{{w}, {}};
    }, [](A a) { return ri32(text_list_draw(a.i[0])); });
    auto hlog = [help] {
        const uint32_t w = help(OV(0x001b3c60), OV(0x001b44a8));
        w32(w + 0x50, u(ri(-1, 3)));
        for (uint32_t i = 0; i < 5; ++i) if (coin()) w32(BUFS + i * 8 + 4, u(ri(0, 7)));
        return w;
    };
    check("help_log_text_focus_in", 0x0027ae40, 20, Ret::Int32, [hlog] { return Args{{hlog()}, {}}; },
          [](A a) { return ri32(help_log_text_focus_in(a.i[0])); });
    check("help_log_text_focus_out", 0x0027af10, 20, Ret::Int32, [hlog] {
        const uint32_t w = hlog();
        w32(w + 0x54, coin() ? 0u : R() & 0x1fffff0u);
        w32(w + 0x38, u(ri(0, 300)));
        return Args{{w}, {}};
    }, [](A a) { return ri32(help_log_text_focus_out(a.i[0])); });
    check("help_log_text_update", 0x0027af80, 40, Ret::Int32, [hlog] {
        const uint32_t w = hlog();
        // the help texts' file in the buffer: a section per language (count, size, its offsets)
        const uint32_t b = ee_read<uint32_t>(MS + 0x108);
        for (uint32_t l = 0; l < 6; ++l) w32(b + l * 4, 0x20 + l * 0x100);
        for (uint32_t l = 0; l < 6; ++l) {
            const uint32_t s = b + 0x20 + l * 0x100;
            w32(s, u(ri(0, 8)));
            w32(s + 4, u(ri(8, 0xf0)));
            wrand(s + 8, 0x3c);
        }
        return Args{{w}, {}};
    }, [](A a) { return ri32(help_log_text_update(a.i[0])); });
    // --- the Weapons / Gadgets hero panel 0x1b2e98: Ratchet, Clank, what is equipped -------------
    static constexpr OvConst<0x001b2e98u> HW{};
    auto equip = [] {  // equipment slots, the class loaded, the orbs, the weapon held, the requests
        for (uint32_t s = 0; s < 4; ++s) w32(MS + 0x30 + s * 4, u(ri(0, 3) == 0 ? 0 : ri(1, 0x24)));
        // the item in hand has a class (a classless one with its moby shown would load class -1)
        while (ee_read<int32_t>(OV(0x00179ac0) + ee_read<uint32_t>(MS + 0x30) * 0x4c + 0x10) == -1) w32(MS + 0x30, u(ri(1, 0x24)));
        const int32_t cls = ee_read<int32_t>(OV(0x00179ac0) + ee_read<uint32_t>(MS + 0x30) * 0x4c + 0x10);
        bool in_level = false;  // its data in the level's item table 0x1afbc0 (else it cannot be loaded)
        for (int32_t k = 0; k < ee_read<int32_t>(OV(0x00160008)); ++k)
            if (ee_read<int32_t>(OV(0x001afbc0) + u(k) * 4) == cls &&
                (ee_read<uint32_t>(ee_read<uint32_t>(OV(0x001afc20) + u(k) * 4)) & 0xffffffu) == 0x444157u)
                in_level = true;
        w32(MS + 0x120, !in_level || ri(0, 3) == 0 ? 0xffffffffu : u(cls));
        w8(0x00141347, u(ri(0, 8)));
        w8(0x00141346, u(ri(0, 8)));
        w32(0x00140408, coin() ? 0u : u(ri(1, 0x24)));
        w32(MS + 0x1c, coin() ? 0xffffffffu : ee_read<uint32_t>(MS + 0x30));
    };
    auto hero_in = [panel, equip] {
        const uint32_t w = panel(HW);
        // the extra sequences in the stash, as the level start leaves them (no disc read under way then)
        const uint16_t busy = ee_read<uint16_t>(0x001516d8);
        w16(0x001516d8, 0);
        menu_sequences_stash();
        w16(0x001516d8, busy);
        bool shown = false;  // already focused on the snapshot: left first, as the menu does
        for (uint32_t k = 0; k < 0x18; ++k) shown = shown || ee_read<uint32_t>(w + 0x44 + k * 4) != 0;
        if (shown) menu_hero_focus_out(w);
        equip();
        menu_hero_focus_in(w);
        return w;
    };
    auto hero_up = [hero_in, equip] {
        const uint32_t w = hero_in();
        const int n = ri(0, 3);
        for (int f = 0; f < n; ++f) {
            if (ri(0, 2) == 0) equip();
            menu_hero_update(w);
        }
        return w;
    };
    check("menu_hero_focus_in", 0x00281d10, 30, Ret::Int32, [panel, equip] {
        const uint32_t w = panel(HW);
        equip();
        w32(OV(0x0015f5c4), coin() ? 3u : (coin() ? 2u : 6u));
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_hero_focus_in(a.i[0])); });
    check("menu_hero_focus_out", 0x00281f00, 30, Ret::Int32, [hero_up] { return Args{{hero_up()}, {}}; },
          [](A a) { return ri32(menu_hero_focus_out(a.i[0])); });
    check("menu_hero_update", 0x00281fb0, 150, Ret::Int32, [hero_up, equip] {
        const uint32_t w = hero_up();
        equip();
        if (coin()) for (uint32_t i = 0; i < 0x18; ++i) w8(w + 0xa4 + i, u(ri(0, 3)));
        return Args{{w}, {}};
    }, [](A a) { return ri32(menu_hero_update(a.i[0])); });
    // a moby of the panel (one of the offsets), its animation maybe ended
    auto hero_moby = [hero_up](std::initializer_list<uint32_t> offs) {
        uint32_t m = 0;
        for (int t = 0; t < 40 && m == 0; ++t) {
            const uint32_t w = hero_up();
            const uint32_t off = offs.begin()[ri(0, static_cast<int>(offs.size()) - 1)];
            m = ee_read<uint32_t>(w + off);
        }
        if (m != 0) {
            if (coin()) w8(m + 0x70, ee_read<uint8_t>(m + 0x70) | 2u);
            if (ee_read<int16_t>(m + 0xa6) == 0x25f) {  // Clank's set 6 and its counter
                w8(m + 0x20, u(ri(0, 3)));
                if (coin()) w8(m + 0x52, 6);
            }
        }
        return m;
    };
    auto moby_check = [&](const char* name, uint32_t addr, std::initializer_list<uint32_t> offs, void (*fn)(uint32_t)) {
        check(name, addr, 40, Ret::None, [hero_moby, offs] {
            return Args{{hero_moby(offs)}, {}};
        }, [fn](A a) { fn(a.i[0]); return uint64_t{0}; });
    };
    moby_check("menu_weapon_moby_update", 0x002827b8, {0x54}, menu_weapon_moby_update);
    moby_check("menu_item2_moby_update", 0x00282970, {0x50}, menu_item2_moby_update);
    moby_check("menu_item1_moby_update", 0x00282a60, {0x58, 0x5c}, menu_item1_moby_update);
    moby_check("menu_item3_moby_update", 0x00282c08, {0x48, 0x4c}, menu_item3_moby_update);
    moby_check("menu_gadget_moby_update", 0x00282d38, {0x6c, 0x70, 0x74}, menu_gadget_moby_update);
    moby_check("menu_orb_moby_update", 0x00282dc8, {0x78, 0x7c, 0x80, 0x84, 0x88, 0x8c, 0x90, 0x94}, menu_orb_moby_update);
    check("hero_anim_update", 0x00283cb8, 120, Ret::Int32, [hero_up] {
        const uint32_t w = hero_up();
        const uint32_t hero = ee_read<uint32_t>(w + 0x44);
        if (hero != 0 && coin()) w8(hero + 0x70, ee_read<uint8_t>(hero + 0x70) | 2u);
        w32(OV(0x001ba180) + 0xc, u(ri(0, 2)));
        // the disc busy or not; a read under way stays under way (its sectors never arrive here)
        if (ee_read<uint8_t>(MS + 0xcb) == 0) w16(0x001516d8, ri(0, 3) == 0 ? 1u : 0u);
        else {  // a finished read must have left its sequence (else the original's unpack halts)
            const uint32_t buf = ee_read<uint32_t>(MS + 0xa0 + (ee_read<uint8_t>(MS + 0xcb) - 1u) * 4);
            if ((ee_read<uint32_t>(buf + ee_read<uint32_t>(MS + 0xbc)) & 0xffffffu) != 0x444157u) w16(0x001516d8, 1);
        }
        w32(0x00137b00, 0);  // the disc free (a refused read halts the original)
        if (ee_read<int32_t>(OV(0x00160410)) > 0) {
            w32(OV(0x001b9fc0), u(ri(0, 3)));
            // a request to read whose sequence has no sectors would halt the original (teq)
            const uint32_t seq = ee_read<uint32_t>(OV(0x001b9fc0) + 4), base = ee_read<uint32_t>(MS + 0xcc);
            if (ee_read<uint32_t>(OV(0x001b9fc0)) == 0 && seq >= base && ee_read<uint32_t>(0x00137b80 + (seq - base) * 8 + 0x1c) == 0)
                w32(OV(0x001b9fc0), 2);
            w32(OV(0x001b9fc0) + 8, u(ri(0, 3)));
            if (coin()) w32(OV(0x001b9fc0) + 0x10, ee_read<uint32_t>(MS + 0x30 + u(ri(0, 3)) * 4));
        }
        return Args{{hero, ee_read<uint32_t>(w + 0x54), ee_read<uint32_t>(w + 0x50), w + 0x60, w + 0x64, w + 0x68}, {}};
    }, [](A a) { return ri32(hero_anim_update(a.i[0], a.i[1], a.i[2], a.i[3], a.i[4], a.i[5])); });
    check("hero_anim_dequeue", 0x002844b8, 10, Ret::Int32, [hero_up] { hero_up(); return Args{}; },
          [](A) { return ri32(hero_anim_dequeue()); });
    check("hero_anim_clear", 0x00284560, 10, Ret::Int32, [hero_up] {
        hero_up();
        w16(0x001516d8, u(ri(0, 1)));
        return Args{};
    }, [](A) { return ri32(hero_anim_clear()); });
    check("hero_anim_extra_free", 0x00284600, 20, Ret::Int32, [hero_up] { hero_up(); return Args{}; },
          [](A) { return ri32(hero_anim_extra_free()); });
    // 0x276078 (menu_sequences_stash) is not checked: its disc reads 0x266040 are served by the
    // host in the port and do nothing under the interpreter.
    check("hero_model_setup", 0x00234b38, 30, Ret::None, [hero_in] {
        const uint32_t w = hero_in();
        w32(OV(0x0015f5c4), coin() ? 3u : (coin() ? 2u : 6u));
        w32(0x0015ed84, coin() ? 0u : u(ri(0, 13)));
        return Args{{ee_read<uint32_t>(w + 0x44)}, {}};
    }, [](A a) { hero_model_setup(a.i[0]); return uint64_t{0}; });
    auto twop = [help] {
        const uint32_t w = help(OV(0x001b4500), OV(0x001b45e0));
        w32(w + 0x44, u(ri(-1, 5)));
        for (uint32_t k : {0x48u, 0x4cu}) {
            const uint32_t b = ee_read<uint32_t>(w + k);
            if (b != 0) { w32(b + 8, 1u << ri(5, 8)); w32(b + 0xc, 1u << ri(5, 8)); }
        }
        w32(OV(0x0015f418), u(ri(0, 0x3d)));
        return w;
    };
    check("two_pictures_update", 0x0027e290, 40, Ret::Int32, [twop] { return Args{{twop()}, {}}; },
          [](A a) { return ri32(two_pictures_update(a.i[0])); });
    check("two_pictures_draw", 0x0027e3d8, 20, Ret::Int32, [twop] { return Args{{twop()}, {}}; },
          [](A a) { return ri32(two_pictures_draw(a.i[0])); });
    check("items_help_focus_in", 0x0027a6e0, 5, Ret::Int32, [help] { const uint32_t w = help(OV(0x001b4648), OV(0x001b48f8)); w8(0x0013d4c2, u(ri(0, 1))); return Args{{w}, {}}; },
          [](A a) { return ri32(items_help_focus_in(a.i[0])); });
    check("weapons_help_focus_in", 0x0027f240, 10, Ret::Int32, [help] { return Args{{help(OV(0x001b4d20), OV(0x001b4da8))}, {}}; },
          [](A a) { return ri32(weapons_help_focus_in(a.i[0])); });
    check("gadgets_help_focus_in", 0x0027f308, 10, Ret::Int32, [help] { return Args{{help(OV(0x001b50a8), OV(0x001b5130))}, {}}; },
          [](A a) { return ri32(gadgets_help_focus_in(a.i[0])); });
    auto icons = [help] {
        const bool g = coin();
        const uint32_t w = help(g ? OV(0x001b50a8) : OV(0x001b4d20), g ? OV(0x001b5188) : OV(0x001b4e00));
        if (g) gadgets_help_focus_in(0); else weapons_help_focus_in(0);
        const int32_t n = ee_read<int32_t>(w + 0x40);
        w32(w + 0x3c, u(ri(0, n > 0 ? n - 1 : 0)));
        w32(w + 0x60, u(ri(0, 6)));
        w32(w + 0x5c, u(ri(-0x1000, 0x400)));
        if (coin()) w32(w + 0x24, u(ri(20, 60)));
        return w;
    };
    check("icon_list_update", 0x00277a58, 80, Ret::Int32, [icons] { return Args{{icons()}, {}}; },
          [](A a) { return ri32(icon_list_update(a.i[0])); });
    check("icon_list_draw", 0x00277be8, 40, Ret::Int32, [icons] { return Args{{icons()}, {}}; },
          [](A a) { return ri32(icon_list_draw(a.i[0])); });
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle



