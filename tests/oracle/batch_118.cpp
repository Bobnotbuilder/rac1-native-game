// Oracle batch 118 (task 0351): the screen fade 0x1f9838 (Novalis 0x21b438, every level's own copy: run with
// ORACLE_LEVEL=<level> build/tests/ovl_oracle --batch 118).
//
// 0. vsync_callback 0x12f1c8 (task 0354): the game's vsync callback, the vsync count 0x15ed48 and the time 0x15ed50.
// 1. screen_fade: the whole call, the original (its vsync wait 0x122298 and DMA kick 0x2a08c8 no-ops, as the host's
//    frame boundary and the host's send) against the port run outside the game's coroutine (its frames inside the call),
//    on the level after a gameplay frame (the packet cursor mid-frame), with n = -2..24, the buffers' index, the frame
//    counter and the cursor's place varied: every word of RAM compared.
// 2. screen_fade_frames: the same cases, the port run on the game's coroutine (runtime/ee/ee_thread.h) a host frame at a
//    time, as rac1-native runs it: at each of its suspensions the RAM's hash against the original's at each of its vsync
//    waits (a host hook on 0x122298), the number of waits (n + 2, at least 2) and the kinds of the frames (held, kicked
//    n times, held); then the final RAM as in 1. The frame counter 0x15f3f8 must have moved by n + 2 (at least 2).
//    ("Original fades" on: advanced_original_fades, task 0354.)
// 3. screen_fade_fast (task 0354, the default): the same cases on the game's coroutine with "Original fades" off: the
//    fade ends inside one ee_thread_run (no suspension), the host's vsync hook called n + 2 times (the waits: the sound's
//    time), the final RAM equal to the original's (the hash, and the frame counter moved by n + 2).

#include "ovl_oracle.h"

#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/screen_fade.h>
#include <game/enhance/advanced.h>
#include <runtime/ee/ee_thread.h>
#include <runtime/ee/ee_timer.h>
#include <game/ovl/vsync_callback.h>

#include <cstring>
#include <vector>

