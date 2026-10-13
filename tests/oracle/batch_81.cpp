// Oracle batch 81: the game's language (0x15ed88) as the main menu's Options > Language page
// sets and shows it (docs/reversing/menu.md §13.8): the page 0x1b8a40's list 0x1b8b68 (entries
// 0x1b8b20: English 0, French 2, German 3, Spanish 4, Italian 5), its update 0x278840 (cross:
// action 9 writes the entry's value to 0x15ed88) and its drawing 0x27b0f0 (flag 0x800: the
// entry of the current language marked), on Veldin's real level data (needs the ISO).

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/pause_menu.h>
#include <game/ovl/pause_menu_pages.h>

namespace rac1::oracle {

namespace {
constexpr uint32_t PAD = 0x0013c940;
constexpr OvConst<0x001b9cf0u> MS{};
constexpr uint32_t LANGUAGE = 0x0015ed88;
constexpr OvConst<0x001b8a40u> P_LANGUAGE{};
constexpr OvConst<0x001b8b68u> W_LANGUAGES{};
}  // namespace

void batch_81() {
    if (g_only_batch != 81) return;
    auto boot = [] { veldin_boot(); };
    g_batch = 81;
    g_ignored_words = vcross_lanes();
    // the callees left TODO_PORT (as in batches 73 / 76)
    g_extra_noop_hooks = {0x00122298, OVF(0x002a0f18), OVF(0x002a08c8), OVF(0x0028ed58),
                          OVF(0x00299a68), OVF(0x0024fe50), OVF(0x0024ff08), OVF(0x0024ff88), OVF(0x0024fad0),
                          OVF(0x00265aa8)};
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    static std::vector<uint8_t> snap, snap_page;
    auto packet = [] {  // the frame packet: buffer 0, the cursor at a random point of it
        w32(OV(0x001611b8), ee_read<uint32_t>(OV(0x00173e08)));
        w32(OV(0x001611bc), ee_read<uint32_t>(OV(0x00173e08)) + 0x30000);
        w32(OV(0x001611d0), 0);
        w32(OV(0x001611c0), ee_read<uint32_t>(OV(0x001611b8)) + u(ri(0, 0x100)) * 0x10);
    };
    auto frames = [](int n) {
        for (int f = 0; f < n; ++f) {
            w32(PAD + 0x1c4, 0);
            w32(PAD + 0x1b4, 0);
            menu_update();
        }
    };
    // the menu open (the port's frames from Start), then the language page asked for
    auto page = [boot, packet, frames] {
        uint8_t* ram = ee_get_ram_base();
        if (snap_page.empty()) {
            boot();
            ee_write<uint16_t>(0x001518d0, 512);
            ee_write<uint16_t>(0x001518d2, 416);
            particle_table_init();
            level_setup_body(1, 0);
            hud_init();
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            w32(0x0015ec8c, 1);
            snap.assign(ram, ram + 0x02000000);
            for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); ee_read<uint8_t>(m + 0x20) != 0xff; m += 0x100) {
                const uint32_t fn = ee_read<uint32_t>(m + 0x74);
                if (fn != 0 && !ovl_moby_fn_ported(fn)) g_extra_noop_hooks.push_back(fn);
            }
            w32(OV(0x0016c06c), 0);
            menu_open(0);
            frames(16);
            w32(MS + 8, P_LANGUAGE);
            frames(16);
            snap_page.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap_page.data(), snap_page.size());
        }
        packet();
    };
    static const int32_t langs[] = {0, 1, 2, 3, 4, 5};
    // the language list's update: cross mostly (action 9), the other keys and focus changes too
    check("menu_button_update_language", 0x00278840, 120, Ret::Int32, [page] {
        page();
        static const uint32_t b[] = {0x40, 0x40, 0x40, 0x1000, 0x4000, 0x10, 0x800, 0x100, 4, 8, 0x20, 0};
        const uint32_t k = b[ri(0, 11)];
        w32(PAD + 0x1c4, k);
        w32(PAD + 0x1b4, coin() ? k : b[ri(0, 11)]);
        w32(LANGUAGE, u(langs[ri(0, 5)]));
        w32(0x0015eeb0, u(ri(0, 3) == 0 ? 0x10 : ri(0, 2)));
        w32(OV(0x00160274), u(ri(0, 30)));
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        if (coin()) w32(ee_read<uint32_t>(MS + 4) + 0x40, W_LANGUAGES);
        w32(W_LANGUAGES + 0x40, u(ri(0, 4)));
        for (uint32_t p = ee_read<uint32_t>(W_LANGUAGES + 0x34); ee_read<int16_t>(p) != 0; p += 0xc) w16(p + 0xa, u(ri(-3, 40)));
        return Args{{W_LANGUAGES}, {}};
    }, [](A a) { return ri32(menu_button_update(a.i[0])); });
    // its drawing: the current language's entry marked (flag 0x800)
    check("text_list_draw_language", 0x0027b0f0, 80, Ret::Int32, [page] {
        page();
        w32(W_LANGUAGES + 0x20, u(ri(120, 300)));
        w32(W_LANGUAGES + 0x24, u(ri(80, 200)));
        w32(MS + 0x124, ri(0, 3) == 0 ? 1u : 0u);
        w16(0x001516d8, u(ri(0, 3) == 0 ? 1 : 0));
        if (ee_read<int16_t>(0x001516d8) == 0) w32(0x00137b00, 0);
        w32(LANGUAGE, u(langs[ri(0, 5)]));
        w32(OV(0x0015f3f8), R());
        w32(W_LANGUAGES + 0x40, u(ri(0, 4)));
        w32(W_LANGUAGES + 0x44, u(ri(-40, 40)));
        if (coin()) w32(ee_read<uint32_t>(MS + 4) + 0x40, W_LANGUAGES);
        if (ri(0, 3) == 0) w32(W_LANGUAGES + 0x30, ee_read<uint32_t>(W_LANGUAGES + 0x30) ^ 0x800u);
        return Args{{W_LANGUAGES}, {}};
    }, [](A a) { return ri32(text_list_draw(a.i[0])); });
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
