// Oracle batch 143 (task 0406): the loading screen's drawing, game mode 6 step 4 (the ship's flight between planets): 0x2900a0
// (flight_draw) and its callees 0x28b440 (the sky), 0x28f6e0 (the next planet's picture), 0x28f820 (its name), on RAM dumps taken
// during the flight by headless runs (RAC1_RAMDUMP, the level exit's loops; assets/dumps/oracle/flight_0406/, skipped without them):
//   v0to1_400.bin  Veldin -> Novalis (Novalis marked done: the loading screen, variant 4: the planet, its name), Veldin's overlay
//   v0to2_450.bin  Veldin -> Aridia (a random variant: the ship, its riders, its trails), Veldin's overlay
//   v1to2_450.bin  Novalis -> Aridia, Novalis's own overlay (ORACLE_LEVEL=1)
// Recording them (from the worktree root; RAC1_RAMHASH is what writes the dump):
//   RAC1_HEADLESS=1 RAC1_CLOCK=1759658400 RAC1_RAMHASH=/dev/null RAC1_POKE8=100:0x13dd59=2 RAC1_LEVEL=0 RAC1_GOTO=1@300 RAC1_FRAMES=420
//     RAC1_RAMDUMP=400,assets/dumps/oracle/flight_0406/v0to1_400.bin build/rac1-native
//   (the same with RAC1_GOTO=2@300 RAC1_RAMDUMP=450,..v0to2_450.bin, and RAC1_LEVEL=1 RAC1_GOTO=2@300 RAC1_RAMDUMP=450,..v1to2_450.bin, no POKE8)
// Per case the state the drawing reads is varied: the camera's turn (the sky's angles), the field of view (its clamp to 0.63), the
// level number (which picks the planet's size and whether the ship's trails / canopy are drawn), the variant 0x13e088, the scene's
// frame (the name's alpha), the save state (the memory card icon and spinner), the frame counter (the spinner's angle), the fade.
// Every word of EE RAM and the scratchpad compared (the frame packet's chain, the moby pass's buffers, the sky's state).
//
//   build/tests/ovl_oracle --batch 143              (Veldin's overlay: the two v0 dumps)
//   ORACLE_LEVEL=1 build/tests/ovl_oracle --batch 143   (Novalis's own overlay: v1to2)

#include "ovl_oracle.h"

#include <game/ovl/flight_draw.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/vu0_macro.h>
#include <game/render/render_toggles.h>
#include <runtime/ee/ee_interpreter.h>
#include <runtime/vu/vu0.h>

#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace rac1::oracle {

namespace {
// a RAM dump of a headless run (32 MB + the scratchpad), empty when the file is missing
const std::vector<uint8_t>& flight_dump(const std::string& path) {
    static std::map<std::string, std::vector<uint8_t>> cache;
    std::vector<uint8_t>& d = cache[path];
    if (d.empty()) {
        if (FILE* f = std::fopen(path.c_str(), "rb")) {
            d.resize(0x02000000 + 0x4000);
            if (std::fread(d.data(), 1, d.size(), f) != d.size()) d.clear();
            std::fclose(f);
        }
    }
    return d;
}
vu0m::Regs g_vin;  // the VU0 as the original found it (its micro memory: the programs the sky's sprites and the moby pass call)
}  // namespace

