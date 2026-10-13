// Oracle batch 125 (task 0363): the world's static lighting VU0 set (micro 0x000..0x0af, MPG'd from 0x100af0 by the VIF0 chain
// 0x100ae0; started by the tie lighting 0x297728 at 0x058 / 0x084 and the shrub lighting 0x288908 at 0x000 / 0x02c), ported by
// tools/codegen/gen_light_vu0.py, against the VU interpreter running the uploaded original:
// - world_light_vu0 (exact): in lock step, the whole state compared after every instruction (as batch 79);
// - world_light_vu0_fast: compared at each call's return (as batch 95: vf / vi / ACC / Q / I, the flags, the data memory, the
//   pipeline) and at the end; random states at every entry in three profiles, and the states the programs start on in the game
//   (RAC1_VU0_CAPTURE of rac1-native into assets/dumps/oracle/vu0_light, generated: not in the repo) as captured and perturbed.
// The routines that call them (0x297728 / 0x288908, whose results the level load and render_frame write) are run end to end with
// the port against the original by batch 101. ORACLE_BENCH=1: the time per instruction, interpreter / exact / fast.

#include "ovl_oracle.h"

#include <game/ovl/world_light_vu0.h>
#include <runtime/vu/vu0.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace rac1::oracle {

namespace {

uint32_t f2u(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }
float u2f(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
bool eqv(const VUVector& x, const VUVector& y) { return std::memcmp(&x, &y, 16) == 0; }

// what differs between the interpreter's run `ra` / state `a` and the port's `rb` / `b` (empty: nothing); VU0's 4 KB of data memory
std::string diff(const VUState& a, const VUState& b, const VuMicroRun& ra, const VuMicroRun& rb) {
    char buf[200];
    std::string d;
    auto add = [&](const char* s) { d += s; d += ' '; };
    for (int r = 0; r < 32; ++r)
        if (!eqv(a.vf[static_cast<size_t>(r)], b.vf[static_cast<size_t>(r)])) {
            std::snprintf(buf, sizeof buf, "vf%d(%08x %08x %08x %08x / %08x %08x %08x %08x)", r, a.vf[r].u[0], a.vf[r].u[1], a.vf[r].u[2], a.vf[r].u[3], b.vf[r].u[0], b.vf[r].u[1],
                          b.vf[r].u[2], b.vf[r].u[3]);
            add(buf);
        }
    for (int r = 0; r < 16; ++r)
        if (a.vi[r] != b.vi[r]) {
            std::snprintf(buf, sizeof buf, "vi%d(%d/%d)", r, a.vi[r], b.vi[r]);
            add(buf);
        }
    if (!eqv(a.acc, b.acc)) add("acc");
    if (f2u(a.q) != f2u(b.q)) {
        std::snprintf(buf, sizeof buf, "q(%08x/%08x)", f2u(a.q), f2u(b.q));
        add(buf);
    }
    if (f2u(a.i) != f2u(b.i)) add("i");
    if (a.macflag != b.macflag) {
        std::snprintf(buf, sizeof buf, "mac(%04x/%04x)", a.macflag, b.macflag);
        add(buf);
    }
    if (a.statusflag != b.statusflag) {
        std::snprintf(buf, sizeof buf, "status(%03x/%03x)", a.statusflag, b.statusflag);
        add(buf);
    }
    if (a.clipflag != b.clipflag) {
        std::snprintf(buf, sizeof buf, "clip(%06x/%06x)", a.clipflag, b.clipflag);
        add(buf);
    }
    if (std::memcmp(a.mem.data(), b.mem.data(), 4096) != 0) {
        for (size_t k = 0; k < 4096; k += 4)
            if (std::memcmp(&a.mem[k], &b.mem[k], 4) != 0) {
                std::snprintf(buf, sizeof buf, "mem@%03zx", k);
                add(buf);
                break;
            }
    }
    const VuPipe &pa = ra.pipe(), &pb = rb.pipe();
    if (pa.pc != pb.pc) {
        std::snprintf(buf, sizeof buf, "pc(%03x/%03x)", pa.pc, pb.pc);
        add(buf);
    }
    if (pa.issued != pb.issued) add("issued");
    if (pa.halted != pb.halted) add("halted");
    if (pa.branch_pending != pb.branch_pending || (pa.branch_pending && pa.branch_target != pb.branch_target)) add("branch");
    if (pa.q_pending != pb.q_pending || (pa.q_pending && (f2u(pa.q_next) != f2u(pb.q_next) || pa.q_ready != pb.q_ready))) add("q-in-flight");
    if (pa.halt_pending != pb.halt_pending || (pa.halt_pending && pa.halt_delay != pb.halt_delay)) add("halt-pending");
    return d;
}

std::vector<uint8_t> g_cover;  // the micro addresses the interpreter ran

// the exact port in lock step with the interpreter: every instruction compared
std::string run_lockstep(VUState& a, VUState& b, const uint64_t* micro, size_t size, uint32_t start, uint64_t* steps) {
    VuMicroRun ra, rb;
    ra.start(a, micro, size, start);
    rb.start(b, micro, size, start, &world_light_vu0_step);
    for (int n = 0; n < 4000; ++n) {
        if (ra.pc() < g_cover.size()) g_cover[ra.pc()] = 1;
        const bool ga = ra.step(), gb = rb.step();
        ++*steps;
        std::string d = diff(a, b, ra, rb);
        if (ga != gb) d += "end ";
        if (!d.empty()) return "after " + std::to_string(n + 1) + " instructions: " + d;
        if (!ga) break;
    }
    ra.finish();
    rb.finish();
    const std::string d = diff(a, b, ra, rb);
    return d.empty() ? std::string() : "at the end: " + d;
}

// the fast port against the interpreter, compared after each call of the port and at the end
std::string run_pair(VUState& a, VUState& b, const uint64_t* micro, size_t size, uint32_t start, uint64_t* steps, uint64_t* calls) {
    VuMicroRun ra, rb;
    ra.start(a, micro, size, start);
    rb.start(b, micro, size, start, &world_light_vu0_fast);
    for (int guard = 0; guard < 100000; ++guard) {
        const uint32_t pc_before = rb.pc();
        const bool gb = rb.step();
        ++*calls;
        const uint64_t n = rb.pipe().issued - ra.pipe().issued;
        bool ga = true;
        for (uint64_t k = 0; k < n && ga; ++k) {
            if (ra.pipe().halted || ra.pc() >= size) {
                ga = false;
                break;
            }
            if (ra.pc() < g_cover.size()) g_cover[ra.pc()] = 1;
            ga = ra.step();
            if (!ga && k + 1 < n) return "the interpreter ended early";
        }
        if (n == 0) ga = ra.step();
        *steps += n;
        std::string d = diff(a, b, ra, rb);
        if (ga != gb) d += "end ";
        if (!d.empty()) {
            char at[96];
            std::snprintf(at, sizeof at, "after %llu instructions (call from pc %03x to %03x): ", static_cast<unsigned long long>(*steps), pc_before, rb.pc());
            return at + d;
        }
        if (!gb) break;
    }
    ra.finish();
    rb.finish();
    const std::string d = diff(a, b, ra, rb);
    return d.empty() ? std::string() : "at the end: " + d;
}

// a random start state; profile 0 floats (as batch 79), 1 / 2 small integers in the integer registers and memory words, 3 the
// lighting's own ranges (unit-ish normals and colours 0..255, light vectors in -1..1)
VUState random_state(int profile) {
    VUState a{};
    for (int r = 1; r < 32; ++r)
        for (int l = 0; l < 4; ++l)
            a.vf[static_cast<size_t>(r)].f[l] = ri(0, 9) == 0 && profile != 3 ? u2f(R()) : profile == 3 ? (ri(0, 2) == 0 ? rf(0, 255) : rf(-1, 1)) : rf(-50, 50);
    a.vf[0].f[0] = a.vf[0].f[1] = a.vf[0].f[2] = 0.0f;
    a.vf[0].f[3] = 1.0f;
    for (int l = 0; l < 4; ++l) a.acc.f[l] = rf(-5, 5);
    for (int r = 1; r < 16; ++r)
        a.vi[static_cast<size_t>(r)] = static_cast<int16_t>(profile == 0 ? (ri(0, 3) == 0 ? ri(-3, 0x3ff) : ri(0, 8)) : profile == 1 ? ri(0, 2) : (ri(0, 1) == 0 ? 0 : ri(-2, 0xff)));
    for (size_t k = 0; k < 4096; k += 4) {
        float f = ri(0, 9) == 0 && profile != 3 ? u2f(R()) : profile == 3 ? rf(-1, 1) : rf(-20, 20);
        if ((profile == 1 || profile == 2) && ri(0, 2) == 0) {
            const uint32_t w = static_cast<uint32_t>(ri(0, profile == 1 ? 3 : 0xff));
            std::memcpy(&f, &w, 4);
        }
        std::memcpy(&a.mem[k], &f, 4);
    }
    a.q = rf(-2, 2);
    a.i = rf(-2, 2);
    a.macflag = R() & 0xffff;
    a.statusflag = R() & 0xfff;
    a.clipflag = R() & 0xffffff;
    return a;
}

// a captured start state (vu0.cpp's RAC1_VU0_CAPTURE format); false when the file is not one of this set
bool load_capture(const std::string& path, VUState& st, uint32_t& start, const std::vector<uint64_t>& micro) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) return false;
    uint32_t hdr[4];
    float fr[4];
    uint32_t fl[4];
    std::vector<uint64_t> mm(512);
    st = VUState{};
    bool ok = std::fread(hdr, 4, 4, f) == 4 && hdr[0] == 0x01305556u && std::fread(st.vf.data(), sizeof(VUVector), 32, f) == 32 &&
              std::fread(st.vi.data(), 2, 16, f) == 16 && std::fread(&st.acc, sizeof(VUVector), 1, f) == 1 && std::fread(fr, 4, 4, f) == 4 &&
              std::fread(fl, 4, 4, f) == 4 && std::fread(st.mem.data(), 1, 4096, f) == 4096 && std::fread(mm.data(), 8, 512, f) == 512;
    std::fclose(f);
    if (!ok || hdr[1] >= 0xb0 || !std::equal(mm.begin(), mm.begin() + 0xb0, micro.begin())) return false;  // (this set's programs only)
    start = hdr[1];
    st.q = fr[0];
    st.i = fr[1];
    st.r = fr[2];
    st.p = fr[3];
    st.macflag = fl[0];
    st.statusflag = fl[1];
    st.clipflag = fl[2];
    return true;
}

