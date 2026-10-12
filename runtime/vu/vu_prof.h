// Testing (RAC1_VU_PROF=1): the time and the instructions the VU0 / VU1 micro programs cost, printed
// by report() (the host calls it before it exits). Off: one predictable branch per program run.
#ifndef RAC1_RUNTIME_VU_VU_PROF_H
#define RAC1_RUNTIME_VU_VU_PROF_H

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>

namespace rac1::vuprof {

struct Counter {
    const char* name = "";
    uint64_t ns = 0, runs = 0, insns = 0;
};

inline bool enabled() {
    static const bool on = std::getenv("RAC1_VU_PROF") != nullptr;
    return on;
}
inline Counter& counter(int i) {
    static Counter c[6] = {{"vu0 skinning / shadows (call + drain)", 0, 0, 0}, {"vu1 moby programs (incl. the GS sink)", 0, 0, 0}, {"  of which the XGKICK sink (GS)", 0, 0, 0}, {"vu0 step() calls of run_until_blocked (ns = TSC ticks)", 0, 0, 0}, {"vu0 call(): port_for (ns = TSC ticks)", 0, 0, 0}, {"vu0 step() calls that ran one instruction", 0, 0, 0}};
    return c[i];
}
inline std::map<uint32_t, uint64_t>& starts() {  // VU0 program starts by micro address
    static std::map<uint32_t, uint64_t> m;
    return m;
}
inline void report() {
    for (const auto& kv : starts()) std::fprintf(stderr, "[vu-prof] vu0 start %03x: %llu times\n", kv.first, static_cast<unsigned long long>(kv.second));
    for (int i = 0; i < 6; ++i) {
        const Counter& c = counter(i);
        if (c.runs == 0) continue;
        std::fprintf(stderr, "[vu-prof] %-40s %9.1f ms  %9llu runs  %12llu instructions  %6.1f ns/insn\n", c.name, static_cast<double>(c.ns) / 1e6,
                     static_cast<unsigned long long>(c.runs), static_cast<unsigned long long>(c.insns),
                     c.insns != 0 ? static_cast<double>(c.ns) / static_cast<double>(c.insns) : 0.0);
    }
}
// RAII: adds the scope's time to counter `i`; the caller adds the instruction count.
class Scope {
public:
    explicit Scope(int i) : on_(enabled()), i_(i) {
        if (on_) {
            t0_ = std::chrono::steady_clock::now();
        }
    }
    ~Scope() {
        if (on_) {
            Counter& c = counter(i_);
            c.ns += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0_).count());
            ++c.runs;
            c.insns += insns;
        }
    }
    uint64_t insns = 0;

private:
    bool on_;
    int i_;
    std::chrono::steady_clock::time_point t0_;
};

}  // namespace rac1::vuprof

#endif
