// Oracle batch 95 (task 0313): the moby pass's ported VU micro programs in their fast form
// (tools/codegen/vu2cpp.py --fast: basic blocks, the flags kept where they can be read, the float ops
// on four lanes) against the VU interpreter running the uploaded original. Batch 79 proves the
// exact step-by-step ports after EVERY instruction; the fast ports run many instructions per call,
// so here the state is compared where it can be seen: after each call of the fast port (a stop at a
// taken backward jump for VU0, the end of the program, a jump out of the ported blocks), the
// interpreter having run exactly as many instructions -- vf / vi / ACC / Q / I, the MAC / status /
// clip flags, the data memory, the pipeline (pc, issued, Q in flight, the XGKICK / E bit
// countdowns) and the XGKICK packets (address and contents, at the same instruction).

#include "ovl_oracle.h"

#include <game/ovl/moby_vu0.h>
#include <game/ovl/moby_vu1.h>
#include <runtime/vu/vu0.h>
#include <runtime/vu/vu1.h>

#include <algorithm>
#include <chrono>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace rac1 {
// (debugging: the fast VU1 port with an exit at every block boundary, generated into the untracked batch_96_dbg.cpp)
bool moby_vu1_fast_dbg(VUState& s, VuPipe& p) __attribute__((weak));
}

