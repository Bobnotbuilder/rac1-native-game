// Oracle batch 66: the sounds: 989snd's EE library and the overlay's sound frame (needs the ISO).

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
namespace rac1::oracle {

void batch_66() {
    if (g_only_batch != 66) return;
    auto boot = [] { veldin_boot(); };
    g_batch = 66;
    g_ignored_words = vcross_lanes();
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
    };
    // a history of library calls (the port's, the same RAM for both sides): batches sent or
    // held, synchronous calls, a full buffer now and then
    auto history = [] {
        const int n = ri(0, 3) == 0 ? ri(200, 300) : ri(0, 12);
        if (ri(0, 2) == 0) snd_hold();
        for (int k = 0; k < n; ++k) {
            switch (ri(0, 9)) {
            case 0: snd_play_sound(R(), R() & 0xff, ri(0, 0x400), ri(-180, 180), ri(-100, 100), 0, coin() ? OVF(0x0028e108u) : 0u, OUT + u(ri(0, 8)) * 0x20); break;
            case 1: snd_set_sound(R(), R() & 0x1f, ri(0, 0x400), 0, 0, 0, OVF(0x0028e150), OUT + u(ri(0, 8)) * 0x20); break;
            case 2: snd_stop_sound(R()); break;
            case 3: snd_sound_is_playing(R(), OVF(0x0028e150), OUT + u(ri(0, 8)) * 0x20); break;
            case 4: snd_set_master_volume(R() & 7, ri(0, 0x400)); break;
            case 5: snd_flush(); break;
            case 6: if (ri(0, 4) == 0) snd_release(); break;
            case 7: snd_cmd_4e(R(), R(), R()); break;
            case 8: if (ri(0, 6) == 0) snd_cmd_18(); break;
            default: snd_cmd_0d(R(), R()); break;
            }
        }
    };
    auto lib = [level, history] { level(); history(); };
    check("snd_flush", 0x0012dc80, 60, Ret::Int32, [lib] { lib(); return Args{}; }, [](A) { return ri32(snd_flush()); });
    check("snd_reply_ready", 0x0012de70, 30, Ret::Int32, [lib] { lib(); return Args{}; }, [](A) { return ri32(snd_reply_ready()); });
    check("snd_batch_send", 0x0012e9d8, 40, Ret::None, [lib] {
        lib();
        if (ee_read<uint32_t>(0x0015ec80) != 0) snd_flush();
        return Args{};
    }, [](A) { snd_batch_send(); return uint64_t{0}; });
    check("snd_command", 0x0012e6e0, 80, Ret::None, [lib] {
        lib();
        const int size = ri(0, 3) == 0 ? 0 : ri(1, 0x40);
        wrand(T, 0x10);
        return Args{{R() & 0x5f, u(size), T, coin() ? 0u : OVF(0x0028e150u), OUT + u(ri(0, 8)) * 0x20}, {}};
    }, [](A a) { snd_command(a.i[0], static_cast<int32_t>(a.i[1]), a.i[2], a.i[3], static_cast<uint64_t>(sx(a.i[4]))); return uint64_t{0}; });
    check("snd_command_added", 0x0012e9a0, 30, Ret::None, [lib] { lib(); return Args{}; }, [](A) { snd_command_added(); return uint64_t{0}; });
    check("snd_command_sync", 0x0012e548, 60, Ret::Int32, [lib] {
        lib();
        wrand(T, 0x10);
        return Args{{R() & 0x5f, coin() ? 0u : u(ri(1, 0x40)), T}, {}};
    }, [](A a) { return ri32(snd_command_sync(a.i[0], static_cast<int32_t>(a.i[1]), a.i[2])); });
    check("snd_play_sound", 0x0012e308, 40, Ret::None, [lib] {
        lib();
        return Args{{R(), R() & 0xff, u(ri(0, 0x400)), u(ri(-180, 180)), u(ri(-200, 200)), u(ri(-2, 2)), OVF(0x0028e108), OUT}, {}};
    }, [](A a) { snd_play_sound(a.i[0], a.i[1], static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]), static_cast<int32_t>(a.i[5]), a.i[6], static_cast<uint64_t>(sx(a.i[7]))); return uint64_t{0}; });
    check("snd_set_sound", 0x0012e4c0, 40, Ret::None, [lib] {
        lib();
        return Args{{R(), R() & 0x1f, u(ri(0, 0x400)), u(ri(-180, 180)), u(ri(-200, 200)), u(ri(-2, 2)), OVF(0x0028e150), OUT}, {}};
    }, [](A a) { snd_set_sound(a.i[0], a.i[1], static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]), static_cast<int32_t>(a.i[4]), static_cast<int32_t>(a.i[5]), a.i[6], static_cast<uint64_t>(sx(a.i[7]))); return uint64_t{0}; });
    check("snd_stop_sound", 0x0012e368, 20, Ret::None, [lib] { lib(); return Args{{R()}, {}}; }, [](A a) { snd_stop_sound(a.i[0]); return uint64_t{0}; });
    check("snd_sound_is_playing", 0x0012e448, 20, Ret::None, [lib] { lib(); return Args{{R(), OVF(0x0028e150), OUT}, {}}; },
          [](A a) { snd_sound_is_playing(a.i[0], a.i[1], static_cast<uint64_t>(sx(a.i[2]))); return uint64_t{0}; });
    check("snd_set_master_volume", 0x0012e208, 20, Ret::None, [lib] { lib(); return Args{{R() & 7, u(ri(0, 0x400))}, {}}; },
          [](A a) { snd_set_master_volume(a.i[0], static_cast<int32_t>(a.i[1])); return uint64_t{0}; });
    check("snd_cmd_08", 0x0012e1a8, 20, Ret::None, [lib] { lib(); return Args{}; }, [](A) { snd_cmd_08(); return uint64_t{0}; });
    check("snd_cmd_18", 0x0012e3b8, 20, Ret::None, [lib] { lib(); return Args{}; }, [](A) { snd_cmd_18(); return uint64_t{0}; });
    check("snd_cmd_06", 0x0012e1d8, 20, Ret::None, [lib] { lib(); return Args{{R()}, {}}; }, [](A a) { snd_cmd_06(a.i[0]); return uint64_t{0}; });
    check("snd_cmd_0b", 0x0012e240, 20, Ret::None, [lib] { lib(); return Args{{R()}, {}}; }, [](A a) { snd_cmd_0b(a.i[0]); return uint64_t{0}; });
    check("snd_cmd_0d", 0x0012e280, 20, Ret::None, [lib] { lib(); return Args{{R(), R()}, {}}; }, [](A a) { snd_cmd_0d(a.i[0], a.i[1]); return uint64_t{0}; });
    check("snd_cmd_4e", 0x0012e2b8, 20, Ret::None, [lib] { lib(); return Args{{R(), R(), R()}, {}}; }, [](A a) { snd_cmd_4e(a.i[0], a.i[1], a.i[2]); return uint64_t{0}; });
    check("snd_cmd_16", 0x0012e3e8, 20, Ret::None, [lib] { lib(); return Args{{R()}, {}}; }, [](A a) { snd_cmd_16(a.i[0]); return uint64_t{0}; });
    check("snd_cmd_17", 0x0012e418, 20, Ret::None, [lib] { lib(); return Args{{R()}, {}}; }, [](A a) { snd_cmd_17(a.i[0]); return uint64_t{0}; });
    check("snd_cmd_ef68", 0x0012ef68, 20, Ret::None, [lib] { lib(); return Args{{R(), R(), R(), R(), R()}, {}}; },
          [](A a) { snd_cmd_ef68(a.i[0], a.i[1], a.i[2], a.i[3], a.i[4]); return uint64_t{0}; });
    check("snd_cmd_efe0", 0x0012efe0, 20, Ret::None, [lib] { lib(); return Args{{R(), R(), R(), R()}, {}}; },
          [](A a) { snd_cmd_efe0(a.i[0], a.i[1], a.i[2], a.i[3]); return uint64_t{0}; });
    check("snd_hold", 0x0012eaf0, 10, Ret::None, [lib] { lib(); return Args{}; }, [](A) { snd_hold(); return uint64_t{0}; });
    check("snd_release", 0x0012eb00, 30, Ret::None, [lib] { lib(); return Args{}; }, [](A) { snd_release(); return uint64_t{0}; });
    check("snd_reply_wait_set", 0x0012def8, 20, Ret::None, [lib] { lib(); return Args{{OUT + 0x400, u(ri(0, 20))}, {}}; },
          [](A a) { snd_reply_wait_set(a.i[0], static_cast<int32_t>(a.i[1])); return uint64_t{0}; });
    check("snd_bank_load_by_loc", 0x0012df20, 30, Ret::Int32, [lib] {
        lib();
        if (ri(0, 3) == 0) w32(0x0015ecc8, 1);
        return Args{{R(), R()}, {}};
    }, [](A a) { return ri32(snd_bank_load_by_loc(a.i[0], a.i[1])); });
    check("snd_bank_load_from_ee_cb", 0x0012e088, 30, Ret::None, [lib] {
        lib();
        if (ri(0, 3) == 0) w32(0x0015ecc8, 1);
        return Args{{R(), OVF(0x0028e0f0), OUT + 0x100}, {}};
    }, [](A a) { snd_bank_load_from_ee_cb(a.i[0], a.i[1], static_cast<uint64_t>(sx(a.i[2]))); return uint64_t{0}; });
    check("snd_flush_bank_loaded", 0x0012dc80, 30, Ret::Int32, [lib] {  // a bank load finished: its callback
        lib();
        snd_bank_load_from_ee_cb(R(), coin() ? OVF(0x0028e0f0u) : 0u, OUT + 0x100);
        return Args{};
    }, [](A) { return ri32(snd_flush()); });
    check("snd_cd_status", 0x0012ee08, 30, Ret::Int32, [lib] {
        lib();
        const bool stream = coin();
        w32(0x0015ec8c, stream ? 1u : 0u);
        w32(0x00137b00, stream && coin() ? 1u : 0u);
        return Args{{stream && ee_read<uint32_t>(0x00137b00) != 0 ? 1u : u(ri(0, 1))}, {}};
    }, [](A a) { return ri32(snd_cd_status(static_cast<int32_t>(a.i[0]))); });
    check("snd_set_stream_callback", 0x0012ef28, 20, Ret::Int32, [lib] {
        lib();
        w32(0x0015ec8c, coin() ? 1u : 0u);
        w32(0x0015ec90, R());
        return Args{{R()}, {}};
    }, [](A a) { return r32(snd_set_stream_callback(a.i[0])); });
    check("snd_pitch_from_cents", 0x0012f1a0, 60, Ret::Int32, [] { return Args{{u(ri(-5000, 5000))}, {}}; },
          [](A a) { return ri32(snd_pitch_from_cents(static_cast<int32_t>(a.i[0]))); });
    check("snd_init", 0x0012da28, 10, Ret::None, [lib] { lib(); wrand(0x0015eca0, 8); return Args{}; }, [](A) { snd_init(); return uint64_t{0}; });
    check("snd_cmd_51", 0x0012efa8, 20, Ret::None, [lib] { lib(); return Args{{R(), R()}, {}}; }, [](A a) { snd_cmd_51(a.i[0], a.i[1]); return uint64_t{0}; });
    check("snd_stream_init", 0x0012eb20, 30, Ret::Int32, [lib] {
        lib();
        w32(0x0015ec8c, ri(0, 3) == 0 ? 1u : 0u);
        return Args{{R(), R(), R(), R()}, {}};
    }, [](A a) { return ri32(snd_stream_init(a.i[0], a.i[1], a.i[2], a.i[3])); });
    check("stream_system_init", 0x00264b68, 20, Ret::None, [lib] { lib(); w32(0x0015ec8c, 0); return Args{}; }, [](A) { stream_system_init(); return uint64_t{0}; });
    check("sound_system_init", 0x0028ca88, 20, Ret::None, [lib] {
        lib();
        w32(0x0015ec8c, 0); w32(0x0015ede8, u(ri(0, 1))); w32(0x0015edec, u(ri(-0x400, 0x400))); w32(0x0015edf0, u(ri(-0x400, 0x400)));
        return Args{};
    }, [](A) { sound_system_init(); return uint64_t{0}; });
    check("clampf", 0x001ff240, 60, Ret::Float, [] { return Args{{}, {rf(-3, 3), rf(-1, 0.5f), rf(0.5f, 2)}}; },
          [](A a) { return rf32(clampf(a.f[0], a.f[1], a.f[2])); });
    check("bytes_sum", 0x001ff7b8, 40, Ret::Int32, [] { wrand(T, 0x10); return Args{{T, u(ri(1, 0x40))}, {}}; },
          [](A a) { return ri32(bytes_sum(a.i[0], static_cast<int32_t>(a.i[1]))); });
    // the overlay's sounds: slots on the level, near the camera
    auto slots = [lib] {
        lib();
        seed_rand();
        const uint32_t first = ee_read<uint32_t>(OV(0x0015ffd8));
        const uint32_t cm = first + 0x100 * u(ri(0, 150));
        for (int k = 0; k < 3; ++k) wf(OV(0x00166dc0) + 4 * u(k), ee_read<float>(cm + 0x10 + 4 * u(k)) + rf(-4, 4));
        for (uint32_t k = 0; k < 6; ++k) {  // sound definitions: range +0 / +4, volumes +8 / +0xc, flags, sound, bank
            const uint32_t d = T + k * 0x20;
            const float lo = rf(1, 20);
            wf(d, lo); wf(d + 4, lo + rf(5, 60)); w32(d + 8, u(ri(0, 0x100))); w32(d + 0xc, u(ri(0x200, 0x400)));
            w8(d + 0x18, ri(0, 2) == 0 ? 1 : 0); w8(d + 0x19, R() & 0xf); w16(d + 0x1a, R() & 0xff); w32(d + 0x1c, 0x00100000);
        }
        for (uint32_t k = 0; k < 4; ++k) for (uint32_t c = 0; c < 3; ++c) wf(0x0013e550 + k * 0x10 + c * 4, ee_read<float>(OV(0x00166dc0) + c * 4) + rf(-1, 1));
        w32(0x0013e590, u(ri(0, 3)));
        for (uint32_t k = 0; k < 6; ++k) w32(0x0013e598 + k * 4, u(ri(0, 0x400)));
        w8(0x0013e5bb, R()); w32(0x0013e5b4, R()); w8(0x0013e5b8, R()); w8(0x0013e5b9, R()); w8(0x0013e5ba, R());
        w32(OV(0x00167014), ri(0, 3) == 0 ? 1u : 0u); wf(0x0013f640, rf(-50, 50));
        const int md = ri(0, 5);
        w32(OV(0x0015f5c4), md < 3 ? 0u : md == 3 ? 2u : md == 4 ? 6u : 1u);
        w32(OV(0x0015f5a8), ri(0, 6) == 0 ? 1u : 0u);
        w32(OV(0x0015f5cc), R() & 0xff); w32(0x0013f2e8, coin() ? ee_read<uint32_t>(OV(0x0015f5cc)) : R());
        for (uint32_t i = 0; i < 30; ++i) {
            const uint32_t e = 0x0013e5c0 + i * 0x70;
            static const uint8_t states[] = {0, 1, 2, 2, 4, 6, 7, 7};
            w8(e + 4, states[ri(0, 7)]);
            w32(e, ri(0, 4) == 0 ? (coin() ? 0u : 0xffffffffu) : (R() | 1));
            w8(e + 5, R());
            w32(e + 8, T + u(ri(0, 5)) * 0x20);
            w32(e + 0x10, u(ri(0, 0x400))); w32(e + 0x14, ri(0, 3) == 0 ? u(ri(-100, 100)) : 0u);
            uint32_t mo = 0;
            if (ri(0, 2) == 0) { mo = first + 0x100 * u(ri(0, 150)); if (ri(0, 5) == 0) w8(mo + 0x20, 0xfe); }
            w32(e + 0x18, mo); w32(e + 0x1c, R());
            for (uint32_t c = 0; c < 3; ++c) wf(e + 0x20 + c * 4, ee_read<float>(OV(0x00166dc0) + c * 4) + rf(-40, 40));
            wf(e + 0x2c, 1.0f);
            wfrand(e + 0x30, 3, -2, 2);
            w32(e + 0x40, u(ri(0, 0x23)));
            for (uint32_t b = 0; b < 0x24; ++b) w8(e + 0x44 + b, ri(0, 3) == 0 ? 1 : 0);
        }
    };
    // the level's sound emitters: the camera near one, timers and slots varied
    auto emitters = [slots] {
        slots();
        const uint32_t n = ee_read<uint32_t>(0x0013f2e0), base = ee_read<uint32_t>(0x0013f2e4);
        for (uint32_t i = 0; i < n; ++i) {
            const uint32_t prm = ee_read<uint32_t>(base + i * 0x90 + 8);
            w32(prm + 0xc, coin() ? 0u : u(ri(-1, 3)));
            w32(prm + 0x10, ri(0, 3) == 0 ? 0xffffffffu : u(ri(0, 29)));
        }
        const uint32_t em = base + u(ri(0, static_cast<int32_t>(n) - 1)) * 0x90;
        const float r = ee_read<float>(em + 0xc);
        for (uint32_t c = 0; c < 3; ++c) wf(OV(0x00166dc0) + c * 4, ee_read<float>(em + 0x40 + c * 4) + rf(-r - 3, r + 3));
        if (ri(0, 3) == 0) {  // its slot playing it
            const uint32_t prm = ee_read<uint32_t>(em + 8);
            const uint32_t e = 0x0013e5c0 + u(ri(0, 29)) * 0x70;
            w32(prm + 0x10, (e - 0x0013e5c0) / 0x70); w32(e + 0x1c, em); w8(e + 4, coin() ? 2u : 0u);
        }
        return em;
    };
    check("emitter_sphere_update", 0x002ed950, 80, Ret::None, [emitters] {
        uint32_t em = 0;
        for (int k = 0; k < 20; ++k) { em = emitters(); if (ee_read<uint32_t>(em + 4) == OVF(0x002ed950)) break; }
        return Args{{em}, {}};
    }, [](A a) { emitter_sphere_update(a.i[0]); return uint64_t{0}; });
    check("emitter_box_update", 0x002edad8, 80, Ret::None, [emitters] {
        uint32_t em = 0;
        for (int k = 0; k < 20; ++k) { em = emitters(); if (ee_read<uint32_t>(em + 4) == OVF(0x002edad8)) break; }
        return Args{{em}, {}};
    }, [](A a) { emitter_box_update(a.i[0]); return uint64_t{0}; });
    check("level_sound_play", 0x0028ddd8, 40, Ret::Int32, [emitters] {
        const uint32_t em = emitters();
        return Args{{u(ri(-1, static_cast<int32_t>(ee_read<uint32_t>(OV(0x0015f5f0))))), coin() ? 4u : 0u, em, u(ri(0, 0x400))}, {}};
    }, [](A a) { return ri32(level_sound_play(static_cast<int32_t>(a.i[0]), a.i[1], a.i[2], static_cast<int32_t>(a.i[3]))); });
    check("sound_emitters_update", 0x0028dfc8, 40, Ret::None, [emitters] { emitters(); return Args{}; },
          [](A) { sound_emitters_update(); return uint64_t{0}; });
    check("sound_frame", 0x0028cc08, 60, Ret::Int32, [slots] { slots(); return Args{}; }, [](A) { return ri32(sound_frame()); });
    check("sound_listener_point", 0x0028c760, 40, Ret::None, [slots] { slots(); return Args{{OUT}, {}}; },
          [](A a) { sound_listener_point(a.i[0]); return uint64_t{0}; });
    check("sound_line_blocked", 0x0028c810, 60, Ret::Int32, [slots] {
        slots();
        for (uint32_t c = 0; c < 3; ++c) wf(OUT + c * 4, ee_read<float>(OV(0x00166dc0) + c * 4) + rf(-6, 6));
        return Args{{0x0013e5c0 + u(ri(0, 29)) * 0x70, OUT}, {}};
    }, [](A a) { return ri32(static_cast<int32_t>(static_cast<int8_t>(sound_line_blocked_v(a.i[0], VV(a.i[1]))))); });
    check("sound_pan", 0x0028c9e8, 60, Ret::Int32, [slots] {
        slots();
        wfrand(OUT, 16, -1, 1);
        const uint32_t e = 0x0013e5c0 + u(ri(0, 29)) * 0x70;
        return Args{{e, e + 0x20, OUT}, {}};
    }, [](A a) { return ri32(sound_pan(a.i[0], a.i[1], MM(a.i[2]))); });
    check("sound_stop_all", 0x0028e048, 20, Ret::None, [slots] { slots(); return Args{}; }, [](A) { sound_stop_all(); return uint64_t{0}; });
    check("snd_play_reply", 0x0028e108, 40, Ret::None, [slots] {
        slots();
        return Args{{coin() ? 0u : R(), coin() ? 0u : 0x0013e5c0 + u(ri(0, 29)) * 0x70}, {}};
    }, [](A a) { snd_play_reply(static_cast<int32_t>(a.i[0]), static_cast<uint64_t>(sx(a.i[1]))); return uint64_t{0}; });
    check("snd_set_reply", 0x0028e150, 40, Ret::None, [slots] {
        slots();
        return Args{{coin() ? 0u : R(), coin() ? 0u : 0x0013e5c0 + u(ri(0, 29)) * 0x70}, {}};
    }, [](A a) { snd_set_reply(static_cast<int32_t>(a.i[0]), static_cast<uint64_t>(sx(a.i[1]))); return uint64_t{0}; });
    check("snd_reply_store", 0x0028e0f0, 20, Ret::None, [] { return Args{{R(), coin() ? 0u : OUT}, {}}; },
          [](A a) { snd_reply_store(static_cast<int32_t>(a.i[0]), static_cast<uint64_t>(sx(a.i[1]))); return uint64_t{0}; });
    g_extra_noop_hooks.clear();
}

}  // namespace rac1::oracle
