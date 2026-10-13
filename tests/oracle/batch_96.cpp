// Oracle batch 96 (task 0319): game mode 1, the movie. The update 0x2999a8 (the card's machine
// idle, the sounds stopped, the player's entry 0x2edd28 on the movie 0x161048 .., the end), the
// end 0x2997c8 (the textures the player overwrote uploaded again, the music, game mode 0, the
// talk that played the movie stepped on) and the two movie starters 0x299560 / 0x2994f8 beside
// 0x299460 (batch 86), on Veldin's real level data (needs the ISO). The movie player itself
// (0x2ee130 / 0x2eddd0 / 0x2ee3d8: Sony's sample player, threads and IPU) is the host's stand-in
// on both sides, movies off (a movie that cannot be opened), as in batch 85.

#include "ovl_oracle.h"

#include <game/lib/libcdvd.h>
#include <game/lib/libmc.h>
#include <game/lib/movie_player.h>
#include <game/ovl/arrival.h>
#include <game/ovl/cutscene.h>
#include <game/ovl/level_change.h>
#include <game/ovl/memory_card.h>
#include <game/ovl/movie_mode.h>
#include <game/ovl/movies.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/veldin_mobys.h>
#include <runtime/ipu/movie_hw.h>
#include <runtime/kernel/disc.h>

