// Oracle batch 241 (levels/quartu-kalebo, release 0.3.0 levels): particle type 0x47's update (Veldin 0x274e48, L15:0x264958), ported at
// Veldin's addresses (assets/generated/src/game/ovl/particle_47_tr.cpp, generated from tools/codegen/gen/mkall/veldin_particle47.json).
// First reached on Quartu (a TODO_PORT moby callback in the first factory hall). ORACLE_LEVEL=15, on RAM dumps of headless runs of the
// port (assets/dumps/oracle/levels_15_16/, skipped without them), from the repository's root:
//   p47_{1000,1200}_L15.bin  RAC1_HEADLESS=1 RAC1_LEVEL=15 RAC1_SIM_INPUT=forward RAC1_FRAMES=<f>+2 RAC1_RAMHASH=/dev/null
//                              RAC1_RAMDUMP=<f>,assets/dumps/oracle/levels_15_16/p47_<f>_L15.bin (Ratchet walking into the first hall: the
//                              type-0x47 particles live).
// Each case takes one live type-0x47 particle of the dump and varies what the update reads: its timer (+0xa), its size (+0xc), its
// flag byte (+0x38) and the random seed; the positions are the dump's. Not proved: the update's near branch (a particle moved within 5
// of the point it measures from, 0x13f420): there the interpreter's original returns offsets of 1e17 .. 1e18 in one lane (its VU0
// vrsqrt / ACC sequence of 0x1ff500 on that state; the port's are about the distance), so those cases are left out. Its +0x20.. words are packed
// directions (dir_pack 0x261480 / dir_unpack 0x2615a8), not floats: kept as the dump has them.
#include "ovl_oracle.h"

#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/particle_47_tr.h>
#include <runtime/ee/ee_interpreter.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace rac1::oracle {

namespace {
const std::vector<uint8_t>& p47_dump(const std::string& path) {
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
void p47_load(const std::vector<uint8_t>& d) {
    std::memcpy(ee_get_ram_base(), d.data(), 0x02000000);
    std::memcpy(ee_ram_ptr(0x70000000u), d.data() + 0x02000000, 0x4000);
}
std::vector<uint32_t> live_particles(uint8_t type) {
    std::vector<uint32_t> out;
    const uint32_t base = ee_read<uint32_t>(OV(0x0016016c));
    const int32_t hi = ee_read<int32_t>(OV(0x00160174));
    for (int32_t i = 0; i <= hi && i < 4096; ++i) {
        const uint32_t p = base + u(i) * 64;
        if (ee_read<uint8_t>(p) == type && ee_read<int8_t>(p + 1) >= 0) out.push_back(p);
    }
    return out;
}
}  // namespace

void batch_241() {
    if (g_only_batch != 241) return;
    g_batch = 241;
    const std::vector<uint32_t> saved_ignored = g_ignored_words;
    g_ignored_words = vcross_lanes();
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // (as in the runtime)
    for (const char* dn : {"p47_1000_L15", "p47_1200_L15"}) {
        const std::string name = std::string("particle_47_update ") + dn;
        if (g_oracle_level != 15) {
            skip_check(name.c_str(), 0x00274e48, "the type-0x47 particles' dumps are Quartu's: ORACLE_LEVEL=15");
            continue;
        }
        const std::vector<uint8_t>& d = p47_dump(dump_path("levels_15_16/") + dn + ".bin");
        if (d.empty()) {
            skip_check(name.c_str(), 0x00274e48, "no RAM dump (assets/dumps/oracle/levels_15_16)");
            continue;
        }
        p47_load(d);
        std::printf("%s: %zu live type-0x47 particles\n", dn, live_particles(0x47).size());
        if (live_particles(0x47).empty()) {
            skip_check(name.c_str(), 0x00274e48, "no live type-0x47 particle in the dump");
            continue;
        }
        auto gen = [&d] {
            p47_load(d);
            const std::vector<uint32_t> ps = live_particles(0x47);
            if (ps.empty()) return Args{{0}, {}};
            const uint32_t p = ps[static_cast<size_t>(ri(0, static_cast<int>(ps.size()) - 1))];
            seed_rand();
            { const int k = ri(0, 3); if (k == 0) w16(p + 0xa, 0); else if (k == 1) w16(p + 0xa, u(ri(1, 2))); else if (k == 2) w16(p + 0xa, u(ri(3, 60))); }
            if (coin()) wf(p + 0xc, rf(0.05f, 3.0f));
            if (coin()) w8(p + 0x38, R() & 0xff);
            return Args{{p}, {}};
        };
        check(name.c_str(), 0x00274e48, 60, Ret::None, gen, [](A a) { if (a.i[0] != 0) particle_47_update(a.i[0]); return uint64_t{0}; });
    }
    g_ignored_words = saved_ignored;
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