void batch_143() {
    if (g_only_batch != 143) return;
    g_batch = 143;
    std::vector<std::string> names;
    if (g_oracle_level == 0) names = {dump_path("flight_0406/v0to1_400.bin"), dump_path("flight_0406/v0to2_450.bin")};
    else if (g_oracle_level == 1) names = {dump_path("flight_0406/v1to2_450.bin")};
    std::vector<const std::vector<uint8_t>*> dumps;
    for (const std::string& n : names)
        if (!flight_dump(n).empty()) dumps.push_back(&flight_dump(n));
    if (dumps.empty()) {
        skip_check("flight_draw", 0x002900a0, "no RAM dump of the loading screen for this level (see the batch's header)");
        return;
    }
    const std::vector<uint32_t> saved_ignored = g_ignored_words;
    g_ignored_words = vcross_lanes();
    // the draw loop's register save area on the scratchpad, 0x2531e0's spill of the caller's t8 / t9 (as batch 79's moby pass)
    for (uint32_t a = 0x70003fa8; a < 0x70004000; a += 4) g_ignored_words.push_back(a);
    for (uint32_t a = 0x70003700; a < 0x70003720; a += 4) g_ignored_words.push_back(a);
    for (uint32_t a = 0x70001ff0; a < 0x70002000; a += 4) g_ignored_words.push_back(a);  // 0x1ee858's saved return address
    for (uint32_t a = OV(0x0019b780); a < OV(0x0019b7e0); a += 4) g_ignored_words.push_back(a);  // 0x2558e0's saved registers
    g_ignored_words.push_back(OV(0x0016044c));  // 0x1ff3c0's w lane: a stale VU0 lane
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // as in the runtime
    const std::vector<uint32_t> hooks = g_extra_noop_hooks;
    g_extra_noop_hooks = {};
    // the port's host switches as the runtime's: the sky's chain built (render_frame's sky_wanted with the MRP), the moby pass run
    const bool forced = g_world_chain_forced, sky = g_render_sky, vu1 = g_moby_vu1_pass;
    g_world_chain_forced = true;
    g_render_sky = true;
    g_moby_vu1_pass = true;
    const auto pre = g_pre_call;
    g_pre_call = [](EeInterpreter& ee) {
        g_vin.vf = ee.vu0().vf;
        g_vin.vi = ee.vu0().vi;
        g_vin.acc = ee.vu0().acc;
        g_vin.q = ee.vu0().q;
        g_vin.i = ee.vu0().i;
        g_vin.mem = ee.vu0().mem;
        ee.set_vu0_coexec(true, 1, 1);
    };

    // one of the dumps, the VU0's micro programs the game uploads (the two VIF0 chains of the ELF), then the state varied
    auto state = [&dumps](bool vary_more) {
        const std::vector<uint8_t>& d = *dumps[static_cast<size_t>(ri(0, static_cast<int>(dumps.size()) - 1))];
        std::memcpy(ee_get_ram_base(), d.data(), 0x02000000);
        std::memcpy(ee_ram_ptr(0x70000000u), d.data() + 0x02000000, 0x4000);
        vu0::vif0_send_chain(0x00100080, true);
        vu0::vif0_send_chain(0x0010e4c0, true);
        if (coin()) wf(OV(0x001604c4), rf(-3.14f, 3.14f));  // the camera's turn the sky's angles start from
        if (coin()) wf(OV(0x0016caf0), ri(0, 2) == 0 ? rf(0.3f, 0.63f) : rf(0.5f, 1.2f));  // the field of view (clamped to 0.63 for the sky)
        if (!vary_more) return;
        if (ri(0, 2) == 0) w32(0x0015ed84, u(ri(0, 20)));  // the level (the planet's size, the ship's trails / canopy)
        if (coin()) w8(0x0013dd43, coin() ? 1 : 0);
        if (ri(0, 2) == 0) w32(0x0013e088, coin() ? 4u : u(ri(0, 3)));  // the variant
        if (coin()) w32(OV(0x0016c894), u(ri(0, 200)));  // the scene's frame (the name's alpha after frame 60)
        if (coin()) w32(OV(0x0015f3f8), R());  // the frame counter (the spinner's angle)
        if (ri(0, 2) == 0) {  // a save running: the memory card's icon and spinner
            w32(0x0013d290 + 0xd4, u(ri(0, 4)));
            w32(0x0013d290 + 0xdc, coin() ? 0xffffffffu : u(ri(0, 3)));
        }
        if (coin()) wf(OV(0x0015f3fc), ri(0, 2) == 0 ? 0.0f : rf(0.0f, 1.5f));  // the fade
    };

    check("flight_banner", 0x0028f820, 30, Ret::None, [&] {
        state(false);
        return Args{{ri(0, 3) == 0 ? 0x80u : u(ri(0, 0x80))}, {}};
    }, [](A a) { flight_banner(static_cast<int32_t>(a.i[0])); return uint64_t{0}; });
    check("flight_planet_picture", 0x0028f6e0, 30, Ret::None, [&] {
        state(false);
        w32(0x0015ed84, u(ri(0, 22)));
        return Args{};
    }, [](A) { vu0m::regs() = g_vin; flight_planet_picture(); return uint64_t{0}; });
    check("flight_sky", 0x0028b440, 30, Ret::None, [&] {
        state(false);
        return Args{};
    }, [](A) { vu0m::regs() = g_vin; flight_sky(); return uint64_t{0}; });
    check("flight_draw", 0x002900a0, 60, Ret::None, [&] {
        state(true);
        return Args{};
    }, [](A) { vu0m::regs() = g_vin; flight_draw(); return uint64_t{0}; });

    g_pre_call = pre;
    g_world_chain_forced = forced;
    g_render_sky = sky;
    g_moby_vu1_pass = vu1;
    g_extra_noop_hooks = hooks;
    g_ee_null_is_ram = null_ram;
    g_ignored_words = saved_ignored;
}

}  // namespace rac1::oracle
