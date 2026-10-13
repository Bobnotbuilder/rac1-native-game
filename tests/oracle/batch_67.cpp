// Oracle batch 67: the HUD, its data and textures, the IOP stash, the frame's packets (needs the ISO).

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
namespace rac1::oracle {

void batch_67() {
    if (g_only_batch != 67) return;
    auto boot = [] { veldin_boot(); };
    g_batch = 67;
    g_ignored_words = vcross_lanes();
    g_extra_noop_hooks.clear();
    static std::vector<uint8_t> snap;
    auto level = [boot] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            boot();
            level_setup_body(1, 0);
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
        // the frame packet (its buffers set by the overlay's start): buffer 0, the cursor at a
        // random point of it
        w32(OV(0x001611b8), ee_read<uint32_t>(OV(0x00173e08)));
        w32(OV(0x001611bc), ee_read<uint32_t>(OV(0x00173e08)) + 0x30000);
        w32(OV(0x001611d0), 0);
        w32(OV(0x001611c0), ee_read<uint32_t>(OV(0x001611b8)) + u(ri(0, 0x100)) * 0x10);
        w32(OV(0x0015f418), u(ri(0, 3) == 0 ? ri(0x3c, 0x40) : ri(0, 0x20)));  // the upload queue
        w32(0x0015ee74, ee_read<uint32_t>(0x0015ee78) + u(ri(0, 0x40)) * 0x100);
    };
    static constexpr OvConst<0x0017e4d8u> H{};
    auto keys = [] {  // the HUD animations' keys
        std::vector<uint32_t> k;
        for (uint32_t p = ee_read<uint32_t>(H + 0x1c); ee_read<uint16_t>(p) != 0xffff && k.size() < 0x400; p += 8) k.push_back(ee_read<uint16_t>(p));
        return k;
    };
    auto key = [keys] { static const std::vector<uint32_t> k = keys(); return ri(0, 9) == 0 || k.empty() ? R() & 0xffff : k[static_cast<size_t>(ri(0, static_cast<int32_t>(k.size()) - 1))]; };
    auto frame = [key] {  // a HUD frame index: a loaded one mostly
        for (int t = 0; t < 8; ++t) { const int32_t f = hud_frame(key(), ri(0, 3)); if (f != 0 && ri(0, 7) != 0) return f; }
        return ri(0, 40);
    };
    auto unload = [] {  // some palettes / textures not resident (GS block 0)
        for (uint32_t i = 0; i < 0x200; ++i) if (ri(0, 3) == 0) { w16(ee_read<uint32_t>(H + 0x24) + i * 8 + 4, 0); w16(ee_read<uint32_t>(H + 0x28) + i * 8 + 4, 0); }
    };
    auto text = [](uint32_t a) {  // a string: printable, colour codes, accents, wide glyphs
        const int n = ri(0, 24);
        for (int i = 0; i < n; ++i) {
            const int k = ri(0, 9);
            w8(a + u(i), k < 6 ? u(ri(0x21, 0x7e)) : k == 6 ? 0x20u : k == 7 ? u(ri(8, 15)) : k == 8 ? u(ri(0x80, 0xa7)) : u(ri(1, 0x1f)));
        }
        w8(a + u(n), 0);
    };
    check("gs_set_def_load_image", 0x00122330, 60, Ret::Int32, [level] {
        level();
        static const uint32_t psms[] = {0, 1, 2, 0xa, 0x13, 0x14, 0x1b, 0x24, 0x2c, 0x30, 0x31, 0x32, 0x3a, 5};
        const uint32_t psm = psms[ri(0, 13)];
        return Args{{OUT, R() & 0x3fff, u(ri(1, 16)), psm, u(ri(-4, 64)), u(ri(-4, 64)), u(1 << ri(0, 8)), u(1 << ri(0, 8))}, {}};  // (no more than 0x7fff quadwords: the error is a printf)
    }, [](A a) {
        return ri32(gs_set_def_load_image(a.i[0], static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]),
                                          static_cast<int32_t>(a.i[4]), static_cast<int32_t>(a.i[5]), static_cast<int32_t>(a.i[6]), static_cast<int32_t>(a.i[7])));
    });
    check("hud_tex_upload", 0x0023c7b0, 40, Ret::None, [level] {
        level();
        return Args{{0x01000000u + (R() & 0xffff0u), R() & 0x3fff, 0x1b, u(ri(4, 8)), u(ri(4, 8)), 0}, {}};
    }, [](A a) { hud_tex_upload(a.i[0], static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]), 0); return uint64_t{0}; });
    check("hud_tex_relocate", 0x002351b0, 30, Ret::None, [level] {
        level();
        const uint32_t hdr = ee_read<uint32_t>(H + 0x18);
        if (coin()) w32(hdr + 0x74, 0);
        if (coin()) w32(hdr + 0x78, 0);
        return Args{{u(ri(0, 1)), 0x00400000u + (R() & 0xffff0u) + u(ri(0, 15))}, {}};
    }, [](A a) { hud_tex_relocate(static_cast<int32_t>(a.i[0]), a.i[1]); return uint64_t{0}; });
    check("hud_tex_upload_set", 0x00235490, 20, Ret::None, [level] {
        level();
        const uint32_t hdr = ee_read<uint32_t>(H + 0x18);
        if (coin()) w32(hdr + 0x74, 0);
        if (coin()) w32(hdr + 0x78, 0);
        w32(0x0015ee88, R() & 0xfff00u);
        return Args{{u(ri(0, 1)), 0x00400000u + (R() & 0xffff0u), 0}, {}};
    }, [](A a) { hud_tex_upload_set(static_cast<int32_t>(a.i[0]), a.i[1], 0); return uint64_t{0}; });
    // (hud_data_load / hud_data_decompress: batch 59, in the level setup; later the level file's
    // compressed blocks are gone)
    // the stash: some entries taken, the free space random
    auto stash = [level] {
        level();
        const int n = ri(0, 3) == 0 ? 0x40 : ri(0, 8);
        const uint32_t base = ee_read<uint32_t>(OV(0x001c12a0));
        uint32_t cur = base;
        for (int i = 0; i < n; ++i) { const uint32_t q = u(ri(1, 0x200)); w32(OV(0x001c12d8) + u(i) * 16, cur); w32(OV(0x001c12dc) + u(i) * 16, q); w32(OV(0x001c12e0) + u(i) * 16, R()); cur += q * 16; }
        w32(OV(0x001c12d4), u(n));
        w32(OV(0x001c12d0), ri(0, 5) == 0 ? base + ee_read<uint32_t>(OV(0x001c12a4)) - u(ri(0, 0x100)) * 16 : cur);
    };
    check("stash_put", 0x00293c58, 40, Ret::Int32, [stash] {
        stash();
        const uint32_t q = u(ri(0, 0x400));
        return Args{{0x01266000u + (R() & 0xfff0u), q, q + u(ri(0, 2)), R()}, {}};
    }, [](A a) { return ri32(stash_put(a.i[0], static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), a.i[3])); });
    check("stash_size", 0x00293ed0, 20, Ret::Int32, [stash] { stash(); return Args{{u(ri(-2, 0x44))}, {}}; },
          [](A a) { return ri32(stash_size(a.i[0])); });
    check("stash_write", 0x00293d38, 30, Ret::Int32, [stash] {
        stash();
        return Args{{0x01266000u + (R() & 0xfff0u), u(ri(-1, 0x41)), u(ri(0, 0x100)), u(ri(0, 0x100))}, {}};
    }, [](A a) { return ri32(stash_write(a.i[0], a.i[1], static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]))); });
    check("stash_read", 0x00293db8, 30, Ret::Int32, [stash] {
        stash();
        for (uint32_t i = 0; i < 0x40; ++i) { const uint32_t t = T4; w32(t, 0x01266000u + i * 0x2000); w32(t + 4, ee_read<uint32_t>(OV(0x001c12a0)) + i * 0x2000); w32(t + 8, 0x2000); w32(t + 12, 0); sif_set_dma(t, 1); }
        return Args{{OUT, u(ri(-1, 0x41)), u(ri(0, 0x40)), coin() ? 0xffffffffu : u(ri(0, 0x40)), 0}, {}};
    }, [](A a) { return ri32(stash_read(a.i[0], a.i[1], static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), a.i[4])); });
    check("hud_frame", 0x0023b208, 60, Ret::Int32, [level, key, unload] {
        level();
        if (coin()) unload();
        for (uint32_t i = 0; i < 0x200; ++i) if (ri(0, 9) == 0) w32(ee_read<uint32_t>(H + 0x28) + i * 8, R());
        return Args{{key(), u(ri(-1, 4))}, {}};
    }, [](A a) { return ri32(hud_frame(a.i[0], static_cast<int32_t>(a.i[1]))); });
    check("hud_tex0", 0x0023b2b8, 60, Ret::Int64, [level, frame, unload] {
        level();
        const int32_t f = frame();
        if (coin()) unload();
        return Args{{u(f)}, {}};
    }, [](A a) { return hud_tex0(static_cast<int32_t>(a.i[0])); });
    check("hud_sprite", 0x0023b4d8, 40, Ret::None, [level, frame, unload] {
        level();
        const int32_t f = frame();
        if (ri(0, 3) == 0) unload();
        w32(H + 0xc, R() & 0xffffff);
        return Args{{u(f), u(ri(-20, 520)), u(ri(-20, 460)), u(ri(0, 128)), u(ri(0, 128)), u(ri(0, 0x80))}, {}};
    }, [](A a) { hud_sprite(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]), static_cast<uint64_t>(sx(a.i[5]))); return uint64_t{0}; });
    check("hud_sprite_flip", 0x0023b860, 40, Ret::None, [level, frame, unload] {
        level();
        const int32_t f = frame();
        if (ri(0, 3) == 0) unload();
        w32(H + 0xc, R() & 0xffffff);
        return Args{{u(f), u(ri(-20, 520)), u(ri(-20, 460)), u(ri(0, 128)), u(ri(0, 128)), u(ri(0, 0x80))}, {}};
    }, [](A a) { hud_sprite_flip(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]), static_cast<uint64_t>(sx(a.i[5]))); return uint64_t{0}; });
    check("hud_sprite_at", 0x0023b120, 40, Ret::None, [level, frame] {
        level();
        const uint32_t el = OV(0x0017dc50) + u(ri(0, 12)) * 0x90;
        w16(el + 0x48, u(ri(-100, 400)));
        w16(el + 0x4a, u(ri(-100, 400)));
        return Args{{el, coin() ? u(frame()) : u(ri(-3, 3)), u(ri(-50, 300)), u(ri(-50, 300)), R() & 7, u(ri(0, 0x80))}, {}};
    }, [](A a) { hud_sprite_at(a.i[0], static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), a.i[4], static_cast<uint64_t>(sx(a.i[5]))); return uint64_t{0}; });
    check("hud_box", 0x0023cea8, 30, Ret::None, [level] {
        level();
        return Args{{u(ri(-50, 400)), u(ri(-50, 400)), u(ri(0x40, 400)), u(ri(8, 64)), u(ri(0, 0x50))}, {}};
    }, [](A a) { hud_box(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<uint64_t>(sx(a.i[4]))); return uint64_t{0}; });
    check("text_width", 0x001fb040, 40, Ret::Int32, [level, text] {
        level();
        text(OUT);
        return Args{{OUT, coin() ? 0xffffffffu : u(ri(0, 10)), coin() ? OV(0x001c3150u) : OV(0x001c3890u)}, {}};
    }, [](A a) { return ri32(text_width(a.i[0], static_cast<int32_t>(a.i[1]), a.i[2])); });
    check("text_width_small", 0x001fb090, 10, Ret::Int32, [level, text] { level(); text(OUT); return Args{{OUT, 0xffffffffu}, {}}; },
          [](A a) { return ri32(text_width_small(a.i[0], static_cast<int32_t>(a.i[1]))); });
    check("text_width_big", 0x001fb0d0, 10, Ret::Int32, [level, text] { level(); text(OUT); return Args{{OUT, u(ri(0, 8))}, {}}; },
          [](A a) { return ri32(text_width_big(a.i[0], static_cast<int32_t>(a.i[1]))); });
    check("glyph_sprite", 0x001fa290, 30, Ret::None, [level] {
        level();
        return Args{{u(ri(-20, 500)), u(ri(-20, 440)), u(ri(0, 32)), u(ri(0, 32)), u(ri(0, 255)), u(ri(0, 255)), u(ri(0, 32)), u(ri(0, 32)), R(), R()}, {}};
    }, [](A a) {
        glyph_sprite(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]),
                     static_cast<int32_t>(a.i[5]), static_cast<int32_t>(a.i[6]), static_cast<int32_t>(a.i[7]), static_cast<uint64_t>(sx(a.i[8])), static_cast<uint64_t>(sx(a.i[9])));
        return uint64_t{0};
    });
    check("effect_tex0", 0x001f9298, 40, Ret::Int64, [level] {
        level();
        for (uint32_t i = 0; i < 8; ++i) if (coin()) { w32(OV(0x0016d180) + i * 16, 0); w32(OV(0x0016d184) + i * 16, 0); }
        return Args{{u(ri(0, 7))}, {}};
    }, [](A a) { return effect_tex0(static_cast<int32_t>(a.i[0])); });
    auto texts = [level, text] {
        level();
        text(OUT);
        w32(OV(0x0015f460), u(ri(0, 1)));
        w32(OV(0x0015f45c), u(ri(0, 1)));
        for (uint32_t i = 0; i < 8; ++i) w32(OV(0x0016c838) + i * 4, R());
        for (uint32_t i = 0; i < 8; ++i) if (coin()) { w32(OV(0x0016d180) + i * 16, 0); w32(OV(0x0016d184) + i * 16, 0); }
    };
    check("text_centered", 0x001fbad8, 40, Ret::Int32, [texts] {
        texts();
        return Args{{u(ri(0, 512)), u(ri(0, 448)), R(), OUT, coin() ? 0xffffffffu : u(ri(0, 12))}, {}};
    }, [](A a) { return ri32(text_centered(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<uint64_t>(sx(a.i[2])), a.i[3], static_cast<int32_t>(a.i[4]))); });
    check("text_right", 0x001fb7f8, 40, Ret::None, [texts] {
        texts();
        return Args{{u(ri(0, 512)), u(ri(0, 448)), R(), OUT, coin() ? 0xffffffffu : u(ri(0, 12))}, {}};
    }, [](A a) { text_right(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<uint64_t>(sx(a.i[2])), a.i[3], static_cast<int32_t>(a.i[4])); return uint64_t{0}; });
    check("hud_message_box", 0x0023cf80, 30, Ret::None, [texts] {
        texts();
        return Args{{u(ri(0, 512)), u(ri(0, 448)), R(), OUT}, {}};
    }, [](A a) { hud_message_box(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<uint64_t>(sx(a.i[2])), a.i[3]); return uint64_t{0}; });
    check("hud_draw", 0x0023af48, 40, Ret::None, [texts, text] {
        texts();
        text(OV(0x00179118));
        text(OV(0x00179168));
        w32(H + 0x30, ri(0, 4) == 0 ? 1u : 0u);
        w32(OV(0x0015f404), ri(0, 6) == 0 ? 1u : 0u);
        w32(OV(0x0015f5c4), ri(0, 6) == 0 ? 6u : 0u);
        w32(OV(0x0015f640), coin() ? 0u : u(ri(0, 1200)));
        w32(OV(0x0015f644), u(ri(-10, 0x90)));
        w32(OV(0x0015f648), u(ri(0, 400)));
        for (uint32_t i = 0; i < 13; ++i) w32(OV(0x0017dc50) + i * 0x90 + 0x18, 0);
        return Args{};
    }, [](A) { hud_draw(); return uint64_t{0}; });
    check("hud_alloc", 0x002355f0, 30, Ret::Int32, [level] {
        level();
        const uint32_t b = ee_read<uint32_t>(OV(0x00173e0c));
        w32(H + 0x10, ri(0, 4) == 0 ? 0u : b + u(ri(0, 0x6400)) * 16);
        w32(H + 0x14, b + 0x64000);
        return Args{{u(ri(0, 3) == 0 ? ri(0x10000, 0x70000) : ri(0, 0x2000))}, {}};
    }, [](A a) { return ri32(static_cast<int32_t>(hud_alloc(static_cast<int32_t>(a.i[0])))); });
    check("hud_alloc_reset", 0x002355c8, 4, Ret::None, [level] { level(); w32(H + 0x10, R()); return Args{}; }, [](A) { hud_alloc_reset(); return uint64_t{0}; });
    auto slots = [level] {
        level();
        for (uint32_t i = 0; i < 13; ++i) {
            const uint32_t e = OV(0x0017dc50) + i * 0x90;
            w32(e + 0x64, coin() ? u(ri(0, 20)) : 0xffffffffu);
            w32(e + 0x68, u(ri(0, 1)));
            w32(e + 4, R() & 0x11);
            w32(e + 0x7c, u(ri(-2, 12)));
            w32(e + 0x6c, u(ri(-7, 0x1f)));
            w32(e + 0x14, 0);
            w32(e + 0x18, 0);
        }
    };
    check("hud_request_flags", 0x002359f8, 30, Ret::None, [slots] { slots(); return Args{{u(ri(-1, 20)), R()}, {}}; },
          [](A a) { hud_request_flags(static_cast<int32_t>(a.i[0]), a.i[1]); return uint64_t{0}; });
    check("hud_init", 0x00234e98, 6, Ret::None, [slots] {
        slots();
        if (coin()) w32(OV(0x0015fa70), 0);
        return Args{};
    }, [](A) { hud_init(); return uint64_t{0}; });
    check("hud_update", 0x0023ac78, 40, Ret::Int32, [slots] {
        slots();
        w32(H + 4, ri(0, 5) == 0 ? 1u : 0u);
        w32(OV(0x0015f968), u(ri(0, 4) == 0));
        w32(0x0013f350 + 0x10b8, u(ri(0, 3) == 0 ? 0 : ri(0, 0x24)));
        w8(0x0013f350 + 0x20a4, u(ri(0, 3) == 0 ? ri(0, 2) : 0));
        w32(OV(0x0015f970), coin() ? 0xffffffffu : u(ri(0, 12)));
        return Args{{u(ri(0, 1))}, {}};
    }, [](A a) { return ri32(hud_update(static_cast<int32_t>(a.i[0]))); });
    // the HUD elements: a slot with random state
    auto element = [slots, frame](uint32_t e) {
        slots();
        for (int k = 0; k < 4; ++k) (void)frame();
        w32(e + 8, u(ri(0, 3) == 0 ? ri(0, 2000000) : ri(1, 12)));
        w32(e + 0xc, ri(0, 5) == 0 ? (coin() ? 0u : 0x0015ed99u) : (coin() ? 0x001415f8u : 0x0015ed98u));
        w32(0x001415f8, u(ri(-2, 10)));
        w32(0x0015ed98, u(ri(0, 3) == 0 ? ri(0, 99999999) : ri(0, 2000)));
        w32(e + 0x50, u(ri(0, 500)));
        w32(e + 0x58, u(ri(0, 0x40)));
        w32(e + 0x5c, u(ri(0, 0x40)));
        w32(e + 0x60, R() & 0xf);
        w32(e + 0x74, ri(0, 3) == 0 ? u(ri(0, 12)) : ee_read<uint32_t>(e + 0x78));
        w32(e + 0x78, u(ri(0, 12)));
        w32(e + 0x7c, u(ri(-5, 200)));
        w8(e + 0x70, u(ri(0, 12)));
        w8(e + 0x71, u(ri(0, 12)));
        w32(e + 0x80, coin() ? OV(0x0015fab0u) : OV(0x0015fab8u));
        for (uint32_t i = 0; i < 8; i += 2) w16(OV(0x0015fab0) + i, u(ri(-0x1000, 0x1100)));
        w16(OV(0x0015fab8), u(ri(-5, 70)));
        w16(e + 0x40, u(ri(0, 60)));
        w8(e + 0x42, u(ri(0, 4)));
        w32(e + 0x4c, ee_read<uint32_t>(OV(0x0015f5cc)) + u(ri(-200, 50)));
        w32(e + 0x44, u(ri(0, 100)));
        w32(0x0015eda0, u(ri(0, 3) == 0 ? ri(0, 10) : (coin() ? 8 : 5)));
        w8(0x0013f350 + 0x20a4, u(ri(0, 4) == 0 ? ri(1, 2) : 0));
        w32(0x0013f350 + 0x22ac, u(ri(0, 9)));
        w32(0x0013f350 + 0x2084, ri(0, 9) == 0 ? 0x32u : 0u);
        w32(0x0015ed80, u(ri(0, 1)));
        w32(0x0015ed88, u(ri(0, 6)));
    };
    auto el = [] { return OV(0x0017dc50) + u(ri(0, 12)) * 0x90; };
    check("hud_element_size", 0x00235c80, 60, Ret::None, [element, el] { const uint32_t e = el(); element(e); return Args{{e}, {}}; },
          [](A a) { hud_element_size(a.i[0]); return uint64_t{0}; });
    check("hud_anim_frame", 0x0023aaa8, 120, Ret::Int32, [element, el] { const uint32_t e = el(); element(e); return Args{{e + 0x40}, {}}; },
          [](A a) { return ri32(hud_anim_frame(a.i[0])); });
    check("hud_health_start", 0x002395b0, 30, Ret::None, [element, el] { const uint32_t e = el(); element(e); return Args{{e}, {}}; },
          [](A a) { hud_health_start(a.i[0]); return uint64_t{0}; });
    check("hud_health_update", 0x00239630, 120, Ret::None, [element, el] { const uint32_t e = el(); element(e); return Args{{e}, {}}; },
          [](A a) { hud_health_update(a.i[0]); return uint64_t{0}; });
    check("hud_health_draw", 0x00239810, 120, Ret::Int32, [element, el] { const uint32_t e = el(); element(e); return Args{{e}, {}}; },
          [](A a) { return ri32(hud_health_draw(a.i[0])); });
    check("hud_counter_start", 0x0023a760, 30, Ret::None, [element, el] { const uint32_t e = el(); element(e); return Args{{e}, {}}; },
          [](A a) { hud_counter_start(a.i[0]); return uint64_t{0}; });
    check("hud_bolts_start", 0x00239cc8, 30, Ret::None, [element, el] { const uint32_t e = el(); element(e); return Args{{e}, {}}; },
          [](A a) { hud_bolts_start(a.i[0]); return uint64_t{0}; });
    check("hud_bolts_update", 0x00239d00, 120, Ret::None, [element, el] { const uint32_t e = el(); element(e); return Args{{e}, {}}; },
          [](A a) { hud_bolts_update(a.i[0]); return uint64_t{0}; });
    check("hud_bolts_draw", 0x00239df8, 120, Ret::Int32, [element, el] { const uint32_t e = el(); element(e); return Args{{e}, {}}; },
          [](A a) { return ri32(hud_bolts_draw(a.i[0])); });
    check("hud_plain_start", 0x00235d80, 30, Ret::None, [element, el] { const uint32_t e = el(); element(e); return Args{{e}, {}}; },
          [](A a) { hud_plain_start(a.i[0]); return uint64_t{0}; });
    check("hud_plain_update", 0x00235ea0, 200, Ret::None, [element, el] {
        const uint32_t e = el(); element(e);
        w32(e + 0x6c, u(ri(0, 1) ? ri(0x18, 0x1e) : ri(-6, 0x17)));
        w8(e + 0x73, u(ri(0, 255)));
        if (coin()) w32(e + 0x74, u(ri(0, 3000)));
        return Args{{e}, {}};
    }, [](A a) { hud_plain_update(a.i[0]); return uint64_t{0}; });
    check("hud_ammo_update", 0x0023cdb8, 100, Ret::None, [element, el] {
        const uint32_t e = el(); element(e);
        w32(e + 0xc, 0x0013d428 + u(ri(0, 0x24)) * 4);
        for (uint32_t i = 0; i < 0x25; ++i) w32(0x0013d428 + i * 4, u(ri(-3, 150)));
        return Args{{e}, {}};
    }, [](A a) { hud_ammo_update(a.i[0]); return uint64_t{0}; });
    check("hud_ammo_draw", 0x0023a7a8, 100, Ret::Int32, [element, el, key] {
        const uint32_t e = el(); element(e);
        w32(e, key());
        w32(e + 8, u(ri(0, 3) ? ri(1, 99) : ri(100, 999)));
        w32(e + 0x74, u(ri(0, 3) ? ri(0, 99) : 0));
        return Args{{e}, {}};
    }, [](A a) { return ri32(hud_ammo_draw(a.i[0])); });
    check("hud_sprite_texels", 0x0023b6c0, 40, Ret::None, [level, frame, unload] {
        level();
        const int32_t f = frame();
        if (ri(0, 3) == 0) unload();
        w32(H + 0xc, R() & 0xffffff);
        return Args{{u(f), u(ri(-20, 520)), u(ri(-20, 460)), u(ri(0, 256)), u(ri(0, 128)), u(ri(0, 0x80))}, {}};
    }, [](A a) { hud_sprite_texels(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]), static_cast<uint64_t>(sx(a.i[5]))); return uint64_t{0}; });
    check("hud_element_offset", 0x00235ad8, 100, Ret::None, [element, el] {
        const uint32_t e = el(); element(e);
        w32(e + 0x60, R() & 0xf); w32(e + 0x7c, u(coin()));
        w32(OUT, u(ri(0, 500))); w32(OUT + 4, u(ri(0, 400)));
        return Args{{e, OUT, OUT + 4, u(ri(-5, 30)), u(ri(0, 4))}, {}};
    }, [](A a) { hud_element_offset(a.i[0], a.i[1], a.i[2], static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4])); return uint64_t{0}; });
    check("hud_mode2_draw", 0x0023a640, 100, Ret::Int32, [element, el] {
        const uint32_t e = el(); element(e);
        w32(e + 8, u(ri(1, 200)));
        w32(e + 0x74, u(ri(0, 200)));
        w16(0x00140986, u(coin()));
        return Args{{e}, {}};
    }, [](A a) { return ri32(hud_mode2_draw(a.i[0])); });
    // the quick select: its slots from random items, the pad's stick / d-pad / triangle
    auto qs = [element, el](bool start) {
        const uint32_t e = el(); element(e);
        for (uint32_t i = 0; i < 8; ++i) w32(0x00141ea0 + i * 4, ri(0, 3) == 0 ? 0u : u(ri(1, 0x24)));
        if (start) return e;
        quick_select_start(e);
        for (uint32_t i = 0; i < 8; ++i) {  // the d-pad neighbours
            for (uint32_t k = 8; k <= 0x14; k += 4) w32(OV(0x0017dac0) + i * 0x1c + k, u(ri(-1, 7)));
            if (ri(0, 4) == 0) w32(OV(0x0017dac0) + i * 0x1c, 0);
        }
        for (uint32_t k = 0x18; k <= 0x24; k += 4) w32(OV(0x0017dc00) + k, u(ri(0, 7)));
        w32(e + 0x74, u(ri(-1, 7)));
        w32(e + 0x78, ri(0, 3) == 0 ? 0x010000ffu : (coin() ? u(ri(0, 3)) : 0x01000000u));
        const float r = ri(0, 2) == 0 ? rf(0, 0.6f) : rf(0.85f, 1.0f), t = rf(-3.2f, 3.2f);
        wf(0x0013c940 + 0x148, r * std::cos(t));
        wf(0x0013c940 + 0x14c, r * std::sin(t));
        w32(0x0013c940 + 0x1c0, coin() ? 0x10u : 0u);
        w32(0x0013c940 + 0x1c4, ri(0, 2) == 0 ? (R() & 0xf000) : 0u);
        w32(0x0013c940 + 0x1d8, u(ri(0, 4) == 0));
        w16(0x00141622, u(ri(0, 3)));
        w16(0x001418e8, coin() ? 0xffffu : u(ri(0, 100)));
        w32(0x0015eea4, u(ri(0, 100000)));
        wf(OV(0x0015f788), rf(0, 0.4f));
        w32(0x00140408, u(ri(0, 0x24)));
        for (uint32_t i = 0; i < 0x25; ++i) w32(0x0013d428 + i * 4, u(ri(0, 300)));
        for (uint32_t i = 0; i < 0x30; ++i) w8(0x0013e520 + i, u(ri(0, 3) == 0));
        return e;
    };
    check("hud_request_clear", 0x00235908, 30, Ret::Int32, [slots] { slots(); return Args{{u(ri(-1, 20))}, {}}; },
          [](A a) { return ri32(hud_request_clear(static_cast<int32_t>(a.i[0]))); });
    check("hud_element_place", 0x00235a70, 40, Ret::Int32, [element, el] {
        const uint32_t e = el(); element(e); w32(OUT, R() & 0xfff); w32(OUT + 4, R() & 0xfff);
        return Args{{e, OUT, OUT + 4}, {}};
    }, [](A a) { return ri32(hud_element_place(a.i[0], a.i[1], a.i[2])); });
    check("text_centered_small", 0x001fb9a8, 30, Ret::Int32, [texts] {
        texts();
        return Args{{u(ri(0, 512)), u(ri(0, 448)), R(), OUT, coin() ? 0xffffffffu : u(ri(0, 12))}, {}};
    }, [](A a) { return ri32(text_centered_small(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<uint64_t>(sx(a.i[2])), a.i[3], static_cast<int32_t>(a.i[4]))); });
    check("quick_select_start", 0x00237ae8, 20, Ret::None, [qs] { return Args{{qs(true)}, {}}; },
          [](A a) { quick_select_start(a.i[0]); return uint64_t{0}; });
    check("quick_select_update", 0x00237ba0, 300, Ret::None, [qs] { return Args{{qs(false)}, {}}; },
          [](A a) { quick_select_update(a.i[0]); return uint64_t{0}; });
    check("quick_select_draw", 0x002382a0, 150, Ret::Int32, [qs] { return Args{{qs(false)}, {}}; },
          [](A a) { return ri32(quick_select_draw(a.i[0])); });
    // the Gadgebot command ring (task 0415): its 4 slots (page 0x15f740) with random neighbours / commands, the pad, 0x17e504
    auto gc = [element, el](bool start) {
        const uint32_t e = el(); element(e);
        if (start) return e;
        gadgebot_cmd_start(e);
        const uint32_t page = ee_read<uint32_t>(OV(0x0015f740));
        for (uint32_t i = 0; i < 4; ++i) {
            for (uint32_t k = 8; k <= 0x14; k += 4) w32(page + i * 0x1c + k, u(ri(-1, 3)));
            w32(page + i * 0x1c + 0x18, u(ri(0, 5)));
            if (ri(0, 5) == 0) w32(page + i * 0x1c, 0);
        }
        for (uint32_t k = 0x38; k <= 0x44; k += 4) w32(OV(0x0017dc00) + k, u(ri(0, 3)));
        w32(e + 0x74, u(ri(-2, 3)));
        w32(e + 0x78, ri(0, 3) == 0 ? 0x010000ffu : (coin() ? u(ri(0, 3)) : 0x01000000u));
        const float r = ri(0, 2) == 0 ? rf(0, 0.6f) : rf(0.85f, 1.0f), t = rf(-3.2f, 3.2f);
        wf(0x0013c940 + 0x148, r * std::cos(t));
        wf(0x0013c940 + 0x14c, r * std::sin(t));
        w32(0x0013c940 + 0x1c0, coin() ? 0x10u : 0u);
        w32(0x0013c940 + 0x1c4, ri(0, 2) == 0 ? (R() & 0xf000) : 0u);
        w32(OV(0x0017e504), u(ri(0, 3) != 0));
        w32(0x00141610, u(ri(0, 4)));
        return e;
    };
    check("gadgebot_cmd_start", 0x00238b18, 20, Ret::None, [gc] { return Args{{gc(true)}, {}}; },
          [](A a) { gadgebot_cmd_start(a.i[0]); return uint64_t{0}; });
    check("gadgebot_cmd_update", 0x00238b80, 300, Ret::None, [gc] { return Args{{gc(false)}, {}}; },
          [](A a) { gadgebot_cmd_update(a.i[0]); return uint64_t{0}; });
    check("gadgebot_cmd_draw", 0x00238f88, 150, Ret::Int32, [gc] { return Args{{gc(false)}, {}}; },
          [](A a) { return ri32(gadgebot_cmd_draw(a.i[0])); });
    // the menus' 2D: scissor, quads, scaled text, the text box; the help desk
    check("packet_scissor", 0x002a0c68, 40, Ret::None, [level] { level(); return Args{{u(ri(-50, 600)), u(ri(-50, 600)), u(ri(-50, 500)), u(ri(-50, 500))}, {}}; },
          [](A a) { packet_scissor(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3])); return uint64_t{0}; });
    check("quad_fill", 0x001fa0e0, 40, Ret::None, [level] { level(); return Args{{u(ri(-50, 500)), u(ri(-50, 500)), u(ri(-50, 600)), u(ri(-50, 600)), R()}, {}}; },
          [](A a) { quad_fill(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<uint64_t>(sx(a.i[4]))); return uint64_t{0}; });
    check("box_rounded", 0x001fad58, 30, Ret::None, [level] { level(); return Args{{u(ri(0, 400)), u(ri(0, 400)), u(ri(0, 500)), u(ri(0, 500)), u(ri(0, 0x80))}, {}}; },
          [](A a) { box_rounded(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), a.i[4]); return uint64_t{0}; });
    check("glyph_sprite_f", 0x001fa648, 60, Ret::None, [level] {
        level();
        return Args{{u(ri(0, 255)), u(ri(0, 255)), u(ri(0, 32)), u(ri(0, 32)), R(), R()}, {rf(-100, 700), rf(-100, 600), rf(0, 40), rf(0, 40)}};
    }, [](A a) { glyph_sprite_f(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<uint64_t>(sx(a.i[4])), static_cast<uint64_t>(sx(a.i[5])), a.f[0], a.f[1], a.f[2], a.f[3]); return uint64_t{0}; });
    check("text_draw_f", 0x001fb4f0, 60, Ret::None, [texts] {
        texts();
        return Args{{R(), OUT, coin() ? 0xffffffffu : u(ri(0, 12)), R(), coin() ? OV(0x001c34f0u) : OV(0x001c3150u)}, {rf(0, 500), rf(0, 400), rf(0.5f, 2.0f)}};
    }, [](A a) { text_draw_f(static_cast<uint64_t>(sx(a.i[0])), a.i[1], static_cast<int32_t>(a.i[2]), static_cast<uint64_t>(sx(a.i[3])), a.i[4], a.f[0], a.f[1], a.f[2]); return uint64_t{0}; });
    check("text_box_set", 0x001fc520, 20, Ret::None, [level] {
        level();
        return Args{{OUT + 0x200, R(), R(), R(), R(), R(), R(), R(), R()}, {}};
    }, [](A a) { text_box_set(a.i[0], static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]), static_cast<int32_t>(a.i[5]), static_cast<int32_t>(a.i[6]), static_cast<int32_t>(a.i[7]), a.i[8]); return uint64_t{0}; });
    // a long text (words, colour codes, line breaks 1) for the box
    auto words = [](uint32_t a) {
        const int n = ri(0, 3) == 0 ? ri(0, 20) : ri(40, 150);
        for (int i = 0; i < n; ++i) {
            const int k = ri(0, 19);
            w8(a + u(i), k < 14 ? u(ri(0x41, 0x7a)) : k < 17 ? 0x20u : k == 17 ? u(ri(8, 15)) : k == 18 ? 1u : u(ri(0x80, 0xa7)));
        }
        w8(a + u(n), 0);
    };
    auto boxed = [texts, words] {
        texts(); words(OUT);
        const uint32_t B = OUT + 0x200;
        const int32_t l = ri(0, 100), r = ri(l + 160, 600), top = ri(0, 300);  // (not over 32 lines: the original's arrays)
        text_box_set(B, top, top + ri(0, 200), l, r, ri(l + 80, r - 80), ri(top - 20, top + 100), ri(8, 20), R() & 0xf);
        w16(B + 0x14, R()); w16(B + 0x16, R());
        return B;
    };
    check("text_box", 0x001fbf48, 100, Ret::None, [boxed] {
        const uint32_t B = boxed();
        return Args{{B, R(), OUT, coin() ? 0xffffffffu : u(ri(0, 60)), R(), coin() ? OV(0x001c34f0u) : OV(0x001c3150u)}, {}};
    }, [](A a) { text_box(a.i[0], static_cast<uint64_t>(sx(a.i[1])), a.i[2], static_cast<int32_t>(a.i[3]), static_cast<uint64_t>(sx(a.i[4])), a.i[5]); return uint64_t{0}; });
    check("text_box_draw", 0x001fc4a8, 60, Ret::None, [boxed] {
        const uint32_t B = boxed();
        return Args{{B, R(), OUT, coin() ? 0xffffffffu : u(ri(0, 60))}, {}};
    }, [](A a) { text_box_draw(a.i[0], static_cast<uint64_t>(sx(a.i[1])), a.i[2], static_cast<int32_t>(a.i[3])); return uint64_t{0}; });
    check("hud_message_text_set", 0x00237718, 20, Ret::None, [level, words] { level(); words(OUT); return Args{{OUT}, {}}; },
          [](A a) { hud_message_text_set(a.i[0]); return uint64_t{0}; });
    auto msgs = [level] {
        level();
        w32(OV(0x0015f594), u(ri(0, 3) == 0 ? ri(1, 6) : 0));
    };
    check("hud_message_request", 0x002643c8, 40, Ret::Int32, [msgs] { msgs(); return Args{{u(ri(1, 6)), coin() ? 0u : u(ri(1, 300))}, {}}; },
          [](A a) { return ri32(hud_message_request(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]))); });
    check("hud_message_force", 0x00264470, 40, Ret::Int32, [msgs] { msgs(); return Args{{u(ri(1, 6)), coin() ? 0u : u(ri(1, 300))}, {}}; },
          [](A a) { return ri32(hud_message_force(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]))); });
    // the help desk's state (a level's messages table 0x15f660 is set by the level setup)
    auto helpst = [texts](bool any) {
        texts();
        static constexpr OvConst<0x00179410u> HD{};
        const int nmsg = 4;
        const uint32_t T = OUT + 0x400;
        for (int i = 0; i < nmsg; ++i) {
            const uint32_t s = OUT + 0x600 + u(i) * 0x100;
            const int n = ri(10, 150);
            for (int k = 0; k < n; ++k) w8(s + u(k), ri(0, 6) == 0 ? 0x20u : u(ri(0x41, 0x7a)));
            w8(s + u(n), 0);
            w32(T + u(i) * 16, s); w32(T + u(i) * 16 + 8, ri(0, 2) == 0 ? 0xffffffffu : u(ri(0, 20)));
        }
        w32(OV(0x0015f660), T);
        w32(HD, any ? u(ri(0, 9)) : u(ri(0, 8)));
        w32(HD + 4, u(ri(0, 12)));
        w32(HD + 8, u(ri(0x20, 0x100))); w32(HD + 0xc, u(ri(0x20, 0x60)));
        w32(HD + 0x10, 0x100); w32(HD + 0x14, u(ri(0x100, 0x180)));
        w32(HD + 0x18, u(ri(8, 0x100))); w32(HD + 0x1c, u(ri(8, 0x60)));
        w32(HD + 0x20, ri(0, 5) == 0 ? 0xffffffffu : u(ri(0, nmsg - 1)));
        w32(HD + 0x24, ri(0, 2) == 0 ? 0xffffffffu : u(ri(0, 40)));
        w32(HD + 0x28, u(ri(0, 20)));
        w32(HD + 0x30, u(ri(0, 4) != 0));
        w32(HD + 0x34, u(ri(0, 3) == 0 ? ri(0, 200) : 0));
        w16(HD + 0x38, u(ri(0, 3) == 0)); w16(HD + 0x3a, u(ri(0, 2))); w32(HD + 0x3c, u(ri(-1, 3)));
        w32(OV(0x0015f5c4), ri(0, 6) == 0 ? 3u : 0u);
        w32(0x0013cae0, ri(0, 3) == 0 ? (R() & 0xf000) : 0u);
        w32(0x0013cae4, ri(0, 4) == 0 ? 0x10u : 0u);
        w8(0x0015ee1d, u(ri(0, 1))); w8(0x0015ee1c, u(ri(0, 1)));
        constexpr uint32_t ST = 0x001516d0;
        w32(ST + 0x50, ri(0, 2) == 0 ? R() : 0u); w32(ST + 0x1c, coin() ? 0xffffffffu : R());
        w16(ST + 0x5a, u(ri(0, 7))); w16(ST + 0x54, u(0x7530 + ri(-1, 20)));
        for (uint32_t i = 0; i < 4; ++i) w32(0x00141968 + i * 8, R() & 0xffff00ff);
    };
    check("dialog_stream_stop", 0x002652e8, 30, Ret::Int32, [helpst] { helpst(true); return Args{}; }, [](A) { return ri32(dialog_stream_stop()); });
    check("help_show", 0x00203b88, 40, Ret::None, [helpst] { helpst(true); w32(OV(0x00179410) + 0x20, u(ri(0, 3))); return Args{}; }, [](A) { help_show(); return uint64_t{0}; });
    check("help_update", 0x00203cc0, 400, Ret::None, [helpst] { helpst(true); return Args{}; }, [](A) { help_update(); return uint64_t{0}; });
    check("help_icon_draw", 0x002046c8, 20, Ret::None, [helpst] { helpst(true); return Args{}; }, [](A) { help_icon_draw(); return uint64_t{0}; });
    check("help_draw", 0x002047b0, 200, Ret::None, [helpst] { helpst(true); w32(OV(0x00179410) + 0x20, u(ri(0, 3))); return Args{}; }, [](A) { help_draw(); return uint64_t{0}; });
    check("packet_ref", 0x002a0a58, 20, Ret::None, [level] { level(); return Args{{R() & 0x01fffff0u, R() & 0xffff}, {}}; },
          [](A a) { packet_ref(a.i[0], a.i[1]); return uint64_t{0}; });
    check("packet_mark", 0x002a13a0, 20, Ret::None, [level] { level(); return Args{{R()}, {}}; }, [](A a) { packet_mark(a.i[0]); return uint64_t{0}; });
    check("texture_uploads", 0x0024d760, 30, Ret::None, [level, frame] {
        level();
        for (int i = 0; i < 3; ++i) (void)hud_tex0(frame());
        return Args{};
    }, [](A) { texture_uploads(); return uint64_t{0}; });
    check("texture_block_begin", 0x001f9060, 20, Ret::None, [level] { level(); return Args{{u(ri(0, 1))}, {}}; },
          [](A a) { texture_block_begin(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    check("texture_block_end", 0x001f9178, 20, Ret::None, [level] {
        level();
        texture_block_begin(ri(0, 1));  // the block's reserved tag
        w32(OV(0x0016a01c), u(ri(0, 1)));
        return Args{};
    }, [](A) { texture_block_end(); return uint64_t{0}; });
    check("screen_bars", 0x001f9bd8, 20, Ret::None, [level] {
        level();
        w32(OV(0x0015f5c4), u(ri(0, 6)));
        return Args{};
    }, [](A) { screen_bars(); return uint64_t{0}; });
    g_extra_noop_hooks.clear();
}

}  // namespace rac1::oracle
