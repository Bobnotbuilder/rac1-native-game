// Oracle batch 90 (task 0309): the memory card's machine 0x24b690 with all its operations
// (states 3..0x17), the save's helpers (0x24d118, 0x24ce68, 0x24b410, 0x24cf10) and the full-screen
// menu's save slot widgets (0x280bd0, 0x280f20, 0x281200, 0x281628, 0x2809e0) with the saves they
// start (0x284808, 0x2848b8, 0x284950) and 0x1fba40. libmc is the host's stand-in on both sides
// (a virtual card in memory, put back before the port's run; a hash of the calls made and of the
// data written kept in an EE word, so the card's traffic is compared too). The machine's cases:
// an operation asked for on a card in one of several states, the port run a random number of
// frames to reach a step of it, then that step run by both.

#include "ovl_oracle.h"

#include <game/lib/libcdvd.h>
#include <game/lib/libmc.h>
#include <game/lib/movie_player.h>
#include <game/ovl/level_change.h>
#include <game/ovl/memory_card.h>
#include <game/ovl/memory_card_ops.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/save_menus.h>
#include <runtime/ipu/movie_hw.h>
#include <runtime/kernel/disc.h>

namespace rac1::oracle {

namespace {
constexpr uint32_t S = 0x0013d290;
constexpr uint32_t TRACE = T2 + 0xff0;          // the card calls' hash
constexpr uint32_t IMG = TEST_AREA + 0x40000;   // a save's image (up to 0x1800 + 20 * 0x1000 bytes)
constexpr uint32_t PAGE = T + 0x800;            // a page of the menu (+0x38 parent, +0x40 the focused widget)

void hooks_card() {  // orig: (none) libmc's calls run as the host's stand-ins
    auto i = [](EeCpu& c, int r) { return static_cast<int32_t>(c.r32(r)); };
    g_extra_hooks.push_back({0x001233f0, [](EeCpu& c) { c.set32(2, mc_init()); }});
    g_extra_hooks.push_back({0x00123d68, [i](EeCpu& c) { c.set32(2, mc_get_info(i(c, 4), i(c, 5), c.r32(6), c.r32(7), c.r32(8))); }});
    g_extra_hooks.push_back({0x001235b0, [i](EeCpu& c) { c.set32(2, mc_open(i(c, 4), i(c, 5), c.r32(6), i(c, 7))); }});
    g_extra_hooks.push_back({0x001236d8, [i](EeCpu& c) { c.set32(2, mc_mkdir(i(c, 4), i(c, 5), c.r32(6))); }});
    g_extra_hooks.push_back({0x00123710, [i](EeCpu& c) { c.set32(2, mc_close(i(c, 4))); }});
    g_extra_hooks.push_back({0x001237c8, [i](EeCpu& c) { c.set32(2, mc_seek(i(c, 4), i(c, 5), i(c, 6))); }});
    g_extra_hooks.push_back({0x00123930, [i](EeCpu& c) { c.set32(2, mc_read(i(c, 4), c.r32(5), i(c, 6))); }});
    g_extra_hooks.push_back({0x00123a48, [i](EeCpu& c) { c.set32(2, mc_write(i(c, 4), c.r32(5), i(c, 6))); }});
    g_extra_hooks.push_back({0x00123ef0, [i](EeCpu& c) { c.set32(2, mc_get_dir(i(c, 4), i(c, 5), c.r32(6), i(c, 7), i(c, 8), c.r32(9))); }});
    g_extra_hooks.push_back({0x001240c0, [i](EeCpu& c) { c.set32(2, mc_chdir(i(c, 4), i(c, 5), c.r32(6), c.r32(7))); }});
    g_extra_hooks.push_back({0x00124200, [i](EeCpu& c) { c.set32(2, mc_format(i(c, 4), i(c, 5))); }});
    g_extra_hooks.push_back({0x001242d0, [i](EeCpu& c) { c.set32(2, mc_delete(i(c, 4), i(c, 5), c.r32(6))); }});
    g_extra_hooks.push_back({0x001243e8, [i](EeCpu& c) { c.set32(2, mc_unformat(i(c, 4), i(c, 5))); }});
    g_extra_hooks.push_back({0x00123c30, [i](EeCpu& c) { c.set32(2, mc_sync(i(c, 4), c.r32(5), c.r32(6))); }});
}
}  // namespace

void batch_90() {
    if (g_only_batch != 90) return;
    g_batch = 90;
    g_ignored_words = vcross_lanes();
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    movie_hw_enable(false);
    g_host_clock_fixed = 1759658400;
    g_extra_hooks = {{0x00121780, [](EeCpu& c) { c.set32(2, cd_read_clock(c.r32(4))); }},
                     {0x0012d6d8, [](EeCpu& c) { clock_to_local(c.r32(4)); }},
                     {0x00266040, [](EeCpu& c) { c.set32(2, disc_read_wait(c.r32(4), c.r32(5), c.r32(6))); }},
                     {0x00121450, [](EeCpu& c) { c.set32(2, cd_read(c.r32(4), c.r32(5), c.r32(6), c.r32(7))); }},
                     {0x00121630, [](EeCpu& c) { c.set32(2, cd_get_error()); }},
                     {0x001216c8, [](EeCpu& c) { c.set32(2, cd_break()); }},
                     {0x00267898, [](EeCpu&) { pad_update(); }}};
    hooks_card();
    g_extra_noop_hooks = {0x00122298, 0x001188e0, 0x001188a0, 0x00122e68, 0x002a08c8, 0x001f8540, 0x0023af30,
                          0x001187d0, 0x001190f8, 0x00243d38, 0x002900a0};
    // 989snd's IOP server (batch 85's): the stream system's disc reads (command 0x38) done
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
    // Veldin's level with the display, the stream system and the save's names (SYSTEM.CNF) set up
    static std::vector<uint8_t> snap;
    static const auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            level_setup_body(1, 0);
            ee_write<uint16_t>(0x001518d0, 512);
            ee_write<uint16_t>(0x001518d2, 416);
            particle_table_init();
            hud_init();
            w32(0x0016c060, 0xf);
            w32(0x0015ec8c, 1);
            w32(0x00137b00, 0);
            sub_00264bf8();
            sub_002a07f8();
            if (disc_read_ee(OUT, 0x121, 1)) save_names_init(OUT);
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    auto pick = [](std::initializer_list<uint32_t> l) { return *(l.begin() + ri(0, static_cast<int>(l.size()) - 1)); };

    // A save's image of the game's state (as 0x24ce68 builds it) and its size.
    static std::vector<uint8_t> image;
    auto image_make = [] {
        save_blocks_write(IMG);
        const uint32_t n = 8 + save_list_size(0x001845c0) + save_list_size(0x001848c0) * 0x14;
        image.resize(n);
        for (uint32_t k = 0; k < n; ++k) image[k] = ee_read<uint8_t>(IMG + k);
    };
    const std::string dir = [] {
        std::string d;
        level();
        for (uint32_t k = 1; k < 0x20; ++k) {
            const char c = static_cast<char>(ee_read<uint8_t>(0x0013d1d0 + k));
            if (c == 0) break;
            d.push_back(c);
        }
        return d;
    }();
    std::printf("batch 90: the save's directory \"%s\"\n", dir.c_str());

    // The card: none, unformatted, or a PS2 card with (or without) the save's directory and its
    // files: save0..4.bin a sound image, a damaged one, a short one, or none.
    auto card_make = [pick, dir, image_make] {
        mc_reset();
        mc_set_trace(0);
        const int kind = ri(0, 9);
        if (kind == 0) return;
        mc_card_insert("");
        if (kind == 1) {
            mc_unformat(0, 0);
            mc_sync(1, 0, 0);
            return;
        }
        if (ri(0, 3) == 0) return;
        image_make();
        mc_card_put(dir + "/icon.sys", std::vector<uint8_t>(0x3c4, 7));
        if (coin()) mc_card_put(dir + "/static.ico", std::vector<uint8_t>(0x900, 3));
        for (int k = 0; k < 5; ++k) {
            std::vector<uint8_t> d = image;
            switch (ri(0, 5)) {
                case 0: continue;
                case 1: d.resize(static_cast<size_t>(ri(0, static_cast<int>(d.size()) - 1))); break;
                case 2: d[static_cast<size_t>(ri(8, static_cast<int>(d.size()) - 1))] ^= 0x5a; break;
                case 3: {  // a block size smaller (a larger one beyond 0x1800 / 0x1000 halts the original: 0x1ff008)
                    const size_t o = coin() ? 0 : 4;
                    uint32_t v;
                    std::memcpy(&v, &d[o], 4);
                    v -= static_cast<uint32_t>(ri(1, 8)) * 4;
                    std::memcpy(&d[o], &v, 4);
                    break;
                }
                default: break;
            }
            mc_card_put(dir + "/save" + std::to_string(k) + ".bin", d);
        }
        (void)pick;
    };

    // ---- the card's machine ----
    // an operation asked for (state 2 starts it), the port run k frames into it
    static uint32_t op_now = 0;
    auto scenario = [pick, card_make, image_make] {
        level();
        card_make();
        image_make();  // (the image in EE RAM for operation 0x13)
        w32(S + 0xd4, 2);
        w32(S + 0xcc, 0);
        w32(S + 0xc4, 0);
        w32(S + 0xdc, op_now);
        w32(S + 0xe0, ri(0, 7) == 0 ? 0xffffffffu : 0u);
        w32(S + 0x1c, ri(0, 7) == 0 ? u(-10) : 0u);
        w32(S + 0, 0);
        w32(S + 4, 0);
        w32(S + 8, 2);
        w32(S + 0xc, pick({0x100u, 0x1f40u, 0x1f40u}));
        w32(S + 0x10, u(ri(0, 3) != 0));
        w32(S + 0x14, u(ri(-2, 4)));
        w32(S + 0x18, u(ri(-1, 4)));
        w32(S + 0xac, ri(0, 3) == 0 ? 1u : 0u);
        w32(S + 0xc8, u(ri(0, 3) == 0 ? ri(0, 19) : 0));
        w32(S + 0xec, IMG);
        w32(S + 0xe4, 0);
        w16(0x001516d8, 0);
        // the operation's length (frames until the machine is idle again), then k frames of it
        static std::vector<uint8_t> ram0;
        uint8_t* ram = ee_get_ram_base();
        ram0.assign(ram, ram + 0x02000000);
        mc_snapshot_take();
        const uint32_t seed = R();
        std::mt19937 clear(seed);
        int len = 0;
        bool started = false;
        for (; len < 600; ++len) {
            if (lw(S + 0xd4) > 2) started = true;
            if (started && lw(S + 0xd4) <= 2 && lw(S + 0xcc) == 0) break;
            if (!started && len > 3) break;
            card_machine();
            if ((clear() & 1) != 0) w16(0x001516d8, 0);
        }
        std::memcpy(ram, ram0.data(), ram0.size());
        mc_snapshot_restore();
        clear.seed(seed);
        const int k = ri(0, len);
        for (int f = 0; f < k; ++f) {
            card_machine();
            if ((clear() & 1) != 0) w16(0x001516d8, 0);
        }
        if (ri(0, 3) == 0) w16(0x001516d8, 0);
        w32(TRACE, 0);
        mc_set_trace(TRACE);
        mc_snapshot_take();
    };
    for (uint32_t op : {3u, 5u, 7u, 9u, 0xbu, 0xdu, 0xfu, 0x11u, 0x13u, 0x15u}) {
        op_now = op;
        char name[40];
        std::snprintf(name, sizeof name, "card_machine_op%x", op);
        check(name, 0x0024b690, op == 3 || op == 5 || op == 0x11 ? 40 : 150, Ret::None, [scenario] {
            scenario();
            return Args{};
        }, [](A) {
            mc_snapshot_restore();
            card_machine();
            return uint64_t{0};
        });
    }
    // any state and step, the answers at random (an open file on the card)
    check("card_machine_any", 0x0024b690, 200, Ret::None, [pick, card_make] {
        level();
        card_make();
        int32_t fd = -1;
        if (mc_card_present() && coin()) {
            const uint32_t nm = OUT + 0x100;
            std::string f = std::string("/") + "x" + std::to_string(ri(0, 1));
            for (size_t k = 0; k <= f.size(); ++k) w8(nm + static_cast<uint32_t>(k), k < f.size() ? static_cast<uint8_t>(f[k]) : 0);
            mc_open(0, 0, nm, 0x203);
            mc_sync(1, OUT + 0x200, OUT + 0x204);
            fd = lw(OUT + 0x204);
        }
        if (coin()) mc_set_pending(true, ri(1, 0x11), static_cast<int32_t>(pick({0u, u(-4), 8u, 0x1000u})));
        w32(S + 0xcc, u(ri(0, 3) == 0));
        w32(S + 0xd4, u(ri(3, 0x18)));
        w32(S + 0xd8, u(ri(0, 0x16)));
        w32(S + 0xc4, 0);
        w32(S + 0xbc, pick({0u, 1u, 8u, u(-1), u(-2), u(-3), u(-4), u(-5), u(-6), u(-7), u(ri(0, 0x2000))}));
        w32(S + 0xf0, pick({8u, 0x3c4u, u(ri(0, 0x1800))}));
        w32(S + 0xd0, fd >= 0 ? u(fd) : u(ri(-1, 3)));
        w32(S + 0x10, u(ri(0, 1)));
        w32(S + 0x14, u(ri(-2, 4)));
        w32(S + 0x18, u(ri(-1, 5)));
        w32(S + 0xb0, pick({save_list_size(0x001845c0), u(ri(0, 0x1800))}));
        w32(S + 0xb4, pick({save_list_size(0x001848c0), u(ri(0, 0x1000))}));
        w32(S + 0xc, pick({0x100u, 0x1f40u}));
        w32(S + 0xac, u(ri(0, 1)));
        w32(S + 0xc8, u(ri(0, 20)));
        w32(S + 0xec, IMG);
        w16(0x001516d8, u(ri(0, 1)));
        w32(TRACE, 0);
        mc_set_trace(TRACE);
        mc_snapshot_take();
        return Args{};
    }, [](A) {
        mc_snapshot_restore();
        card_machine();
        return uint64_t{0};
    });

    // a load's / save's level blocks one by one (state 0xe step 7: read, the next level; state 0x10
    // step 6: the seek to the level's block)
    check("card_machine_levels", 0x0024b690, 60, Ret::None, [pick] {
        level();
        mc_reset();
        mc_card_insert("");
        const int32_t lv = ri(0, 19);
        const uint32_t size = save_list_size(0x001848c0);
        save_block_write(0x001506d0, lv, 0x001848c0);
        const bool load = coin();
        w32(S + 0xcc, 0);
        w32(S + 0xd4, load ? 0xeu : 0x10u);
        w32(S + 0xd8, load ? 7u : 6u);
        w32(S + 0xc4, 0);
        w32(S + 0xc8, u(lv));
        w32(S + 0xf0, size);
        w32(S + 0xbc, ri(0, 3) != 0 ? size : pick({0u, u(-4), u(-2)}));
        w32(S + 0xd0, 0);
        w32(S + 0xac, u(ri(0, 1)));
        w32(TRACE, 0);
        mc_set_trace(TRACE);
        mc_snapshot_take();
        return Args{};
    }, [](A) {
        mc_snapshot_restore();
        card_machine();
        return uint64_t{0};
    });

    // ---- the save's helpers ----
    check("save_slot_summary", 0x0024d118, 30, Ret::None, [] {
        level();
        save_block_write(T3, 0, 0x001845c0);
        if (ri(0, 2) == 0) w32(T3 + 8 + u(ri(0, 8)) * 4, R());
        return Args{{T3, u(ri(0, 1)), u(ri(0, 4))}};
    }, [](A a) {
        save_slot_summary(a.i[0], static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]));
        return uint64_t{0};
    });
    check("save_blocks_write", 0x0024ce68, 4, Ret::None, [] {
        level();
        w32(0x0015ed84, u(ri(0, 17)));
        return Args{{IMG}};
    }, [](A a) {
        save_blocks_write(a.i[0]);
        return uint64_t{0};
    });
    check("save_names_init", 0x0024b410, 10, Ret::None, [] {
        level();
        const char* cnf[] = {"BOOT2 = cdrom0:\\SCUS_971.99;1", "BOOT2 = cdrom0:\\SCES_503.86;1", "BOOT2 = cdrom0:\\SLUS_123.45;1"};
        const char* s = cnf[ri(0, 2)];
        for (uint32_t k = 0; k < 0x40; ++k) w8(OUT + k, k < std::strlen(s) ? static_cast<uint8_t>(s[k]) : 0);
        return Args{{OUT}};
    }, [](A a) {
        save_names_init(a.i[0]);
        return uint64_t{0};
    });
    check("card_lib_init", 0x0024cf10, 2, Ret::None, [] {
        level();
        return Args{};
    }, [](A) {
        card_lib_init();
        return uint64_t{0};
    });
    check("text_centered_med", 0x001fba40, 20, Ret::Int32, [pick] {
        level();
        return Args{{u(ri(0, 0x200)), u(ri(0, 0x1a0)), R(), static_cast<uint32_t>(sub_00203ad8(static_cast<int32_t>(pick({0x5217u, 0x4fb5u, 0x4f2au})))), coin() ? 0xffffffffu : u(ri(0, 6))}};
    }, [](A a) {
        return ri32(text_centered_med(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(a.i[2]))), a.i[3],
                                      static_cast<int32_t>(a.i[4])));
    });

    // ---- the menu's save slot widgets ----
    constexpr uint32_t MS = 0x001b9cf0;
    // the menu on a page whose focused widget is `w`, the card idle or busy, the slots' summaries
    auto menu_state = [pick](uint32_t w) {
        mc_reset();
        w32(MS + 4, PAGE);
        w32(PAGE + 0x40, ri(0, 7) == 0 ? T : w);
        w32(PAGE + 0x38, coin() ? 0u : PAGE + 0x100);
        w32(MS + 0x124, u(ri(0, 1)));
        w32(MS + 0x128, u(ri(0, 2) == 0));
        w32(MS + 0xd0, pick({0x001b6678u, 0x001b8f38u, 0x001b9098u, PAGE}));
        w32(MS + 0xe0, IMG);
        w32(0x001b6678 + 0x84, u(ri(0, 1)));
        w32(0x001b8f38 + 0x84, u(ri(0, 1)));
        w32(0x001b9098 + 0x84, u(ri(0, 1)));
        w32(w + 0x30, pick({0u, 1u, 0x2000u, 0x2001u}));
        w32(w + 0x40, u(ri(0, 4)));
        w32(w + 0x48, IMG);
        w32(w + 0x4c, pick({0u, 0u, 1u, 2u}));
        w32(w + 0x20, u(ri(0x80, 0x200)));
        w32(w + 0x24, u(ri(0x40, 0x100)));
        const uint32_t keys[] = {0u, 0x40u, 0x10u, 0x20u, 0x1000u, 0x4000u, 0x100u, 0x50u};
        w32(0x0013c940 + 0x1c4, keys[ri(0, 7)]);
        w32(0x0013c940 + 0x1a4, keys[ri(0, 7)]);
        w32(0x0013c940 + 0x1b4, keys[ri(0, 7)]);
        w32(0x0015eeb0, pick({0x10u, 1u, 0x10u, 3u}));
        w32(0x0015eeb4, R() & 0x3ff);
        w32(0x0015ee34, u(ri(0, 4)));
        w32(S + 0xd4, pick({0u, 2u, 2u, 3u}));
        w32(S + 0xdc, ri(0, 3) == 0 ? u(ri(0, 0x15)) : 0xffffffffu);
        w32(S + 8, ri(0, 5) == 0 ? 0u : 2u);
        w32(S + 0xe4, ri(0, 3) == 0 ? u(ri(1, 0x2718)) : 0u);
        w32(S + 0x14, u(ri(0, 4)));
        for (int32_t k = 0; k < 5; ++k) {
            const uint32_t r = S + u(k) * 0x1c;
            w32(r + 0x20, ri(0, 2) == 0 ? 0xffffffffu : u(ri(0, 17)));
            w32(r + 0x24, pick({u(ri(0, 999)), u(ri(1000, 999999)), u(ri(1000000, 20000000))}));
            w32(r + 0x28, u(ri(0, 3) == 0 ? ri(1, 120) : 0));
            w32(r + 0x2c, u(ri(0, 216000 * 120)));
            for (uint32_t b = 0; b < 8; ++b) w8(r + 0x30 + b, R());
        }
        w32(0x0015ed80, u(ri(0, 1)));
        w32(0x0015ed84, u(ri(0, 17)));
        w32(0x0015ed98, R() & 0xfffff);
        w32(0x0015ee20, u(ri(0, 3)));
        w32(0x0015ee24, R() & 0xffffff);
        w32(0x0015edf0, u(ri(0, 0x400)));
        w32(0x0015edec, u(ri(0, 0x400)));
    };
    check("save_slots_update", 0x00280bd0, 120, Ret::Int32, [menu_state] {
        level();
        menu_state(0x001b58e0);
        return Args{{0x001b58e0}};
    }, [](A a) { return ri32(save_slots_update(a.i[0])); });
    check("load_slots_update", 0x00280f20, 120, Ret::Int32, [menu_state] {
        level();
        menu_state(0x001b5930);
        return Args{{0x001b5930}};
    }, [](A a) { return ri32(load_slots_update(a.i[0])); });
    check("new_game_slots_update", 0x00281200, 120, Ret::Int32, [menu_state, pick] {
        level();
        const uint32_t w = pick({0x001b8e68u, 0x001b9048u});
        menu_state(w);
        return Args{{w}};
    }, [](A a) { return ri32(new_game_slots_update(a.i[0])); });
    check("save_slots_draw", 0x00281628, 60, Ret::Int32, [menu_state] {
        level();
        menu_state(0x001b58e0);
        return Args{{0x001b58e0}};
    }, [](A a) { return ri32(save_slots_draw(a.i[0])); });
    check("save_slot_planet_draw", 0x002809e0, 40, Ret::Int32, [menu_state] {
        level();
        menu_state(0x001b5888);
        w32(PAGE + 0x40, 0x001b58e0);
        return Args{{0x001b5888}};
    }, [](A a) { return ri32(save_slot_planet_draw(a.i[0])); });
    check("save_slots_focus_in", 0x00280b60, 20, Ret::Int32, [menu_state] {
        level();
        menu_state(0x001b58e0);
        for (uint32_t k = 0; k < 5; ++k) w32(0x001ba1b8 + k * 8 + 4, R() & 3);
        return Args{{0x001b58e0}};
    }, [](A a) { return ri32(save_slots_focus_in(a.i[0])); });
    check("save_slots_focus_out", 0x00280ba0, 20, Ret::Int32, [menu_state] {
        level();
        menu_state(0x001b58e0);
        for (uint32_t k = 0; k < 5; ++k) w32(0x001ba1b8 + k * 8 + 4, R() & 3);
        w32(0x001b58e0 + 0x48, coin() ? lwu(0x001ba1b8 + u(ri(0, 4)) * 8) : R());
        return Args{{0x001b58e0}};
    }, [](A a) { return ri32(save_slots_focus_out(a.i[0])); });
    check("question_focus_in", 0x0027f690, 10, Ret::Int32, [menu_state] {
        level();
        menu_state(0x001b6750);
        for (uint32_t k = 0; k < 5; ++k) w32(0x001ba1b8 + k * 8 + 4, R() & 3);
        return Args{{0x001b6750}};
    }, [](A a) { return ri32(question_focus_in(a.i[0])); });
    check("question_focus_out", 0x0027f6d0, 10, Ret::Int32, [menu_state] {
        level();
        menu_state(0x001b6750);
        for (uint32_t k = 0; k < 5; ++k) w32(0x001ba1b8 + k * 8 + 4, R() & 3);
        w32(0x001b6750 + 0x54, coin() ? lwu(0x001ba1b8 + u(ri(0, 4)) * 8) : R());
        return Args{{0x001b6750}};
    }, [](A a) { return ri32(question_focus_out(a.i[0])); });
    check("question_update", 0x0027f700, 60, Ret::Int32, [menu_state] {
        level();
        menu_state(0x001b6750);
        w32(PAGE + 0x84, u(ri(0, 1)));
        return Args{{0x001b6750}};
    }, [](A a) { return ri32(question_update(a.i[0])); });
    check("question_draw", 0x0027f798, 30, Ret::Int32, [menu_state, pick] {
        level();
        menu_state(0x001b6750);
        w32(MS + 0xd4, pick({0u, 1u, 2u, 3u, 4u, u(-1)}));
        return Args{{0x001b6750}};
    }, [](A a) { return ri32(question_draw(a.i[0])); });
    check("save_game_image", 0x00284808, 6, Ret::None, [menu_state] {
        level();
        menu_state(0x001b58e0);
        return Args{{IMG, u(ri(0, 4))}};
    }, [](A a) {
        save_game_image(a.i[0], static_cast<int32_t>(a.i[1]));
        return uint64_t{0};
    });
    check("new_game_image", 0x002848b8, 4, Ret::None, [menu_state] {
        level();
        menu_state(0x001b8e68);
        return Args{{IMG, u(ri(0, 4))}};
    }, [](A a) {
        new_game_image(a.i[0], static_cast<int32_t>(a.i[1]));
        return uint64_t{0};
    });
    check("challenge_game_image", 0x00284950, 6, Ret::None, [menu_state] {
        level();
        menu_state(0x001b9048);
        for (uint32_t k = 0; k < 0x25; ++k) w8(0x0013d4c0 + k, u(ri(0, 1)));
        w8(0x0013d388 + 4, u(ri(0, 1)));
        w8(0x0013d388 + 5, u(ri(0, 1)));
        return Args{{u(ri(-1, 4))}};
    }, [](A a) {
        challenge_game_image(static_cast<int32_t>(a.i[0]));
        return uint64_t{0};
    });

    movie_hw_enable(true);
    mc_reset();
    mc_set_trace(0);
    g_host_clock_fixed = 0;
    g_extra_noop_hooks.clear();
    g_extra_hooks.clear();
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
