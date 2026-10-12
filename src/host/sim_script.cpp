// (testing) RAC1_SIM_INPUT=seq:<script>: a timed pad script for headless runs (see sim_script.h).
#include "sim_script.h"

#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>

#include <cstdio>

#include <cstdlib>
#include <string>
#include <vector>

namespace rac1 {

namespace {

struct Segment {
    uint64_t first = 0, last = 0;
    uint16_t buttons = 0;
    bool set[4] = {};
    float axis[4] = {};
};

uint16_t button_bit(const std::string& n) {
    static const struct { const char* name; uint16_t bit; } k[] = {
        {"l2", 0x0001}, {"r2", 0x0002}, {"l1", 0x0004}, {"r1", 0x0008}, {"t", 0x0010}, {"o", 0x0020}, {"x", 0x0040}, {"s", 0x0080},
        {"se", 0x0100}, {"l3", 0x0200}, {"r3", 0x0400}, {"st", 0x0800}, {"u", 0x1000}, {"r", 0x2000}, {"d", 0x4000}, {"l", 0x8000},
    };
    for (const auto& b : k)
        if (n == b.name) return b.bit;
    return 0;
}

std::vector<Segment> parse(const char* s) {
    std::vector<Segment> out;
    const std::string all(s);
    size_t pos = 0;
    while (pos <= all.size()) {
        size_t end = all.find(';', pos);
        if (end == std::string::npos) end = all.size();
        const std::string seg = all.substr(pos, end - pos);
        pos = end + 1;
        const size_t colon = seg.find(':');
        if (colon == std::string::npos) continue;
        Segment g;
        char* rest = nullptr;
        g.first = std::strtoull(seg.c_str(), &rest, 10);
        if (*rest == '-') g.last = std::strtoull(rest + 1, nullptr, 10);
        else if (*rest == '+') g.last = g.first + std::strtoull(rest + 1, nullptr, 10);
        else g.last = g.first + 1;
        const std::string in = seg.substr(colon + 1);
        size_t p = 0;
        while (p <= in.size()) {
            size_t e = in.find('+', p);
            if (e == std::string::npos) e = in.size();
            const std::string tok = in.substr(p, e - p);
            p = e + 1;
            const size_t eq = tok.find('=');
            if (eq != std::string::npos) {
                static const char* axes[] = {"lx", "ly", "rx", "ry"};
                for (int a = 0; a < 4; ++a)
                    if (tok.compare(0, eq, axes[a]) == 0) {
                        g.set[a] = true;
                        g.axis[a] = std::strtof(tok.c_str() + eq + 1, nullptr);
                    }
            } else {
                g.buttons |= button_bit(tok);
            }
        }
        out.push_back(g);
    }
    return out;
}

}  // namespace

// orig: (host) the timed pad script
void sim_script(const char* script, uint64_t frame, uint16_t& held, float& lx, float& ly, float& rx, float& ry) {
    static const std::vector<Segment> segs = parse(script);
    // RAC1_SEQ_TRACE=<n>: every n frames the hero's state (0x1413d4), position, the game mode (0x15f5c4), the level (0x15ed84), the bolts (0x15ed98)
    static const int trace = std::getenv("RAC1_SEQ_TRACE") != nullptr ? std::atoi(std::getenv("RAC1_SEQ_TRACE")) : 0;
    if (trace > 0 && frame % static_cast<uint64_t>(trace) == 0) {
        const uint32_t hm = static_cast<uint32_t>(lw(0x001413d0));
        std::printf("[seq] frame %llu state %d pos %.2f %.2f %.2f mode %d level %d bolts %d tgt %08x/%d item %d/%d/%08x\n", static_cast<unsigned long long>(frame), lw(0x001413d4),
                    hm ? lwc1(hm + 0x10) : 0.0f, hm ? lwc1(hm + 0x14) : 0.0f, hm ? lwc1(hm + 0x18) : 0.0f, lw(OV(0x0015f5c4)), lw(0x0015ed84),
                    lw(0x0015ed98), static_cast<uint32_t>(lw(0x0013f350 + 0x968)), lw(0x0013f350 + 0x99c) >> 16, lw(0x0013f350 + 0x10b8), lw(0x0013f350 + 0x10b4), static_cast<uint32_t>(lw(0x0013f350 + 0x1090)));  // (the swingshot target +0x968, +0x99e)
        // the Suck Cannon in hand (item 9): the mobys it holds (the hero's carried count 0x1413c8)
        if (lw(0x0013f350 + 0x10b8) == 9)
            std::printf("[seq-suck] frame %llu carried %d\n", static_cast<unsigned long long>(frame), lw(0x001413c8));
        // the hero as Clank (form 0x1413f4 == 1): the form, the Gadgebots' command 0x141610, the HUD slot 3's update
        if ((lw(0x001413f4) & 0xff) != 0)
            std::printf("[seq-clank] frame %llu form %d cmd %d hud3 %08x\n", static_cast<unsigned long long>(frame), lw(0x001413f4) & 0xff,
                        lw(0x00141610), static_cast<uint32_t>(lw(OV(0x0017dc50) + 3 * 0x90 + 0x14)));
        // and each moby class the first time one is alive (the placed ones at the first trace, then what the level spawns)
        static std::vector<bool> seen(0x10000, false);
        for (uint32_t m = static_cast<uint32_t>(lw(OV(0x0015ffd8))); m < static_cast<uint32_t>(lw(OV(0x0015ffe0))); m += 0x100) {
            const uint32_t st = static_cast<uint32_t>(lw(m + 0x20)) & 0xff;
            const uint32_t cls = static_cast<uint32_t>(lw(m + 0xa4)) >> 16;
            if (st >= 0xfd || seen[cls]) continue;
            seen[cls] = true;
            std::printf("[seq-class] frame %llu class %u moby %08x fn %08x pos %.1f %.1f %.1f\n", static_cast<unsigned long long>(frame), cls, m,
                        static_cast<uint32_t>(lw(m + 0x74)), lwc1(m + 0x10), lwc1(m + 0x14), lwc1(m + 0x18));
        }
        // RAC1_SEQ_WATCH=<class>: that class's mobys each trace: state, +0xbc, position, update flags +0x34, the first pvars
        // words; and the hero-freeze word 0x15f404, the active camera
        static const int watch = std::getenv("RAC1_SEQ_WATCH") != nullptr ? std::atoi(std::getenv("RAC1_SEQ_WATCH")) : -1;
        if (watch >= 0) {
            std::printf("[seq-watch] frame %llu 0x15f404 %d\n", static_cast<unsigned long long>(frame), lw(OV(0x0015f404)));
            for (uint32_t m = static_cast<uint32_t>(lw(OV(0x0015ffd8))); m < static_cast<uint32_t>(lw(OV(0x0015ffe0))); m += 0x100) {
                if (static_cast<int>(static_cast<uint32_t>(lw(m + 0xa4)) >> 16) != watch) continue;
                const uint32_t pv = static_cast<uint32_t>(lw(m + 0x78));
                std::printf("[seq-watch]   moby %08x state %u bc %u flags %04x pos %.1f %.1f %.1f pv", m, static_cast<uint32_t>(lw(m + 0x20)) & 0xff,
                            static_cast<uint32_t>(lw(m + 0xbc)) & 0xff, static_cast<uint32_t>(lw(m + 0x34)) & 0xffff, lwc1(m + 0x10), lwc1(m + 0x14), lwc1(m + 0x18));
                for (uint32_t k = 0; pv != 0 && k < 0x40; k += 4) std::printf(" %08x", static_cast<uint32_t>(lw(pv + k)));
                std::printf("\n");
            }
        }
    }
    // RAC1_MUSIC_TRACE=1: the music streams' state block 0x1516d0 (music_streams.h) each frame it changes: the sequence
    // step +0x20, the next track / the track faded over to +0x22 / +0x23, the 7 s timer +0x2c running (1), the music slot +0x34 (handle,
    // track, volume, flags, state, pause request), the second music slot +0x6c (handle, track, state, +0x10) and the dialog slot +0x50
    static const bool music_trace = std::getenv("RAC1_MUSIC_TRACE") != nullptr;
    if (music_trace) {
        constexpr uint32_t S = 0x001516d0;
        char line[256];
        std::snprintf(line, sizeof line, "seq %d next %d over %d timer %d | A %08x trk %d vol %d fl %d st %d pz %04x | C %08x trk %d st %d f %d | B %08x st %d",
                      lh(S + 0x20), static_cast<int8_t>(lbu(S + 0x22)), static_cast<int8_t>(lbu(S + 0x23)), lw(S + 0x2c) > 0 ? 1 : 0,
                      lwu(S + 0x34), lh(S + 0x38), lh(S + 0x3a), lh(S + 0x3c), lh(S + 0x3e), lhu(S + 0x40), lwu(S + 0x6c), lh(S + 0x70),
                      lh(S + 0x76), lh(S + 0x7c), lwu(S + 0x50), lh(S + 0x5a));
        static std::string last;
        if (last != line) {
            last = line;
            std::printf("[music] frame %llu %s\n", static_cast<unsigned long long>(frame), line);
        }
    }
    float* ax[4] = {&lx, &ly, &rx, &ry};
    for (const Segment& g : segs) {
        if (frame < g.first || frame >= g.last) continue;
        held |= g.buttons;
        for (int a = 0; a < 4; ++a)
            if (g.set[a]) *ax[a] = g.axis[a];
    }
}

}  // namespace rac1
