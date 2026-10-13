// Oracle batch 74: the music and dialog streams (the overlay's 0x265aa8 and its callees, 989snd's
// stream calls) on Veldin's real level data (needs the ISO).

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/music_streams.h>

namespace rac1::oracle {

void batch_74() {
    if (g_only_batch != 74) return;
    g_batch = 74;
    g_ignored_words = vcross_lanes();
    constexpr uint32_t S = 0x001516d0, SA = S + 0x34, SB = S + 0x50, SC = S + 0x6c;
    static std::vector<uint8_t> snap;
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            level_setup_body(1, 0);
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    // a few library calls batched or held before (the replies of earlier batches pending)
    auto history = [] {
        const int n = ri(0, 3) == 0 ? ri(0, 40) : ri(0, 4);
        if (ri(0, 2) == 0) snd_hold();
        for (int k = 0; k < n; ++k) {
            switch (ri(0, 4)) {
            case 0: snd_set_master_volume(R() & 7, ri(0, 0x400)); break;
            case 1: snd_flush(); break;
            case 2: snd_stream_remaining(R(), OVF(0x002663d8), coin() ? SA : SB); break;
            case 3: snd_sound_is_playing(R(), OVF(0x00266380), coin() ? SC : 0u); break;
            default: if (ri(0, 4) == 0) snd_release(); break;
            }
        }
    };
    // a stream slot: handle, state (paused or not), pause request and its timer, flags
    auto slot = [](uint32_t r) {
        static const uint32_t states[] = {0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9};
        const int h = ri(0, 5);
        w32(r, h == 0 ? 0u : h == 1 ? 0xffffffffu : (R() | 1));
        w16(r + 4, u(ri(0, 3)));
        w16(r + 6, u(ri(0, 0x400)));
        w16(r + 8, R() & 3);
        w16(r + 0xa, states[ri(0, 10)] | (ri(0, 4) == 0 ? 0x8000u : 0u));
        const int p = ri(0, 3);
        w16(r + 0xc, p == 0 ? 0x8000u : p == 1 ? 4u : 0u);
        w16(r + 0xe, u(ri(-1, 3)));
        w16(r + 0x10, ri(0, 2) == 0 ? 1u : 0u);
        w32(r + 0x14, u(ri(0, 0x10000)));
        w32(r + 0x18, u(ri(0, 0x10000)));
    };
    // the streams' state (see music_streams.h): the stream system up (the disc reads by command
    // 0x38), the disc idle unless a read is awaited, the level's music table real or varied
    auto streams = [level, history, slot] {
        level();
        history();
        w32(0x0015ec8c, 1);
        w32(0x00137b00, ri(0, 3) == 0 ? 1u : 0u);
        w32(0x00137b10, coin() ? 0u : R());
        w8(S + 0xb, ri(0, 8) == 0 ? 1u : 0u);
        w16(S + 8, u(ri(0, 2)));
        w8(S + 0xa, ri(0, 3) == 0 ? 1u : 0u);
        w32(S + 0xc, R() & NOT_ADDR(0x1fffff));
        w32(S + 0x10, u(ri(0, 8)));
        w32(S + 0x14, OUT);
        w32(S + 0x1c, ri(0, 3) == 0 ? u(ri(0, 6) * 10000 + ri(0, 40)) : 0xffffffffu);
        w16(S + 0x20, u(ri(0, 5)));
        w8(S + 0x22, ri(0, 2) == 0 ? 0xffu : u(ri(0, 3)));
        w8(S + 0x23, ri(0, 2) == 0 ? 0xffu : u(ri(0, 3)));
        w32(S + 0x24, u(ri(-100, 5000)));
        w32(S + 0x28, u(ri(1, 5000)));
        w32(S + 0x2c, u(ri(-1, 3)));
        slot(SA);
        slot(SB);
        slot(SC);
        if (ri(0, 3) == 0) for (uint32_t k = 0; k < 4; ++k) w32(0x0013a628 + k * 4, coin() ? 0u : (R() & NOT_ADDR(0x1fffff)));
        w32(0x0015ed88, u(ri(0, 5)));
        w8(0x0015ee1c, R());
        // a failed read retried (S+8 = 2) needs the disc free: else the original halts (0x1ff008)
        if (ee_read<int16_t>(S + 8) == 2 && ee_read<uint8_t>(S + 0xa) == 0) w32(0x00137b00, 0);
    };
    check("music_streams_frame", 0x00265aa8, 120, Ret::None, [streams] { streams(); return Args{}; },
          [](A) { music_streams_frame(); return uint64_t{0}; });
    check("music_streams_frame_seq", 0x00265aa8, 120, Ret::None, [streams] {  // the music's sequence steps 2..5
        streams();
        w8(S + 0xb, 0);
        w16(S + 0x20, u(ri(2, 5)));
        w16(SA + 0xa, coin() ? 0u : 3u);
        w16(SA + 0xc, 0);
        w16(SC + 0xc, 0);
        w16(SC + 0xa, coin() ? 4u : u(ri(0, 3)));
        if (coin()) w32(SA, R() | 1);
        if (coin()) w32(SC, 0);
        return Args{};
    }, [](A) { music_streams_frame(); return uint64_t{0}; });
    check("stream_slot_update", 0x002658c0, 80, Ret::None, [streams] {
        streams();
        static const uint32_t rs[] = {SA, SB, SC};
        return Args{{rs[ri(0, 2)]}, {}};
    }, [](A a) { stream_slot_update(a.i[0]); return uint64_t{0}; });
    auto track = [streams] {
        streams();
        if (coin()) w32(SA, 0);
        return Args{{u(ri(0, 3)), R() & 3, u(ri(0, 0x400))}, {}};
    };
    check("music_start", 0x00265418, 40, Ret::None, track, [](A a) { music_start(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2])); return uint64_t{0}; });
    check("music_preload", 0x00265340, 40, Ret::None, track, [](A a) { music_preload(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2])); return uint64_t{0}; });
    check("music_queue_next", 0x002654f0, 40, Ret::None, track, [](A a) { music_queue_next(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2])); return uint64_t{0}; });
    check("music2_start", 0x002655d8, 40, Ret::Int32, [streams] {
        streams();
        if (coin()) w32(SC, 0);
        return Args{{u(ri(0, 3)), u(ri(0, 3)), R() & 3, u(ri(0, 0x400))}, {}};
    }, [](A a) { return ri32(music2_start(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2]), static_cast<int32_t>(a.i[3]))); });
    // the dialog lines: ids of every range, the slot free half the time
    auto line = [streams](int lo, int hi) {
        return [streams, lo, hi] {
            streams();
            if (coin()) w32(SB, 0);
            return Args{{u(ri(lo, hi)), R() & 3, u(ri(0, 0x400))}, {}};
        };
    };
    auto dp = [](void (*f)(int32_t, int32_t, int32_t)) {
        return [f](A a) { f(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2])); return uint64_t{0}; };
    };
    check("dialog_play", 0x00265148, 80, Ret::None, [streams] {
        streams();
        if (coin()) w32(SB, 0);
        return Args{{u(ri(0, 6) * 10000 + ri(0, 60)), R() & 3, u(ri(0, 0x400))}, {}};
    }, dp(dialog_play));
    check("dialog_play_60000", 0x00264c18, 30, Ret::None, line(60000, 60100), dp(dialog_play_60000));
    check("dialog_play_50000", 0x00264cf0, 30, Ret::None, line(50000, 50100), dp(dialog_play_50000));
    check("dialog_play_40000", 0x00264dd8, 30, Ret::None, line(40000, 40100), dp(dialog_play_40000));
    check("dialog_play_30000", 0x00264eb0, 30, Ret::None, line(30000, 30100), dp(dialog_play_30000));
    check("dialog_play_20000", 0x00264fa8, 30, Ret::None, line(20000, 20100), dp(dialog_play_20000));
    check("dialog_play_10000", 0x00265078, 30, Ret::None, line(10000, 10100), dp(dialog_play_10000));
    check("stream_disc_read", 0x00265fa0, 40, Ret::Int32, [streams] {
        streams();
        w32(0x00137b00, 0);  // the disc free (a refused read halts the original)
        return Args{{OUT, R() & NOT_ADDR(0x1fffff), u(ri(0, 8))}, {}};
    }, [](A a) { return ri32(stream_disc_read(a.i[0], a.i[1], a.i[2])); });
    check("music_stop_all", 0x002656c0, 30, Ret::None, [streams] {
        streams();
        for (uint32_t r : {SA, SB, SC}) if (ee_read<uint32_t>(r) == 0xffffffffu) w32(r, 0);  // no reply awaited
        return Args{};
    }, [](A) { music_stop_all(); return uint64_t{0}; });
    // the replies' callbacks
    auto reply = [streams] {
        streams();
        static const uint32_t rs[] = {SA, SB, SC, 0};
        const int k = ri(0, 3);
        return Args{{k == 0 ? 0u : k == 1 ? 0xffffffffu : k == 2 ? 1u : R(), rs[ri(0, 3)]}, {}};
    };
    auto cb = [](void (*f)(int32_t, uint64_t)) {
        return [f](A a) { f(static_cast<int32_t>(a.i[0]), static_cast<uint64_t>(sx(a.i[1]))); return uint64_t{0}; };
    };
    check("stream_read_done", 0x00266168, 30, Ret::None, [streams] { streams(); return Args{{u(ri(0, 2)), 0}, {}}; }, cb(stream_read_done));
    check("stream_started_reply", 0x002661a8, 30, Ret::None, reply, cb(stream_started_reply));
    check("dialog_play_reply", 0x002661d8, 40, Ret::None, reply, cb(dialog_play_reply));
    check("music_preload_reply", 0x00266238, 40, Ret::None, reply, cb(music_preload_reply));
    check("music2_play_reply", 0x00266298, 30, Ret::None, reply, cb(music2_play_reply));
    check("music_queue_reply", 0x002662e8, 30, Ret::None, reply, cb(music_queue_reply));
    check("music_play_reply", 0x00266340, 30, Ret::None, reply, cb(music_play_reply));
    check("stream_playing_reply", 0x00266380, 30, Ret::None, reply, cb(stream_playing_reply));
    check("stream_remaining_reply", 0x002663d8, 40, Ret::None, reply, cb(stream_remaining_reply));
    // 989snd's EE stream calls
    check("snd_stream_play", 0x0012ec08, 40, Ret::None, [streams] {
        streams();
        return Args{{R() & NOT_ADDR(0x1fffff), coin() ? 0u : R(), R() & 0xffff, R() & 0xffff, sx16(R()), sx16(R()), R() & 7, coin() ? 0u : R(), R() & 0x3f, coin() ? 0u : OVF(0x002661d8u), SB}, {}};
    }, [](A a) {
        snd_stream_play(a.i[0], a.i[1], a.i[2], a.i[3], static_cast<int32_t>(a.i[4]), static_cast<int32_t>(a.i[5]), a.i[6], a.i[7], a.i[8], a.i[9], static_cast<uint64_t>(sx(a.i[10])));
        return uint64_t{0};
    });
    check("snd_stream_pause", 0x0012ec70, 20, Ret::None, [streams] { streams(); return Args{{R()}, {}}; }, [](A a) { snd_stream_pause(a.i[0]); return uint64_t{0}; });
    check("snd_stream_remaining", 0x0012ecd0, 20, Ret::None, [streams] { streams(); return Args{{R(), OVF(0x002663d8), SC}, {}}; },
          [](A a) { snd_stream_remaining(a.i[0], a.i[1], static_cast<uint64_t>(sx(a.i[2]))); return uint64_t{0}; });
    check("snd_stream_started", 0x0012ed00, 20, Ret::None, [streams] { streams(); return Args{{R(), OVF(0x002661a8), SA}, {}}; },
          [](A a) { snd_stream_started(a.i[0], a.i[1], static_cast<uint64_t>(sx(a.i[2]))); return uint64_t{0}; });
    check("snd_stream_reads_wait", 0x0012ed30, 20, Ret::Int32, [streams] { streams(); return Args{{u(ri(0, 1))}, {}}; },
          [](A a) { return ri32(snd_stream_reads_wait(a.i[0])); });
    check("snd_stream_stop_all", 0x0012ebd0, 20, Ret::None, [streams] { streams(); return Args{}; }, [](A) { snd_stream_stop_all(); return uint64_t{0}; });
    check("snd_stream_cd_read", 0x0012ed58, 30, Ret::Int32, [streams] { streams(); return Args{{R() & NOT_ADDR(0x1fffff), u(ri(1, 8)), OUT, S + 0x30}, {}}; },
          [](A a) { return ri32(snd_stream_cd_read(a.i[0], a.i[1], a.i[2], a.i[3])); });
    check("snd_cd_error", 0x0012eef0, 20, Ret::Int32, [streams] { streams(); return Args{}; }, [](A) { return ri32(snd_cd_error()); });
}

}  // namespace rac1::oracle