namespace rac1::oracle {

namespace {
// RAM below the interpreter's stack (0x1fe0000) and the scratchpad, word by word
uint64_t ram_hash() {
    const uint8_t* ram = ee_get_ram_base();
    uint64_t h = 0xcbf29ce484222325ull;
    for (uint32_t a = 0; a < 0x01fe0000u; a += 8) {
        uint64_t w;
        std::memcpy(&w, ram + a, 8);
        h = (h ^ w) * 0x100000001b3ull;
        h ^= h >> 29;
    }
    const uint8_t* spr = ee_get_scratchpad_base();
    for (uint32_t a = 0; a < 0x4000; a += 8) {
        uint64_t w;
        std::memcpy(&w, spr + a, 8);
        h = (h ^ w) * 0x100000001b3ull;
    }
    return h;
}
}  // namespace

void batch_118() {
    if (g_only_batch != 118) return;
    g_batch = 118;
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // as in the runtime
    g_extra_noop_hooks.clear();  // (0x122298 and 0x2a08c8 stay no-ops: oracle_env.cpp)

    // the level after one gameplay frame (the port's), kept as a snapshot
    static std::vector<uint8_t> snap;
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty()) {
            veldin_boot();
            level_setup_body(1, 0);
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            snap.assign(ram, ram + 0x02000000);
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    // a case: the fade's frames, the packet buffers' index / cursor, the frame counter
    auto setup = [level] {
        level();
        static const int32_t ns[] = {-2, -1, 0, 1, 2, 3, 4, 6, 10, 12, 16, 18, 24};
        const int32_t n = coin() ? ns[ri(0, 12)] : ri(1, 20);
        w32(OV(0x0015f3f8), coin() ? R() : u(ri(0, 100000)));
        w32(OV(0x001611a0), 0);  // (no DMA busy: the host's are done at once)
        if (coin()) {  // the other packet buffer current, its cursor somewhere in it
            const uint32_t idx = 1u - ee_read<uint32_t>(OV(0x001611d0));
            w32(OV(0x001611d0), idx);
            w32(OV(0x001611c0), ee_read<uint32_t>(OV(0x001611b8) + idx * 4) + u(ri(0, 0x400)) * 0x10);
        }
        return n;
    };

    check("screen_fade", 0x001f9838, 60, Ret::None, [setup] { return Args{{u(setup())}, {}}; },
          [](A a) { screen_fade(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });

    // the vsync callback 0x12f1c8 (base ELF, task 0354): the vsync count and the time, 64 bit (their carries), T1_COUNT 0
    // on both sides (the interpreter's unmodelled timer, the host's ee_t1 reset)
    check("vsync_callback", 0x0012f1c8, 60, Ret::Int32, [] {
              auto w64 = [](uint32_t a, uint64_t v) { w32(a, static_cast<uint32_t>(v)); w32(a + 4, static_cast<uint32_t>(v >> 32)); };
              w64(0x0015ed40, coin() ? (static_cast<uint64_t>(R()) << 32 | R()) : 0xffffffffull - u(ri(0, 3)));
              w64(0x0015ed48, coin() ? (static_cast<uint64_t>(R()) << 32 | R()) : 0xffffffffull - u(ri(0, 3)));
              w64(0x0015ed50, static_cast<uint64_t>(R()) << 32 | R());
              ee_t1_reset();
              return Args{{}, {}};
          },
          [](A) { return static_cast<uint64_t>(static_cast<uint32_t>(vsync_callback())); });

    // ---- 2. a host frame at a time
    if (const char* only = std::getenv("ORACLE_ONLY"); only == nullptr || std::strcmp(only, "screen_fade_frames") == 0) {
        const uint32_t at = g_oracle_level != 0 ? ovl_code_lookup(0x001f9838) : 0x001f9838u;
        std::printf("%-26s 0x%08x%s\n", "screen_fade_frames", at, g_oracle_level != 0 ? " (the level's copy)" : "");
        advanced_set_original_fades(true);
        int bad = 0, cases = 0;
        for (int c = 0; c < 40; ++c) {
            const int32_t n = setup();
            const uint32_t count0 = ee_read<uint32_t>(OV(0x0015f3f8));
            std::vector<uint8_t> start(ee_get_ram_base(), ee_get_ram_base() + 0x02000000);
            std::vector<uint8_t> start_spr(ee_get_scratchpad_base(), ee_get_scratchpad_base() + 0x4000);
            // the original: the RAM at each vsync wait
            std::vector<uint64_t> orig;
            g_extra_hooks = {{0x00122298, [&orig](EeCpu&) { orig.push_back(ram_hash()); }}};
            call_original(at, {u(n)});
            g_extra_hooks.clear();
            const uint64_t orig_end = ram_hash();
            const uint32_t orig_count = ee_read<uint32_t>(OV(0x0015f3f8));
            std::memcpy(ee_get_ram_base(), start.data(), start.size());
            std::memcpy(ee_get_scratchpad_base(), start_spr.data(), start_spr.size());
            // the port on the coroutine, resumed until it returns
            std::vector<uint64_t> port;
            std::vector<EeVsyncFrame> kinds;
            bool done = ee_thread_run([n] { screen_fade(n); });
            while (!done) {
                port.push_back(ram_hash());
                kinds.push_back(ee_thread_frame());
                if (port.size() > 1000) break;
                done = ee_thread_run([] {});
            }
            const uint64_t port_end = ram_hash();
            const uint32_t want = 2u + static_cast<uint32_t>(n > 0 ? n : 0);
            bool ok = done && port == orig && port_end == orig_end && port.size() == want && ee_read<uint32_t>(OV(0x0015f3f8)) == count0 + want &&
                      orig_count == count0 + want;
            for (size_t k = 0; ok && k < kinds.size(); ++k)
                ok = kinds[k] == ((k == 0 || k + 1 == kinds.size()) ? EeVsyncFrame::Held : EeVsyncFrame::Kicked);
            if (!ok && bad++ < 2) {
                size_t first = 0;
                while (first < port.size() && first < orig.size() && port[first] == orig[first]) ++first;
                std::printf("  case %d: n %d: waits original %zu port %zu (want %u), first different wait %zu, end %s, counter +%u / +%u\n", c, n,
                            orig.size(), port.size(), want, first, port_end == orig_end ? "equal" : "DIFFERENT", orig_count - count0,
                            ee_read<uint32_t>(OV(0x0015f3f8)) - count0);
            }
            ++cases;
        }
        std::printf("%-26s 0x%08x  %s (%d cases, %d bad)\n", "screen_fade_frames", at, bad == 0 ? "ok" : "FAIL", cases, bad);
        if (bad != 0) ++g_fail;
        advanced_set_original_fades(false);
    }

    // ---- 3. the fast fade (the default): one ee_thread_run, the vsync hook n + 2 times
    if (const char* only = std::getenv("ORACLE_ONLY"); only == nullptr || std::strcmp(only, "screen_fade_fast") == 0) {
        const uint32_t at = g_oracle_level != 0 ? ovl_code_lookup(0x001f9838) : 0x001f9838u;
        std::printf("%-26s 0x%08x%s\n", "screen_fade_fast", at, g_oracle_level != 0 ? " (the level's copy)" : "");
        advanced_set_original_fades(false);
        static int hooks = 0;
        int bad = 0, cases = 0;
        for (int c = 0; c < 40; ++c) {
            const int32_t n = setup();
            const uint32_t count0 = ee_read<uint32_t>(OV(0x0015f3f8));
            std::vector<uint8_t> start(ee_get_ram_base(), ee_get_ram_base() + 0x02000000);
            std::vector<uint8_t> start_spr(ee_get_scratchpad_base(), ee_get_scratchpad_base() + 0x4000);
            call_original(at, {u(n)});
            const uint64_t orig_end = ram_hash();
            std::memcpy(ee_get_ram_base(), start.data(), start.size());
            std::memcpy(ee_get_scratchpad_base(), start_spr.data(), start_spr.size());
            hooks = 0;
            ee_thread_set_vsync_hook([](EeVsyncFrame) { ++hooks; });
            const bool done = ee_thread_run([n] { screen_fade(n); });
            ee_thread_set_vsync_hook(nullptr);
            const uint64_t port_end = ram_hash();
            const uint32_t want = 2u + static_cast<uint32_t>(n > 0 ? n : 0);
            const bool ok = done && !ee_thread_suspended() && port_end == orig_end && hooks == static_cast<int>(want) &&
                            ee_read<uint32_t>(OV(0x0015f3f8)) == count0 + want;
            if (!ok && bad++ < 2)
                std::printf("  case %d: n %d: %s, vsyncs %d (want %u), end %s, counter +%u\n", c, n, done ? "ended" : "SUSPENDED", hooks, want,
                            port_end == orig_end ? "equal" : "DIFFERENT", ee_read<uint32_t>(OV(0x0015f3f8)) - count0);
            while (!done && ee_thread_suspended() && ee_thread_run([] {}) == false) {
            }
            ++cases;
        }
        std::printf("%-26s 0x%08x  %s (%d cases, %d bad)\n", "screen_fade_fast", at, bad == 0 ? "ok" : "FAIL", cases, bad);
        if (bad != 0) ++g_fail;
    }
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