namespace rac1::oracle {

void batch_96() {
    if (g_only_batch != 96) return;
    g_batch = 96;
    g_ignored_words = vcross_lanes();
    g_ignored_words.push_back(OV(0x00160188));  // the particle update's saved return address
    g_ignored_words.push_back(OV(0x0016018c));
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    movie_hw_enable(false);
    g_host_clock_fixed = 1759658400;
    g_extra_hooks = {{0x00121780, [](EeCpu& c) { c.set32(2, cd_read_clock(c.r32(4))); }},
                     {0x0012d6d8, [](EeCpu& c) { clock_to_local(c.r32(4)); }},
                     {0x00123d68, [](EeCpu& c) { c.set32(2, mc_get_info(static_cast<int32_t>(c.r32(4)), static_cast<int32_t>(c.r32(5)), c.r32(6), c.r32(7), c.r32(8))); }},
                     {0x00123c30, [](EeCpu& c) { c.set32(2, mc_sync(static_cast<int32_t>(c.r32(4)), c.r32(5), c.r32(6))); }},
                     {OVF(0x00266040), [](EeCpu& c) { c.set32(2, disc_read_wait(c.r32(4), c.r32(5), c.r32(6))); }},
                     {0x00121450, [](EeCpu& c) { c.set32(2, cd_read(c.r32(4), c.r32(5), c.r32(6), c.r32(7))); }},
                     {0x00121630, [](EeCpu& c) { c.set32(2, cd_get_error()); }},
                     {0x001216c8, [](EeCpu& c) { c.set32(2, cd_break()); }},
                     {OVF(0x00267898), [](EeCpu&) { pad_update(); }},
                     // the movie player's entry points: the host's stand-ins
                     {OVF(0x002ee130), [](EeCpu& c) { c.set32(2, movie_player_open(c.r32(4), c.r32(5), static_cast<int32_t>(c.r32(6))) ? 1 : 0); }},
                     {OVF(0x002eddd0), [](EeCpu& c) {
                          movie_player_loop_start(c.r32(4), c.r32(5), c.r32(6));
                          while (!movie_player_loop_frame()) {
                          }
                          c.set32(2, 0);
                      }},
                     {OVF(0x002ee3d8), [](EeCpu&) { movie_player_close(); }},
                     // the GS memory read back (sceGsExecStoreImage: the DMA from the VIF1 FIFO, not modelled by the interpreter)
                     {0x001227d8, [](EeCpu& c) { c.set32(2, gs_exec_store_image(c.r32(4), c.r32(5))); }},
                     // the player as the port runs it (0x2999a8's callee)
                     {OVF(0x002edd28), [](EeCpu& c) {
                          movie_stream_start(c.r32(4), c.r32(5), c.r32(6), c.r32(7), static_cast<int32_t>(c.r32(8)));
                          while (!movie_stream_frame()) {
                          }
                          c.set32(2, 0);
                      }}};
    // the callees left TODO_PORT: the kernel's vsync wait, thread priority and vsync callback, the
    // DMA kick, the screen fade, the level change request, the effects of other levels' scenes
    g_extra_noop_hooks = {0x00122298, 0x001188e0, 0x001188a0, 0x00122e68, OVF(0x002a08c8), OVF(0x001f8540), OVF(0x0023af30),
                          0x001187d0, 0x001190f8, OVF(0x00243d38), OVF(0x002900a0), OVF(0x0023e008), OVF(0x0026ced0), OVF(0x0024d430)};
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
    static const uint32_t S = 0x001516d0, P = 0x0013f350, TK = OV(0x00179100);
    static std::vector<uint8_t> snap;
    static uint32_t ship = 0;  // the Veldin ship moby (class 834, the talker whose script has the movie)
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            level_setup_body(1, 0);
            ee_write<uint16_t>(0x001518d0, 512);
            ee_write<uint16_t>(0x001518d2, 416);
            particle_table_init();
            hud_init();
            w32(OV(0x0016c060), 0xf);
            w32(0x0015ec8c, 1);
            w32(0x00137b00, 0);
            sub_00264bf8();
            sub_002a07f8();
            std::printf("batch 96: buffer *(0x15f40c) %08x, *(0x173e04) %08x, vram %08x %08x\n", ee_read<uint32_t>(OV(0x0015f40c)), ee_read<uint32_t>(OV(0x00173e04)), ee_read<uint32_t>(0x0015ee88), ee_read<uint32_t>(0x0015ee78));
            snap.assign(ram, ram + 0x02000000);
            for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); ee_read<uint8_t>(m + 0x20) != 0xff; m += 0x100) {
                const uint32_t fn = ee_read<uint32_t>(m + 0x74);
                if (ee_read<int16_t>(m + 0xa6) == 834) ship = m;
                if (fn != 0 && !ovl_moby_fn_ported(fn)) g_extra_noop_hooks.push_back(fn);
            }
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    // the movie's state as movie_start (0x2995d0) leaves it: sector, size, the sound track, the
    // buffers; the VRAM addresses kept below 4 MB, the images read from anywhere in the level's memory
    auto movie_state = [](bool pal) {
        w32(0x0015ed80, pal ? u(ri(0, 3) == 0) : 0u);
        w32(0x0015ed88, u(ri(0, 5)));
        w32(OV(0x00161048), u(ri(500000, 700000)));
        w32(OV(0x0016104c), u(ri(1, 0x1000000)));
        w32(OV(0x00161050), u(ri(0, 5)));
        w32(OV(0x00161054), NOT_ADDR(0x00200000u) + u(ri(0, 0x100000)));  // (any alignment: the player's buffers are rounded up to 64 bytes)
        w32(OV(0x00161058), ee_read<uint32_t>(OV(0x0015f40c)) + u(ri(0, 0x10000)));  // (stored into: past the code)
        if (coin()) {
            w32(0x0015ee88, 0x100u * u(ri(0, 0x2000)));
            w32(0x0015ee78, 0x100u * u(ri(0, 0x3000)));
        }
        w32(0x0015eed8, ri(0, 2) == 0 ? 2u : coin() ? 0xffffffffu : u(ri(0, 1)));
        w16(S + 0x38, R());
        w8(0x0013e5bb, R());
        w32(OV(0x0015f5c4), 1);
    };
    // the talk that played the movie: the ship's talk struct and a script of `n` records (scenes /
    // movies / none; the next records forward), as batch 80's: the end steps it on, which may play
    // the next record's scene or movie at once
    auto talk_state = [](int32_t n, bool movies_only = false) {
        wrand(M3, 0x10);
        for (int32_t r = 0; r < n; ++r) {
            const uint32_t e = T + u(r) * 0x1c;
            wrand(e, 7);
            static const uint32_t texts[] = {0, 0x4ee2, 0x4ee5, 0x4f1d};
            w32(e, texts[ri(0, 3)]);
            const int k = ri(0, 3);
            w16(e + 4, k == 0 ? 0xffffu : (k == 1 || movies_only) ? (0x4000u | u(ri(0, 3))) : 4u);
            w16(e + 6, ri(0, 4) == 0 ? u(r) : u(ri(r, n - 1)));
            w16(e + 8, u(ri(0, 7)));
            w16(e + 0xa, u(ri(0, 6)));
            w16(e + 0xc, u(ri(r, n - 1)));
            w16(e + 0xe, u(ri(r, n - 1)));
            w16(e + 0x10, R() & 0xfu);
        }
        const int32_t i = talker_index(ship);
        if (i >= 0) w32(OV(0x001b1678) + u(i) * 4, T);
        w32(0x0015ed98, u(ri(0, 4000)));
        for (int32_t k = 0; k < 0x30; ++k) w8(0x0013d4c0 + u(k), R() & 1);
        for (int32_t k = 0; k < 0x30; ++k) w8(0x0013d4e8 + u(k), R() & 1);
        for (int32_t k = 0; k < 0x10; ++k) w8(0x0013d388 + u(k), R() & 1);
        w32(OV(0x00160130), coin() ? 0xffffffffu : u(ri(0, 3)));
        talk_init(ship, M3);
        w16(M3 + 0x36, u(ri(0, n - 1)));
        w32(TK + 8, ship);
        w32(TK + 0xc, M3);
        // the scene a record plays: the dialog slot ready (batch 80's), the hero alive
        w16(S + 0x5a, 3);
        w32(S + 0x50, (R() & 0x7ffffffeu) | 1u);
        w16(S + 0x5c, 0);
        w32(S + 0x1c, 0xffffffffu);
        w32(P + 0x22a8, ri(0, 8) == 0 ? 0u : 1u);
        w32(OV(0x0015f5cc), u(ri(0, 100)));
    };
    auto hook_new_mobys = [] {
        for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); ee_read<uint8_t>(m + 0x20) != 0xff; m += 0x100) {
            const uint32_t fn = ee_read<uint32_t>(m + 0x74);
            if (fn != 0 && !ovl_moby_fn_ported(fn) && std::find(g_extra_noop_hooks.begin(), g_extra_noop_hooks.end(), fn) == g_extra_noop_hooks.end())
                g_extra_noop_hooks.push_back(fn);
        }
    };

    // ---- sceGsSetDefStoreImage (the packet it builds over what was at lp)
    check("gs_set_def_store_image", 0x00122518, 60, Ret::Int32, [level] {
        level();
        wrand(T, 0x20);
        return Args{{T, R() & 0xffff, R() & 0xff, R() & 0xff, R() & 0xffff, R() & 0xffff, R() & 0xffff, R() & 0xffff}, {}};
    }, [](A a) {
        return ri32(gs_set_def_store_image(a.i[0], static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]),
                                           static_cast<int32_t>(a.i[5]), static_cast<int32_t>(a.i[6]), static_cast<int32_t>(a.i[7])));
    });
    // ---- the end (0x2997c8): nothing talking, or the ship's talk
    check("movie_mode_end", 0x002997c8, 60, Ret::None, [level, movie_state, talk_state, hook_new_mobys] {
        level();
        mc_reset();
        movie_state(true);
        if (coin()) {
            talk_state(ri(1, 6));
            hook_new_mobys();
        }
        return Args{};
    }, [](A) {
        mc_reset();
        movie_mode_end();
        return uint64_t{0};
    });

    // ---- the whole mode 1 update (0x2999a8): the card's machine first (idle, an operation asked or
    // running), the player (hooked: the stand-in's frames to the end), then the end
    check("movie_mode", 0x002999a8, 40, Ret::None, [level, movie_state, talk_state, hook_new_mobys] {
        level();
        mc_reset();
        movie_state(true);
        static const uint32_t states[] = {0u, 1u, 2u, 2u, 2u, 2u};
        w32(0x0013d290 + 0xd4, states[ri(0, 5)]);
        w32(0x0013d290 + 0xdc, ri(0, 3) != 0 ? 0xffffffffu : u(ri(-1, 2)));
        // sounds and music running (stopped by the update)
        for (uint32_t o : {0x40u, 0x5cu, 0x78u}) {
            w16(S + o, coin() ? 0x8000u : u(ri(0, 4)));
            w16(S + o + 2, R());
        }
        if (coin()) {
            talk_state(ri(1, 6));
            hook_new_mobys();
        }
        return Args{};
    }, [](A) {
        mc_reset();
        movie_mode_run();
        return uint64_t{0};
    });
    // the host's frame split: the same movie, the update called once a frame until the mode ends
    // (here every frame's work compared at the end)
    check("movie_mode_frames", 0x002999a8, 20, Ret::None, [level, movie_state, talk_state, hook_new_mobys] {
        level();
        mc_reset();
        movie_state(true);
        w32(0x0013d290 + 0xd4, 2);
        w32(0x0013d290 + 0xdc, 0xffffffffu);
        if (coin()) {
            talk_state(ri(1, 6));
            hook_new_mobys();
        }
        return Args{};
    }, [](A) {
        mc_reset();
        for (int f = 0; f < 100000; ++f) {
            movie_mode_update();
            if (ee_read<int32_t>(OV(0x0015f5c4)) != 1) break;
        }
        return uint64_t{0};
    });

    // ---- talk_step (0x266970) with movie records: the record's flag 4 plays the next one's movie at once
    check("talk_step_movie", 0x00266970, 60, Ret::None, [level, talk_state, hook_new_mobys] {
        level();
        talk_state(ri(2, 6), true);
        hook_new_mobys();
        w32(TK + 8, 0);
        w32(TK + 0xc, 0);
        w32(OV(0x0015f5c4), 0);
        return Args{{ship, M3, 1}, {}};
    }, [](A a) { talk_step(a.i[0], a.i[1], static_cast<int32_t>(a.i[2])); return uint64_t{0}; });
    // ---- the starters of the pause menu's screens beside 0x299460 (batch 86)
    auto starter_state = [level] {
        level();
        w32(0x0015ed80, u(ri(0, 3) == 0));
        w32(0x0015ed88, u(ri(0, 5)));
        w32(OV(0x0015f40c), NOT_ADDR(0x00200000u) + 0x10 * u(ri(0, 0x10000)));
        if (coin()) {
            w32(0x0015ee88, 0x100u * u(ri(0, 0x2000)));
            w32(0x0015ee78, 0x100u * u(ri(0, 0x3000)));
        }
        w8(0x0013e5bb, R());
    };
    check("movie_story_play", 0x00299560, 40, Ret::None, [starter_state] {
        starter_state();
        return Args{{u(ri(-2, 0x10))}, {}};
    }, [](A a) { movie_story_play(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    check("movie_table2_play", 0x002994f8, 40, Ret::None, [starter_state] {
        starter_state();
        return Args{{u(ri(-2, 0x10))}, {}};
    }, [](A a) { movie_table2_play(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });

    movie_hw_enable(true);
    mc_reset();
    g_host_clock_fixed = 0;
    g_extra_noop_hooks.clear();
    g_extra_hooks.clear();
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