namespace rac1::oracle {

namespace {

uint32_t f2u(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }
float u2f(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
bool eqv(const VUVector& x, const VUVector& y) { return std::memcmp(&x, &y, 16) == 0; }

// What differs between the interpreter's run `ra` / state `a` and the port's `rb` / `b` (empty: nothing).
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
    if (a.mem != b.mem) add("mem");
    const VuPipe &pa = ra.pipe(), &pb = rb.pipe();
    if (pa.pc != pb.pc) {
        std::snprintf(buf, sizeof buf, "pc(%03x/%03x)", pa.pc, pb.pc);
        add(buf);
    }
    if (pa.issued != pb.issued) add("issued");
    if (pa.halted != pb.halted) add("halted");
    if (pa.branch_pending != pb.branch_pending || (pa.branch_pending && pa.branch_target != pb.branch_target)) add("branch");
    if (pa.q_pending != pb.q_pending || (pa.q_pending && (f2u(pa.q_next) != f2u(pb.q_next) || pa.q_ready != pb.q_ready))) add("q-in-flight");
    if (pa.kick_delay != pb.kick_delay || (pa.kick_delay != 0 && pa.kick_addr != pb.kick_addr)) add("kick-pending");
    if (pa.halt_pending != pb.halt_pending || (pa.halt_pending && pa.halt_delay != pb.halt_delay)) add("halt-pending");
    return d;
}

using Packets = std::vector<std::vector<uint8_t>>;
std::function<void(uint32_t)> tap(VUState& st, Packets& out) {
    return [&st, &out](uint32_t addr) {
        const uint32_t qwc = vu1::gif_packet_qwc(st, addr);
        std::vector<uint8_t> pkt(static_cast<size_t>(qwc) * 16 + 4);
        std::memcpy(pkt.data(), &addr, 4);
        for (uint32_t q = 0; q < qwc; ++q) std::memcpy(&pkt[4 + q * 16], &st.mem[((addr + q) & 0x3ff) << 4], 16);
        out.push_back(std::move(pkt));
    };
}

// One program run, interpreter against fast port, compared after each call of the port. `release`: the EE's
// CTC2 that frees a VU0 program waiting on vi1 / vi2 (done at every other stop, on both sides).
struct RunResult {
    bool same = true;
    uint64_t steps = 0;
    int calls = 0;
    int single = 0;  // calls that ran one instruction (the exact step function took over)
    std::string why;
};
std::vector<uint8_t> g_cover;  // the instructions (micro addresses) the interpreter ran in the cases of the current test
RunResult run_pair(VUState& a, VUState& b, const uint64_t* micro, size_t size, uint32_t start, VuNativeStep fast, uint64_t limit, bool release, Packets* ka,
                   Packets* kb) {
    if (g_cover.size() < size) g_cover.assign(size, 0);
    RunResult res;
    VuMicroRun ra, rb;
    ra.start(a, micro, size, start);
    rb.start(b, micro, size, start, fast);
    rb.limit_issue(limit);
    int stops = 0;
    for (;;) {
        if (rb.pipe().issued >= limit) break;  // (the caller's instruction budget: the port stops exactly there)
        const uint32_t pc_before = rb.pc();
        const bool gb = rb.step();
        ++res.calls;
        const uint64_t n = rb.pipe().issued - ra.pipe().issued;
        bool ga = true;
        for (uint64_t k = 0; k < n && ga; ++k) {
            if (ra.pipe().halted || ra.pc() >= size) {
                ga = false;
                break;
            }
            g_cover[ra.pc()] = 1;
            ga = ra.step();
            if (!ga && k + 1 < n) {
                res.same = false;
                res.why = "interpreter ended early";
                return res;
            }
        }
        if (n == 0) ga = ra.step();
        res.steps += n;
        if (n == 1) ++res.single;
        std::string d = diff(a, b, ra, rb);
        if (ka != nullptr && *ka != *kb) d += "kicks ";
        if (ga != gb) d += "end ";
        if (!d.empty()) {
            res.same = false;
            char at[80];
            std::snprintf(at, sizeof at, "after %llu instructions (call from pc %03x to %03x): ", static_cast<unsigned long long>(res.steps), pc_before, rb.pc());
            res.why = at + d;
            return res;
        }
        if (!gb) break;
        if (release && rb.pipe().back_jump && (++stops & 1) == 0) {  // (the EE frees the waits)
            a.vi[1] = b.vi[1] = 0;
            a.vi[2] = b.vi[2] = 0;
        }
    }
    ra.finish();
    rb.finish();
    std::string d = diff(a, b, ra, rb);
    if (ka != nullptr && *ka != *kb) d += "kicks ";
    if (!d.empty()) {
        res.same = false;
        res.why = "at the end: " + d;
    }
    return res;
}

// the micro addresses of [first, last] the cases did not reach (ORACLE_COVER=1 lists them), as `covered / total`
void report_cover(const char* what, uint32_t first, uint32_t last) {
    uint32_t n = 0, c = 0;
    std::string miss;
    for (uint32_t pc = first; pc <= last && pc < g_cover.size(); ++pc) {
        ++n;
        if (g_cover[pc] != 0) {
            ++c;
        } else if (std::getenv("ORACLE_COVER") != nullptr) {
            char b[16];
            std::snprintf(b, sizeof b, " %x", pc);
            miss += b;
        }
    }
    std::printf("  coverage %s: %u / %u instructions run by the interpreter%s%s\n", what, c, n, miss.empty() ? "" : ", not run:", miss.c_str());
}

}  // namespace

void batch_95() {
    if (g_only_batch != 95) return;
    g_batch = 95;
    vu0::vif0_send_chain(0x00100080, true);
    vu0::vif0_send_chain(0x0010e4c0, true);
    if (const char* dump = std::getenv("ORACLE_VU0_DUMP")) {  // (generator input: the uploaded VU0 micro memory)
        FILE* fp = std::fopen(dump, "wb");
        if (fp != nullptr) {
            std::fwrite(vu0::global().micro.data(), 8, vu0::global().micro.size(), fp);
            std::fclose(fp);
        }
    }
    const char* only = std::getenv("ORACLE_ONLY");
    // ---- VU0: random states at every entry (as batch 79), the EE's release of vi1 / vi2 at the stops
    if (only == nullptr || std::string(only) == "moby_vu0_fast") {
        const auto& micro = vu0::global().micro;
        struct Entry { uint32_t pc; VuNativeStep fast; };
        const Entry entries[] = {{0x000, &moby_vu0_skin_fast}, {0x111, &moby_vu0_skin_fast}, {0x121, &moby_vu0_skin_fast},
                                 {0x12d, &moby_vu0_skin_fast}, {0x190, &moby_vu0_shadow_fast}, {0x192, &moby_vu0_shadow_fast}};
        const int cases = std::getenv("ORACLE_VU0_CASES") != nullptr ? std::atoi(std::getenv("ORACLE_VU0_CASES")) : 3600;
        int ok = 0, shown = 0;
        uint64_t steps_total = 0;
        uint64_t calls_total = 0, single_total = 0;
        g_cover.clear();
        for (int c = 0; c < cases; ++c) {
            const Entry& e = entries[c % 6];
            VUState a{};
            // three kinds of start states: as batch 79's (mostly floats, some integers), and two with small integers in the integer
            // registers and in the memory words (ILW reads them as counters / pointers) so that the branches go both ways
            const int profile = (c / 6) % 3;
            for (int r = 1; r < 32; ++r)
                for (int l = 0; l < 4; ++l) a.vf[static_cast<size_t>(r)].f[l] = ri(0, 9) == 0 ? u2f(R()) : rf(-50, 50);
            a.vf[0].f[3] = 1.0f;
            for (int l = 0; l < 4; ++l) a.acc.f[l] = rf(-5, 5);
            for (int r = 1; r < 16; ++r)
                a.vi[static_cast<size_t>(r)] = static_cast<int16_t>(profile == 0 ? (ri(0, 3) == 0 ? ri(-3, 0x3ff) : ri(0, 8)) : profile == 1 ? ri(0, 2) : (ri(0, 1) == 0 ? 0 : ri(-2, 5)));
            for (size_t k = 0; k < a.mem.size(); k += 4) {
                float f = ri(0, 9) == 0 ? u2f(R()) : rf(-20, 20);
                if (profile != 0 && ri(0, 2) == 0) {
                    const uint32_t w = static_cast<uint32_t>(ri(0, profile == 1 ? 3 : 40));
                    std::memcpy(&f, &w, 4);
                }
                std::memcpy(&a.mem[k], &f, 4);
            }
            a.q = rf(-2, 2);
            a.i = rf(-2, 2);
            a.macflag = R() & 0xffff;
            a.statusflag = R() & 0xfff;
            a.clipflag = R() & 0xffffff;
            VUState b = a;
            const RunResult r = run_pair(a, b, micro.data(), micro.size(), e.pc, e.fast, 4000, true, nullptr, nullptr);
            steps_total += r.steps;
            calls_total += static_cast<uint64_t>(r.calls);
            single_total += static_cast<uint64_t>(r.single);
            if (r.same) ++ok;
            else if (shown++ < 6) std::printf("  moby_vu0_fast: case %d entry %03x: %s\n", c, e.pc, r.why.c_str());
        }
        report_cover("VU0 skinning set 0x000..0x147", 0x000, 0x147);
        report_cover("VU0 shadow set 0x190..0x1ed", 0x190, 0x1ed);
        std::printf("%-26s %s  %s (%d/%d, %llu instructions in %llu calls, %llu of them single steps)\n", "moby_vu0_fast", "0x00100090", ok == cases ? "ok" : "FAIL", ok, cases,
                    static_cast<unsigned long long>(steps_total), static_cast<unsigned long long>(calls_total), static_cast<unsigned long long>(single_total));
        if (ok != cases) ++g_fail;
    }
    // ---- VU1: the states the moby programs start on in the running game (assets/dumps/oracle/vu1_moby), as
    // captured and perturbed, the XGKICK packets compared
    if (only == nullptr || std::string(only) == "moby_vu1_fast") {
        const char* dir_env = std::getenv("ORACLE_VU1_DIR");  // (one or more directories, colon separated)
        const std::string dirs = dir_env != nullptr ? std::string(dir_env) : dump_path("vu1_moby");
        const std::string dir = dirs;
        std::vector<std::string> files;
        for (size_t at = 0; at <= dirs.size();) {
            size_t end = dirs.find(':', at);
            if (end == std::string::npos) end = dirs.size();
            const std::string d = dirs.substr(at, end - at);
            at = end + 1;
            if (std::filesystem::is_directory(d))
                for (const auto& de : std::filesystem::recursive_directory_iterator(d))
                    if (de.path().extension() == ".bin") files.push_back(de.path().string());
        }
        std::sort(files.begin(), files.end());
        g_cover.clear();
        int cases = 0, ok = 0, shown = 0, kicks_total = 0;
        uint64_t steps_total = 0;
        uint64_t calls_total = 0, single_total = 0;
        auto micro = std::make_unique<std::array<uint64_t, 2048>>();
        const int variants = std::getenv("ORACLE_VU1_VARIANTS") != nullptr ? std::atoi(std::getenv("ORACLE_VU1_VARIANTS")) : 7;
        for (const std::string& path : files) {
            FILE* fp = std::fopen(path.c_str(), "rb");
            if (fp == nullptr) continue;
            uint32_t hdr[4];
            VUState base{};
            float fr[4];
            uint32_t fl[4];
            bool rd = std::fread(hdr, 4, 4, fp) == 4 && std::fread(base.vf.data(), sizeof(VUVector), 32, fp) == 32 &&
                      std::fread(base.vi.data(), 2, 16, fp) == 16 && std::fread(&base.acc, sizeof(VUVector), 1, fp) == 1 &&
                      std::fread(fr, 4, 4, fp) == 4 && std::fread(fl, 4, 4, fp) == 4 &&
                      std::fread(base.mem.data(), 1, base.mem.size(), fp) == base.mem.size() && std::fread(micro->data(), 8, 2048, fp) == 2048;
            std::fclose(fp);
            if (!rd || hdr[0] != 0x31555676u) continue;
            if (vu1::code_hash(micro->data(), 0x508) != 0x1585563a86921802ull) continue;  // (another pass's programs)
            base.q = fr[0], base.i = fr[1], base.r = fr[2], base.p = fr[3];
            base.macflag = fl[0], base.statusflag = fl[1], base.clipflag = fl[2];
            base.xtop = hdr[2];
            if (std::getenv("ORACLE_BENCH") != nullptr) {  // timing: the captured state run by the exact port and by the fast one, no checks
                static double t_exact = 0, t_fast = 0;
                static uint64_t n_exact = 0, n_fast = 0, files_done = 0;
                double best[2] = {1e9, 1e9};
                uint64_t cnt[2] = {0, 0};
                for (int rep = 0; rep < 5; ++rep)
                    for (int which = 0; which < 2; ++which) {
                        VUState a = base;
                        a.xgkick_hook = [&](uint32_t) {};
                        VuMicroRun run;
                        run.start(a, micro->data(), micro->size(), hdr[1], which == 0 ? &moby_vu1_step : &moby_vu1_fast);
                        run.limit_issue(2000000);
                        const auto t0 = std::chrono::steady_clock::now();
                        while (run.pipe().issued < 2000000 && run.step()) {
                        }
                        run.finish();
                        const double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                        best[which] = std::min(best[which], dt);  // (the quietest of the repetitions: the machine is shared)
                        cnt[which] = run.pipe().issued;
                    }
                t_exact += best[0], t_fast += best[1];
                n_exact += cnt[0], n_fast += cnt[1];
                if (++files_done == files.size() || files_done % 200 == 0)
                    std::printf("  bench moby_vu1: exact %.2f ns/insn (%llu insns), fast %.2f ns/insn (%llu insns)\n", t_exact * 1e9 / static_cast<double>(n_exact),
                                static_cast<unsigned long long>(n_exact), t_fast * 1e9 / static_cast<double>(n_fast), static_cast<unsigned long long>(n_fast));
                continue;
            }
            for (int variant = 0; variant < variants; ++variant) {
                VUState a = base;
                if (variant >= 4) {  // a wild start: the integer registers and some memory words small integers, the control flow explored (the budget cuts the run)
                    for (int r = 1; r < 16; ++r)
                        if (ri(0, 1) == 0) a.vi[static_cast<size_t>(r)] = static_cast<int16_t>(ri(0, 2) == 0 ? ri(-2, 0x3ff) : ri(0, 6));
                    for (size_t k = 0; k < a.mem.size(); k += 4)
                        if (ri(0, 5) == 0) {
                            const uint32_t w = static_cast<uint32_t>(ri(0, 24));
                            std::memcpy(&a.mem[k], &w, 4);
                        }
                    for (int r = 1; r < 32; ++r)
                        for (int l = 0; l < 4; ++l)
                            if (ri(0, 3) == 0) a.vf[static_cast<size_t>(r)].f[l] = ri(0, 4) == 0 ? u2f(R()) : rf(-50, 50);
                } else if (variant != 0) {
                    for (int r = 1; r < 32; ++r)
                        for (int l = 0; l < 4; ++l)
                            if (ri(0, 3) == 0) a.vf[static_cast<size_t>(r)].f[l] = rf(-50, 50);
                    for (size_t k = 0; k < a.mem.size(); k += 4) {
                        uint32_t w;
                        std::memcpy(&w, &a.mem[k], 4);
                        const uint32_t e = (w >> 23) & 0xff;
                        if (e > 0x60 && e < 0x9f && ri(0, 2) == 0) {  // a float: moved by up to +-25% (x2 for the clip tests)
                            float f = u2f(w) * rf(variant % 4 == 3 ? -1.0f : 0.75f, variant % 4 == 3 ? 2.0f : 1.25f);
                            std::memcpy(&a.mem[k], &f, 4);
                        }
                    }
                }
                VUState b = a;
                Packets ka, kb;
                a.xgkick_hook = tap(a, ka);
                b.xgkick_hook = tap(b, kb);
                const uint64_t budget = variant >= 4 ? static_cast<uint64_t>(ri(200, 30000)) : (variant % 2 == 1 && ri(0, 2) == 0) ? static_cast<uint64_t>(ri(1, 6000)) : 2000000;  // (some with a tight budget: a runaway program)
                const RunResult r = run_pair(a, b, micro->data(), micro->size(), hdr[1], (std::getenv("ORACLE_VU1_DBG") != nullptr && moby_vu1_fast_dbg != nullptr) ? &moby_vu1_fast_dbg : &moby_vu1_fast, budget, false, &ka, &kb);
                steps_total += r.steps;
                calls_total += static_cast<uint64_t>(r.calls);
                single_total += static_cast<uint64_t>(r.single);
                kicks_total += static_cast<int>(ka.size());
                ++cases;
                if (r.same) ++ok;
                else if (shown++ < 6) std::printf("  moby_vu1_fast: %s variant %d (start %03x): %s\n", path.c_str(), variant, hdr[1], r.why.c_str());
            }
        }
        report_cover("VU1 moby programs 0x000..0x507", 0x000, 0x507);
        if (cases == 0 && files.empty()) {  // no captures recorded on this machine: skipped, not failed
            std::printf("%-26s %s  skipped (no captured states in %s: run rac1-native with RAC1_VU1_CAPTURE=%s)\n", "moby_vu1_fast", "0x0010faa8", dir.c_str(), dir.c_str());
            ++g_skipped;
        } else if (cases == 0) {
            std::printf("%-26s %s  FAIL (no captured states in %s: run rac1-native with RAC1_VU1_CAPTURE=%s)\n", "moby_vu1_fast", "0x0010faa8", dir.c_str(), dir.c_str());
            ++g_fail;
        } else {
            std::printf("%-26s %s  %s (%d/%d, %llu instructions in %llu calls, %llu single steps, %d kicks)\n", "moby_vu1_fast", "0x0010faa8", ok == cases ? "ok" : "FAIL", ok, cases,
                        static_cast<unsigned long long>(steps_total), static_cast<unsigned long long>(calls_total), static_cast<unsigned long long>(single_total), kicks_total);
            if (ok != cases) ++g_fail;
        }
    }
}

}  // namespace rac1::oracle
