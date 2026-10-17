// Oracle batch 84 (task 0300): the level change. The request 0x24d430 (the clock, the level's
// talkers, the planet map's revealed area 0x249ea0 with its run coder 0x200418 / reduction
// 0x24a3c8, the save slot's record and blocks 0x24d030 with their checksum 0x24cf78, the card's
// save asked for) on Veldin's real level state (needs the ISO); the level exit 0x292e08's ported
// parts.

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
#include <game/lib/libcdvd.h>
#include <game/lib/libmc.h>
#include <game/lib/movie_player.h>
#include <game/ovl/level_change.h>
#include <game/ovl/movies.h>
#include <runtime/ipu/movie_hw.h>
#include <runtime/kernel/disc.h>

namespace rac1::oracle {

void batch_84() {
    if (g_only_batch != 84) return;
    g_batch = 84;
    g_ignored_words = vcross_lanes();
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    // the console's clock: the host's stand-ins on both sides, at a fixed time
    g_host_clock_fixed = 1759658400;  // 2025-10-05 10:00 UTC
    g_extra_hooks = {{0x00121780, [](EeCpu& c) { c.set32(2, cd_read_clock(c.r32(4))); }},
                     {0x0012d6d8, [](EeCpu& c) { clock_to_local(c.r32(4)); }}};
    static std::vector<uint8_t> snap;
    static const auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            level_setup_body(1, 0);
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    // a 512 x 512 bitmap (0x8000 bytes), the spans, the coder's output above them (a run past the
    // output's end can carry the coder past it: upwards, away from its inputs)
    static const uint32_t BITS = TEST_AREA, SPANS = TEST_AREA + 0x8000, DST = TEST_AREA + 0xc000;
    // the spans over the bitmap: (skip, count) byte pairs up to bit 0x40000 exactly
    auto spans = [](uint32_t at, int big) {
        uint32_t pos = 0, p = at;
        const uint32_t room = 0x3c00;
        while (p + 2 < at + room) {
            const uint32_t skip = u(ri(0, big ? 255 : 8)), count = u(ri(0, 255));
            if (pos + skip + count + 256 >= 0x40000) break;
            w8(p, skip);
            w8(p + 1, count);
            p += 2;
            pos += skip + count;
        }
        // the rest skipped in steps of at most 255 (count 0), the last skip landing on 0x40000
        while (0x40000 - pos > 255) {
            w8(p, 255);
            w8(p + 1, 0);
            p += 2;
            pos += 255;
        }
        w8(p, 0x40000 - pos);
        w8(p + 1, 0);
    };
    // a bitmap with long runs (the run length limits) or noise
    auto bitmap = [](uint32_t at) {
        const int kind = ri(0, 2);
        uint32_t v = coin() ? 0xffu : 0u;
        for (uint32_t i = 0; i < 0x8000; ++i) {
            if (kind == 0) w8(at + i, R());
            else {
                if (ri(0, kind == 1 ? 40 : 4000) == 0) v ^= 0xffu;
                w8(at + i, ri(0, 30) == 0 ? R() : v);
            }
        }
    };

    check("save_checksum", 0x0024cf78, N, Ret::Int32, [] {
        level();
        const int32_t n = ri(0, 3) == 0 ? ri(0x17f0, 0x1810) : ri(0, 0x400);
        wrand(T, 0x1800 / 4);
        return Args{{T, u(n)}};
    }, [](A a) { return r32(save_checksum(a.i[0], static_cast<int32_t>(a.i[1]))); });

    check("save_block_write", 0x0024d030, N, Ret::Int32, [] {
        level();
        const int n = ri(0, 5);
        for (int k = 0; k < n; ++k) {
            const uint32_t e = T + u(k) * 0x10;
            w32(e, T2 + u(ri(0, 0x100)));
            w32(e + 4, u(ri(1, 0x41)));
            w32(e + 8, R());
            w32(e + 0xc, R());
        }
        w32(T + u(n) * 0x10, 0);
        wrand(T2, 0x800 / 4);
        return Args{{OUT, u(ri(0, 3)), T}};
    }, [](A a) { return ri32(save_block_write(a.i[0], static_cast<int32_t>(a.i[1]), a.i[2])); });

    check("talkers_save", 0x0024ab50, N, Ret::None, [] {
        level();
        w32(0x0015ed84, u(ri(0, 0x14)));
        for (uint32_t i = 0; i < 0x40; ++i) w32(OV(0x001791b8) + i * 4, coin() ? 0u : M + u(ri(0, 4)) * 0x100);
        wfrand(M, 0x400 / 4, -500.0f, 500.0f);
        return Args{};
    }, [](A) { talkers_save(); return uint64_t{0}; });

    check("map_runs_encode", 0x00200418, 24, Ret::Int32, [spans, bitmap] {
        level();
        spans(SPANS, coin());
        bitmap(BITS);
        wrand(DST, 0x800 / 4);
        return Args{{DST, u(ri(0, 2) == 0 ? ri(4, 0x80) : 0x800), SPANS, BITS}};
    }, [](A a) { return ri32(map_runs_encode(a.i[0], static_cast<int32_t>(a.i[1]), a.i[2], a.i[3])); });

    check("map_area_reduce", 0x0024a3c8, 16, Ret::None, [bitmap] {
        level();
        bitmap(BITS);
        w32(OV(0x001841fc), BITS);
        wrand(DST, 0x800 / 4);
        return Args{{DST}};
    }, [](A a) { map_area_reduce(a.i[0]); return uint64_t{0}; });

    check("map_area_save", 0x00249ea0, 30, Ret::None, [spans, bitmap] {
        level();
        w32(0x0015ed84, u(ri(0, 18)));
        w32(OV(0x00184218), u(ri(0, 3)));
        spans(SPANS, coin());
        bitmap(BITS);
        w32(OV(0x001841fc), BITS);
        w32(OV(0x00184204), SPANS);
        w32(0x0013d560 + ee_read<uint32_t>(0x0015ed84) * 4, coin() ? 0xffffffffu : u(ri(0, 0x60)));  // the level's longest code so far
        wrand(DST, 0x800 / 4);
        return Args{{DST}};
    }, [](A a) { map_area_save(a.i[0]); return uint64_t{0}; });

    // the request: no slot / a slot (valid or not), a save pending or not, the card busy or idle
    constexpr uint32_t SAVE = 0x0013d290;
    check("level_change_request", 0x0024d430, N, Ret::Int32, [] {
        level();
        w32(0x0015ed84, u(ri(0, 18)));
        w32(OV(0x00184218), 0);  // the map's area cleared (its coder has its own checks above)
        const int32_t slot = ri(0, 3) == 0 ? -1 : ri(0, 3);
        w32(SAVE + 0xc0, u(slot));
        for (uint32_t s = 0; s < 4; ++s) w32(SAVE + s * 0xb8 + 0x14, ri(0, 3) == 0 ? 0xffffffffu : u(ri(0, 3)));
        w32(SAVE + 0x14, u(ri(0, 3)));
        w32(SAVE + 0xf4, u(ri(0, 1)));
        w32(SAVE + 0xd4, u(ri(0, 4)));
        w32(SAVE + 0xdc, coin() ? 0xffffffffu : u(ri(0, 0x10)));
        w32(0x0015eeb4, R());
        w32(0x0015ed98, R());
        w32(0x0015ee20, R());
        w32(0x0015ee24, R());
        for (uint32_t i = 0; i < 19; ++i) w8(0x0013dd58 + i, u(ri(0, 1)));
        return Args{{u(ri(0, 1)), u(ri(-1, 18))}};
    }, [](A a) { return r32(level_change_request(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1])) ? 1u : 0u); });

    // ---- the level exit: the disc's reads (libcdvd) the host's on both sides, the pad's too
    g_extra_hooks.push_back({0x00121450, [](EeCpu& c) { c.set32(2, cd_read(c.r32(4), c.r32(5), c.r32(6), c.r32(7))); }});
    g_extra_hooks.push_back({0x00121630, [](EeCpu& c) { c.set32(2, cd_get_error()); }});
    g_extra_hooks.push_back({0x001216c8, [](EeCpu& c) { c.set32(2, cd_break()); }});
    g_extra_hooks.push_back({OVF(0x00267898), [](EeCpu&) { pad_update(); }});
    // the movie player (Sony's sample player: threads, IPU) the host's stand-in on both sides, the
    // movies off (task 0301: the story's cards 0x2929f0 and movies 0x292420 are ported)
    movie_hw_enable(false);
    g_extra_hooks.push_back({OVF(0x002edd28), [](EeCpu& c) {
                                 movie_stream_start(c.r32(4), c.r32(5), c.r32(6), c.r32(7), static_cast<int32_t>(c.r32(8)));
                                 while (!movie_stream_frame()) {
                                 }
                                 c.set32(2, 0);
                             }});
    // the callees left TODO_PORT: the kernel's vsync wait, DMA kick and handlers, the card's
    // machine, the state table, the GS display registers, the fade, the vsync callback
    // the memory card's machines (0x24b690 / 0x24ac20, task 0301) run on both sides, libmc the
    // host's stand-in (no card)
    g_extra_hooks.push_back({0x00123d68, [](EeCpu& c) { c.set32(2, mc_get_info(static_cast<int32_t>(c.r32(4)), static_cast<int32_t>(c.r32(5)), c.r32(6), c.r32(7), c.r32(8))); }});
    g_extra_hooks.push_back({0x00123c30, [](EeCpu& c) { c.set32(2, mc_sync(static_cast<int32_t>(c.r32(4)), c.r32(5), c.r32(6))); }});
    g_extra_noop_hooks = {0x00122298, OVF(0x002a08c8), 0x001187d0, 0x001190f8,
                          OVF(0x002900a0),  // (and the flight's drawing; the boot module's read back 0x243d38 runs on both sides: batch 420)
                          OVF(0x001f8540), 0x00122e68};
    // 989snd's IOP server with the stream system's disc reads (command 0x38) done as the IRX does
    // them (batch 75's): the scene blocks' reads complete on both sides
    auto snd_server = [](bool reads) {
        auto result = [](uint32_t id, uint32_t a0) -> uint32_t {
            const uint32_t h = (id * 0x9e3779b1u) ^ (a0 * 0x85ebca6bu);
            return ((h >> 7) % 4 == 0) ? 0u : ((h >> 3) | 1u);
        };
        sif_rpc_register(0x123456, [result, reads](uint32_t fno, uint32_t send, uint32_t, uint32_t recv, uint32_t) {
            if (fno == 0x4d) {
                const uint32_t n = ee_read<uint32_t>(send);
                uint32_t p = send + 4;
                for (uint32_t i = 0; i < n; ++i) {
                    const uint32_t id = ee_read<uint16_t>(p), size = ee_read<uint16_t>(p + 2);
                    ee_write<uint32_t>(recv + 4 + i * 4, result(id, size != 0 ? ee_read<uint32_t>(p + 4) : 0u));
                    if (reads && id == 0x38) {
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
    };
    snd_server(true);
    // Veldin's level with the stream system up and the display size (as batch 75's)
    static std::vector<uint8_t> snap2;
    static const auto level2 = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap2.empty()) {
            level();
            ee_write<uint16_t>(0x001518d0, 512);
            ee_write<uint16_t>(0x001518d2, 416);
            particle_table_init();
            hud_init();
            w32(OV(0x0016c060), 0xf);
            w32(0x0015ec8c, 1);
            w32(0x00137b00, 0);
            sub_00264bf8();
            snap2.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap2.data(), snap2.size());
        }
    };
    auto pick = [](std::initializer_list<uint32_t> l) { return *(l.begin() + ri(0, static_cast<int>(l.size()) - 1)); };

    check("cd_read_request", 0x00265f40, 20, Ret::Int32, [] {
        level();
        w8(0x0015ed58, u(ri(0, 1)));
        wrand(0x0015eebc, 2);
        return Args{{T2, u(ri(1500, 1600)), u(ri(1, 4))}};
    }, [](A a) { return ri32(cd_read_request(a.i[0], a.i[1], a.i[2])); });

    check("level_load_step", 0x00243db0, N, Ret::Int32, [pick] {
        level2();
        w16(0x0015ee48, ri(0, 2) == 0 ? 3u : u(ri(0, 8)));  // (step 3 often: its two ways)
        w32(0x0015ed80, u(ri(0, 3) == 0));
        w16(0x0013e056, u(ri(-1, 1)));
        w16(0x0015ee4a, u(ri(0, 1)));
        w32(0x0015ed5c, pick({0u, 0xffffffffu, R() | 1u}));
        w32(0x0015eec0, u(ri(0, 1)));
        w32(0x0015eebc, u(ri(0, 0x300)));
        w32(0x0015ed58, u(ri(0, 1)));
        return Args{};
    }, [](A) { return ri32(level_load_step()); });

    // an image: its header (+8 / +0xc size, +0x10 format, +0x14 palette format, +0x1c levels)
    static const uint32_t IMG = TEST_AREA + 0x4000;
    check("tex_image_upload", 0x0023f1d8, N, Ret::Int32, [pick] {
        level();
        const uint32_t w = 1u << ri(3, 6), h = 1u << ri(3, 6);
        w32(IMG + 8, w);
        w32(IMG + 0xc, h);
        w32(IMG + 0x10, pick({0u, 2u, 0x13u, 0x13u, 0x14u, 0x14u, 1u}));
        w32(IMG + 0x14, u(ri(0, 1)));
        w32(IMG + 0x1c, u(ri(1, 4)));
        wrand(IMG + 0x20, 0x2000);
        w32(0x0015ee74, u(ri(0, 0x1000)) << 8);
        return Args{{IMG, OUT}};
    }, [](A a) { return ri32(tex_image_upload(a.i[0], a.i[1])); });

    check("loading_screen_setup", 0x00291d78, 4, Ret::None, [] {
        level2();
        w32(0x0015ed84, u(ri(0, 18)));
        w32(0x0015ed88, u(ri(0, 5)));
        w8(0x0013dd43, u(ri(0, 1)));
        return Args{};
    }, [](A) { loading_screen_setup(); return uint64_t{0}; });

    // the flight (the loading screen's scene set up, then some of its frames played)
    static std::vector<uint8_t> snap3;
    static const auto flight = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap3.empty()) {
            level2();
            w32(0x0015ed84, 3);
            loading_screen_setup();
            snap3.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap3.data(), snap3.size());
        }
    };
    static const uint32_t SC = OV(0x0016c860), G = 0x0013e030;
    check("flight_camera", 0x0028e540, 20, Ret::Int32, [] {
        flight();
        w32(SC + 0x38, u(ri(0, 0x5f)));
        w8(0x0015edb4, u(ri(0, 1)));
        return Args{};
    }, [](A) { return r32(flight_camera()); });
    check("flight_sequence_update", 0x0028f8c0, 30, Ret::None, [pick] {
        flight();
        w32(0x0015ed84, pick({0u, 1u, 3u, u(ri(2, 18))}));
        w8(0x0013dd43, u(ri(0, 1)));
        w16(0x0015ee48, u(ri(0, 7)));
        w16(0x0015ee4a, u(ri(0, 1)));
        w32(G + 0x5c, u(ri(0, 2)));
        wf(OV(0x0015f3fc), rf(0.0f, 1.0f));
        const int32_t n = ee_read<int16_t>(SC + 0x40);
        const int32_t at = ri(0, 3) == 0 ? n - 1 : ri(0, n > 1 ? n - 2 : 0);  // its end, or a frame in it
        for (int32_t f = 0; f < at % 0x60; ++f) flight_sequence_update();     // (frames played before: the blocks streamed)
        w32(SC + 0x34, u(at));
        return Args{};
    }, [](A) { flight_sequence_update(); return uint64_t{0}; });

    // the whole level exit (its loops run to their end on both sides): to another planet, to one
    // with the story's cards and movies (run to their end too), out of the game; the card idle (its
    // wait over at once)
    check("level_exit", 0x00292e08, 4, Ret::None, [pick] {
        level2();
        w32(0x0015ed84, u(ri(0, 18)));
        w32(OV(0x0015f5c0), pick({3u, 3u, u(ri(1, 18)), 0u, 0xffffffffu}));
        w32(0x0015ed88, u(ri(0, 5)));
        for (uint32_t i = 0; i < 0x20; ++i) w8(0x0013dd40 + i, pick({0u, 0u, 1u, 2u}));
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

    snd_server(false);
    movie_hw_enable(true);
    mc_reset();
    g_extra_noop_hooks.clear();
    g_extra_hooks.clear();
    g_host_clock_fixed = 0;
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
