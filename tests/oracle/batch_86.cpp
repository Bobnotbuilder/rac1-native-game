// Oracle batch 86 (task 0303): Veldin's remaining moby callbacks on Veldin's real level data
// (needs the ISO): the class-1545 update 0x2e3800 with the smoke helpers 0x2637f8 / 0x263b70,
// the class-1440 muzzle flash's billboard 0x2e1c78, the ship's update 0x28e180 with 0x2644e0
// and the movie start 0x299460 / 0x2995d0, the ships' canopy 0x2935b8.

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/veldin_mobys.h>
#include <runtime/kernel/disc.h>

#include <cmath>
#include <map>

namespace rac1::oracle {

void batch_86() {
    if (g_only_batch != 86) return;
    g_batch = 86;
    g_ignored_words = vcross_lanes();
    g_ignored_words.push_back(OV(0x00160188));  // the particle update's saved return address
    g_ignored_words.push_back(OV(0x0016018c));
    // the kernel's vsync wait (the cutscene start's loop)
    g_extra_noop_hooks = {0x00122298};
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;
    // 989snd's IOP server (as batch 80's): replies a function of the command; the stream system's
    // disc reads (command 0x38) done into EE RAM as the IRX does them
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
    static const uint32_t SC = OV(0x0016c860), P = 0x0013f350, S = 0x001516d0;
    static std::vector<uint8_t> snap;
    static std::vector<uint32_t> jointed;  // the level's mobys with a class model (chains)
    static uint32_t ship = 0, ship530 = 0, crash = 0, e1440 = 0;
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            w16(0x001518d0, 512);  // the display as the frame sets it (512 x 416)
            w16(0x001518d2, 416);
            particle_table_init();
            level_setup_body(1, 0);
            hud_init();
            w32(OV(0x0016c060), 0xf);
            w32(0x0015ec8c, 1);  // the stream system up, the disc idle
            w32(0x00137b00, 0);
            sub_00264bf8();
            gameplay_frame();
            screen_setup();
            projection_setup();
            render_view_setup();
            snap.assign(ram, ram + 0x02000000);
            for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); ee_read<uint8_t>(m + 0x20) != 0xff; m += 0x100) {
                const uint32_t fn = ee_read<uint32_t>(m + 0x74);
                const int c = ee_read<int16_t>(m + 0xa6);
                if (fn != 0 && !ovl_moby_fn_ported(fn)) g_extra_noop_hooks.push_back(fn);
                if (ee_read<uint32_t>(m + 0x24) != 0 && c != 0) jointed.push_back(m);
                if (c == 0x213 && ship == 0) ship = m;
                if (c == 0x212 && ship530 == 0) ship530 = m;
                if (c == 1545 && crash == 0) crash = m;
                if (c == 1440 && e1440 == 0) e1440 = m;
            }
            std::printf("batch 86: ship %08x, 530 %08x, 1545 %08x, 1440 %08x, %zu mobys with chains\n", ship, ship530, crash, e1440, jointed.size());
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    level();
    if (std::getenv("ORACLE_B86_INFO") != nullptr) {  // the level's mobys: class, update, state
        std::map<int, int> n;
        for (uint32_t m = ee_read<uint32_t>(OV(0x0015ffd8)); ee_read<uint8_t>(m + 0x20) != 0xff; m += 0x100) {
            const int c = ee_read<int16_t>(m + 0xa6);
            if (n[c]++ < 2)
                std::printf("moby %08x class %d (0x%x) fn %08x state %d class data %08x pvars %08x\n", m, c, c, ee_read<uint32_t>(m + 0x74), ee_read<uint8_t>(m + 0x20),
                            ee_read<uint32_t>(m + 0x24), ee_read<uint32_t>(m + 0x78));
        }
    }
    auto pick = [] { return jointed[static_cast<size_t>(ri(0, static_cast<int32_t>(jointed.size()) - 1))]; };
    // a moby moved, turned and scaled a little (its world matrix +0xc0 a turn about z)
    auto shake = [](uint32_t m) {
        for (uint32_t k = 0; k < 3; ++k) wf(m + 0x10 + k * 4, ee_read<float>(m + 0x10 + k * 4) + rf(-3, 3));
        if (coin()) {
            const float a = rf(-3.1f, 3.1f), s = rf(0.3f, 1.6f);
            const float r[12] = {s * std::cos(a), s * std::sin(a), 0, 0, -s * std::sin(a), s * std::cos(a), 0, 0, 0, 0, s, 0};
            for (uint32_t i = 0; i < 12; ++i) wf(m + 0xc0 + 4 * i, r[i]);
        }
        if (coin()) wf(m + 0x2c, rf(0.5f, 2.0f) * ee_read<float>(m + 0x2c));
    };
    // the particles partly used up (none free now and then)
    auto particles = [] {
        if (ri(0, 6) == 0) w32(OV(0x00161390), u(ri(0x100, 0x400)));
        seed_rand();
    };
    // a point in front of the view (or behind it)
    auto in_view = [](uint32_t at) {
        const float cx = ee_read<float>(OV(0x00166dc0)), cy = ee_read<float>(OV(0x00166dc4)), cz = ee_read<float>(OV(0x00166dc8));
        const float d = ri(0, 4) == 0 ? rf(-10, 1) : rf(1, 40), ox = rf(-0.5f, 0.5f) * d, oy = rf(-0.4f, 0.4f) * d;
        for (uint32_t k = 0; k < 3; ++k)
            wf(at + 4 * k, (k == 0 ? cx : k == 1 ? cy : cz) + ee_read<float>(OV(0x00166c88) + 0x10 * k) * d + ee_read<float>(OV(0x00166c80) + 0x10 * k) * ox +
                               ee_read<float>(OV(0x00166c84) + 0x10 * k) * oy);
        wf(at + 0xc, coin() ? 1.0f : rf(0, 2));
    };

    // ---- the smoke
    check("scene_moby_smoke", 0x002637f8, 120, Ret::None, [level, pick, shake, particles] {
        level();
        uint32_t m = pick();
        if (ri(0, 30) == 0) m = 0;
        shake(m == 0 ? M : m);
        w16(m + 0xa6, ri(0, 5) == 0 ? R() & 0xfffu : 0x2eeu);
        particles();
        return Args{{m}, {}};
    }, [](A a) { scene_moby_smoke(a.i[0]); return uint64_t{0}; });
    check("smoke_burst", 0x00263b70, 80, Ret::None, [level, particles] {
        level();
        wfrand(T, 8, -300, 300);
        wfrand(T + 0x20, 4, -0.1f, 0.1f);
        particles();
        return Args{{T, coin() ? T : T + 0x10, coin() ? OV(0x0015f580u) : T + 0x20}, {rf(1000, 200000), rf(1000, 200000)}};
    }, [](A a) { smoke_burst(a.i[0], a.i[1], a.i[2], a.f[0], a.f[1]); return uint64_t{0}; });
    // the crash site in the opening cutscene's scenes: scene 2..4 (or others) near the frames that
    // matter, its scene mobys some of the level's (posed: class 750 now and then)
    check("moby_1545_update", 0x002e3800, 200, Ret::None, [level, pick, shake, particles] {
        level();
        w8(crash + 0x20, u(ri(0, 6) == 0 ? ri(0, 3) : 1));
        w32(OV(0x0015f5c4), ri(0, 5) == 0 ? u(ri(0, 6)) : 2u);
        const int32_t scene = ri(0, 4) == 0 ? ri(0, 6) : ri(2, 4);
        w32(SC + 0x30, u(scene));
        static const int32_t marks[] = {0x618, 0x96, 0xdc};
        const int32_t mk = marks[ri(0, 2)];
        w32(SC + 0x34, u(coin() ? mk + ri(-1, 1) : ri(0, 0x700)));
        for (uint32_t i = 0; i < 12; ++i) {
            const uint32_t m = ri(0, 4) == 0 ? 0u : pick();
            w32(SC + 0x178 + i * 4, m);
            if (m != 0 && ri(0, 2) != 0) {
                w16(m + 0xa6, 0x2ee);
                shake(m);
            }
        }
        if (coin()) w32(SC + 0x184, ee_read<uint32_t>(0x001413d0));  // (the hero: his chain points)
        particles();
        return Args{{crash}, {}};
    }, [](A a) { moby_1545_update(a.i[0]); return uint64_t{0}; });
    // scene 4's burst at the edges of its frames 0x96..0xdc
    check("moby_1545_burst", 0x002e3800, 60, Ret::None, [level, pick, shake, particles] {
        level();
        w8(crash + 0x20, 1);
        w32(OV(0x0015f5c4), 2);
        w32(SC + 0x30, 4);
        w32(SC + 0x34, u(coin() ? ri(0x94, 0x98) : ri(0xda, 0xde)));
        const uint32_t m = coin() ? ee_read<uint32_t>(0x001413d0) : pick();
        shake(m);
        w32(SC + 0x184, m);
        particles();
        return Args{{crash}, {}};
    }, [](A a) { moby_1545_update(a.i[0]); return uint64_t{0}; });
    // ---- the class-1440 muzzle flash's billboard
    check("e1440_flash_draw", 0x002e1c78, 200, Ret::None, [level, in_view] {
        level();
        const uint32_t pv = ee_read<uint32_t>(e1440 + 0x78);
        in_view(pv + 0x1c0);
        if (coin()) { w32(OV(0x0016d180) + 0xb * 0x10, 0); w32(OV(0x0016d184) + 0xb * 0x10, 0); }  // its texture not set up yet
        if (ri(0, 4) == 0) wfrand(0x0013f5e0, 3, -1, 1);
        w32(OV(0x001611c0), 0x01e10000u + 0x10 * u(ri(0, 0x100)));
        return Args{{e1440}, {}};
    }, [](A a) { e1440_flash_draw(a.i[0]); return uint64_t{0}; });
    // ---- the ship
    auto hud_state = [] {
        if (coin()) {
            hud_message_request(ri(0, 3), 0x53e4);
        } else {
            w32(OV(0x0015f594), u(ri(0, 3)));
            w32(OV(0x00161e04), coin() ? 0xffffffffu : u(ri(0, 8)));
        }
    };
    check("hud_message_release", 0x002644e0, N, Ret::Int32, [level, hud_state] {
        level();
        hud_state();
        return Args{{u(ri(0, 3))}, {}};
    }, [](A a) { return ri32(hud_message_release(static_cast<int32_t>(a.i[0]))); });
    // the movie's start: its images read from anywhere in the level's memory, VRAM addresses
    // kept below 4 MB (the original's debug trap)
    auto movie_setup = [](bool pal) {
        if (pal) w32(0x0015ed80, u(ri(0, 3) == 0));
        w32(OV(0x0015f40c), NOT_ADDR(0x00200000u) + 0x10 * u(ri(0, 0x10000)));
        if (coin()) {
            w32(0x0015ee88, 0x100u * u(ri(0, 0x2000)));
            w32(0x0015ee78, 0x100u * u(ri(0, 0x3000)));
        }
        w8(0x0013e5bb, R());
    };
    check("movie_start", 0x002995d0, 30, Ret::None, [level, movie_setup] {
        level();
        movie_setup(true);
        return Args{{R(), R(), R()}, {}};
    }, [](A a) { movie_start(a.i[0], a.i[1], a.i[2]); return uint64_t{0}; });
    check("movie_play", 0x00299460, 40, Ret::None, [level, movie_setup] {
        level();
        movie_setup(true);
        w32(P + 0x22a8, u(ri(0, 3) != 0));
        w8(P + 0x20b1, u(ri(0, 3) == 0));
        return Args{{u(ri(-2, 0x18))}, {}};
    }, [](A a) { movie_play(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    // the ship: shown or not, the hero near its hatch (or not), the prompt, triangle, level 10's
    // boarding (scene 9) and the scenes / movie after it (state 0x2a)
    auto ship_setup = [level, hud_state, movie_setup](bool scenes) {
        level();
        const uint32_t pv = ee_read<uint32_t>(ship + 0x78);
        w8(ship + 0x20, scenes ? 0x2au : (ri(0, 8) == 0 ? 0x2au : u(ri(0, 1))));
        w8(ship + 0x31, R() & 1);
        w8(ship + 0xbc, R());
        w16(ship + 0xa6, u(ri(0x213, 0x215)));
        w16(0x0013e056, u(ri(0, 2)));
        w16(pv + 0xc, R() & 1);
        w16(ship + 0x34, ee_read<uint16_t>(ship + 0x34) ^ (ri(0, 4) == 0 ? 1u : 0u));
        const float a = rf(-3.1f, 3.1f), d = coin() ? rf(0, 7) : rf(0, 40);
        wf(0x0013f3d0, ee_read<float>(ship + 0x10) + d * std::cos(a));
        wf(0x0013f3d4, ee_read<float>(ship + 0x14) + d * std::sin(a));
        wf(0x0013f3d8, ee_read<float>(ship + 0x18) + rf(-3, 3));
        wf(OV(0x00166dc0), ee_read<float>(ship + 0x10) + rf(-40, 40));
        wf(OV(0x00166dc4), ee_read<float>(ship + 0x14) + rf(-40, 40));
        w32(0x0013cae4, coin() ? 0x10u : R() & 0xefu);
        if (coin()) {  // level 10's boarding (its scene 9): Veldin's scene 4 stands in for it
            w32(0x0015ed84, 10);
            for (uint32_t i = 0; i < 8; i += 4) w32(0x00137b80 + 0x12e8 + 9 * 8 + i, ee_read<uint32_t>(0x00137b80 + 0x12e8 + 4 * 8 + i));
            for (uint32_t i = 0; i < 0x250; i += 4) w32(0x00137b80 + 0x2afc + 9 * 0x250 + i, ee_read<uint32_t>(0x00137b80 + 0x2afc + 4 * 0x250 + i));
        } else {
            w32(0x0015ed84, 0);
        }
        w8(0x001413f4, R() & 1);
        w32(OV(0x0015f5c4), ri(0, 4) == 0 ? 2u : 0u);
        for (uint32_t k = 0; k < 4; ++k) w8(0x0013d3d0 + k, ri(0, 2) == 0 ? 0u : 1u);
        w8(0x0013d4dc, R() & 1);
        w8(0x0013dd4b, R() & 1);
        hud_state();
        movie_setup(false);  // (the cutscenes' scene table is the NTSC one: no PAL here)
        w32(P + 0x22a8, 1);
        w8(P + 0x20b1, 0);
        // the scene's sound ready (the dialog slot in state 3 with a handle: the cutscene start's
        // wait loop not entered)
        w16(S + 0x5a, 3);
        w32(S + 0x50, (R() & 0x7ffffffeu) | 1u);
        w16(S + 0x5c, 0);
        w32(S + 0x1c, 0xffffffffu);
    };
    check("ship_update", 0x0028e180, 150, Ret::None, [ship_setup] {
        ship_setup(false);
        return Args{{ship}, {}};
    }, [](A a) { ship_update(a.i[0]); return uint64_t{0}; });
    check("ship_update_scenes", 0x0028e180, 30, Ret::None, [ship_setup] {
        ship_setup(true);
        return Args{{ship}, {}};
    }, [](A a) { ship_update(a.i[0]); return uint64_t{0}; });
    // ---- the ships' canopy: every class's table on the ship (or class 530's own moby), the
    // camera near or far, the cut / blend state, the last coordinates kept
    check("ship_canopy_draw", 0x002935b8, 200, Ret::None, [level, in_view] {
        level();
        const bool own = ri(0, 3) == 0;
        const uint32_t m = own ? ship530 : ship;
        if (!own) w16(m + 0xa6, u(ri(0x212, 0x215)));
        w32(OV(0x0015f5c4), ri(0, 2) == 0 ? (coin() ? 6u : u(ri(1, 3))) : 0u);
        w32(0x0013e050, coin() ? 4u : u(ri(0, 5)));
        if (coin()) {  // the ship in view
            in_view(m + 0x10);
        }
        if (coin()) {
            wf(OV(0x00166dc0), ee_read<float>(m + 0x10) + rf(-20, 20));
            wf(OV(0x00166dc4), ee_read<float>(m + 0x14) + rf(-20, 20));
        }
        w32(OV(0x00160564), u(ri(0, 2)));
        w32(OV(0x00160568), u(ri(0, 1)));
        w32(OV(0x0016056c), u(ri(0, 3) == 0 ? 0 : ri(0, 70)));
        wfrand(OV(0x001c0c40), 0x66 * 2, 0, 1);
        wfrand(OV(0x001c0f70), 0x66 * 2, 0, 1);
        if (coin()) { w32(OV(0x0016d180) + 0x15 * 0x10, 0); w32(OV(0x0016d184) + 0x15 * 0x10, 0); }
        w32(OV(0x001611c0), 0x01e10000u + 0x10 * u(ri(0, 0x100)));
        return Args{{m}, {}};
    }, [](A a) { ship_canopy_draw(a.i[0]); return uint64_t{0}; });
    snd_server(false);
    g_extra_noop_hooks.clear();
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