void report_cover(const char* what) {
    uint32_t n = 0, c = 0;
    std::string miss;
    for (uint32_t pc = 0; pc < 0xb0; ++pc) {
        ++n;
        if (g_cover[pc] != 0) {
            ++c;
        } else {
            char b[16];
            std::snprintf(b, sizeof b, " %x", pc);
            miss += b;
        }
    }
    std::printf("  coverage %s: %u / %u instructions run by the interpreter%s%s\n", what, c, n, miss.empty() ? "" : ", not run:", miss.c_str());
}

}  // namespace

void batch_125() {
    if (g_only_batch != 125) return;
    g_batch = 125;
    vu0::vif0_send_chain(0x00100ae0, true);
    const std::vector<uint64_t> micro(vu0::global().micro.begin(), vu0::global().micro.end());
    const uint32_t entries[] = {0x000, 0x02c, 0x058, 0x084};
    const char* only = std::getenv("ORACLE_ONLY");
    const int scale = std::getenv("ORACLE_LIGHT_SCALE") != nullptr ? std::max(1, std::atoi(std::getenv("ORACLE_LIGHT_SCALE"))) : 1;

    // the captured states (as captured, then perturbed)
    std::vector<std::pair<VUState, uint32_t>> captured;
    {
        const char* dir_env = std::getenv("ORACLE_VU0_DIR");
        const std::string dir = dir_env != nullptr ? std::string(dir_env) : dump_path("vu0_light");
        std::vector<std::string> files;
        std::error_code ec;
        if (std::filesystem::is_directory(dir, ec))
            for (const auto& e : std::filesystem::directory_iterator(dir, ec)) files.push_back(e.path().string());
        std::sort(files.begin(), files.end());
        for (const std::string& p : files) {
            VUState st;
            uint32_t start = 0;
            if (load_capture(p, st, start, micro)) captured.emplace_back(st, start);
        }
        std::printf("  %zu captured start states of this set in %s\n", captured.size(), dir.c_str());
    }
    auto perturb = [](VUState s) {
        for (int r = 1; r < 32; ++r)
            for (int l = 0; l < 4; ++l)
                if (ri(0, 3) == 0) s.vf[static_cast<size_t>(r)].f[l] *= rf(0.5f, 1.5f);
        for (size_t k = 0; k < 4096; k += 4)
            if (ri(0, 7) == 0) {
                float f;
                std::memcpy(&f, &s.mem[k], 4);
                f = ri(0, 3) == 0 ? -f : f * rf(0.25f, 2.0f);
                std::memcpy(&s.mem[k], &f, 4);
            }
        return s;
    };

    // ---- the exact port, in lock step
    if (only == nullptr || std::string(only) == "world_light_vu0") {
        g_cover.assign(512, 0);
        const int cases = 800 * scale;
        int ok = 0, shown = 0;
        uint64_t steps = 0;
        for (int c = 0; c < cases; ++c) {
            const uint32_t e = entries[c % 4];
            VUState a = random_state((c / 4) % 4);
            VUState b = a;
            const std::string why = run_lockstep(a, b, micro.data(), micro.size(), e, &steps);
            if (why.empty()) ++ok;
            else if (shown++ < 6) std::printf("  world_light_vu0: case %d entry %03x: %s\n", c, e, why.c_str());
        }
        int cok = 0;
        for (size_t k = 0; k < captured.size(); ++k)
            for (int v = 0; v < 2; ++v) {
                VUState a = v == 0 ? captured[k].first : perturb(captured[k].first);
                VUState b = a;
                const std::string why = run_lockstep(a, b, micro.data(), micro.size(), captured[k].second, &steps);
                if (why.empty()) ++cok;
                else if (shown++ < 6) std::printf("  world_light_vu0: captured %zu (%s) entry %03x: %s\n", k, v == 0 ? "as captured" : "perturbed", captured[k].second, why.c_str());
            }
        report_cover("lighting set 0x000..0x0af (exact)");
        const int total = cases + 2 * static_cast<int>(captured.size());
        std::printf("%-26s %s  %s (%d/%d, %llu instructions in lock step)\n", "world_light_vu0", "0x00100af0", ok + cok == total ? "ok" : "FAIL", ok + cok, total,
                    static_cast<unsigned long long>(steps));
        if (ok + cok != total) ++g_fail;
    }

    // ---- the fast port, compared at its exits and at the end
    if (only == nullptr || std::string(only) == "world_light_vu0_fast") {
        g_cover.assign(512, 0);
        const int cases = 8000 * scale;
        int ok = 0, shown = 0;
        uint64_t steps = 0, calls = 0;
        for (int c = 0; c < cases; ++c) {
            const uint32_t e = entries[c % 4];
            VUState a = random_state((c / 4) % 4);
            VUState b = a;
            const std::string why = run_pair(a, b, micro.data(), micro.size(), e, &steps, &calls);
            if (why.empty()) ++ok;
            else if (shown++ < 6) std::printf("  world_light_vu0_fast: case %d entry %03x: %s\n", c, e, why.c_str());
        }
        int cok = 0;
        const int variants = 8;
        for (size_t k = 0; k < captured.size(); ++k)
            for (int v = 0; v < variants; ++v) {
                VUState a = v == 0 ? captured[k].first : perturb(captured[k].first);
                VUState b = a;
                const std::string why = run_pair(a, b, micro.data(), micro.size(), captured[k].second, &steps, &calls);
                if (why.empty()) ++cok;
                else if (shown++ < 6) std::printf("  world_light_vu0_fast: captured %zu variant %d entry %03x: %s\n", k, v, captured[k].second, why.c_str());
            }
        report_cover("lighting set 0x000..0x0af (fast)");
        const int total = cases + variants * static_cast<int>(captured.size());
        std::printf("%-26s %s  %s (%d/%d, %llu instructions in %llu calls)\n", "world_light_vu0_fast", "0x00100af0", ok + cok == total ? "ok" : "FAIL", ok + cok, total,
                    static_cast<unsigned long long>(steps), static_cast<unsigned long long>(calls));
        if (ok + cok != total) ++g_fail;
    }

    // ---- testing: the cost per instruction, interpreter / exact / fast, on the captured states (or random lighting-profile states)
    if (std::getenv("ORACLE_BENCH") != nullptr) {
        std::vector<std::pair<VUState, uint32_t>> st = captured;
        if (st.empty())
            for (int c = 0; c < 64; ++c) st.emplace_back(random_state(3), entries[c % 4]);
        auto bench = [&](const char* what, VuNativeStep port) {
            double best = 1e30;
            uint64_t insns = 0;
            for (int round = 0; round < 5; ++round) {
                insns = 0;
                const auto t0 = std::chrono::steady_clock::now();
                for (int rep = 0; rep < 50; ++rep)
                    for (const auto& [s0, start] : st) {
                        VUState s = s0;
                        VuMicroRun r;
                        r.start(s, micro.data(), micro.size(), start, port);
                        while (r.step()) {
                        }
                        r.finish();
                        insns += r.pipe().issued;
                    }
                best = std::min(best, std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count());
            }
            std::printf("  bench %-12s %.2f ns per instruction (%llu instructions)\n", what, best / static_cast<double>(insns), static_cast<unsigned long long>(insns));
        };
        bench("interpreter", nullptr);
        bench("exact", &world_light_vu0_step);
        bench("fast", &world_light_vu0_fast);
    }
}

}  // namespace rac1::oracle
