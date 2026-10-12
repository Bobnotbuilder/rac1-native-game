// A sampling profiler for the host's frame (perf is not available on the development machine,
// perf_event_paranoid): RAC1_SAMPLE_PROF=<file> samples the main thread's call stack every 0.5 ms of wall time (a POSIX
// timer aimed at the thread, SIGPROF, the stack unwound by glibc's backtrace from the .eh_frame tables) and writes the
// samples and /proc/self/maps to <file> at the run's end; a sample report script symbolises and aggregates them.
// RAC1_SAMPLE_FROM=<frame>: only from that host frame on (sample_prof_frame). Linux only; host code.
#include "sample_prof.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#if defined(__linux__) && defined(__GLIBC__)
#include <csignal>
#include <ctime>
#include <execinfo.h>
#include <sys/syscall.h>
#include <unistd.h>
#define RAC1_SAMPLE_PROF_OK 1
#endif

namespace rac1 {

#ifdef RAC1_SAMPLE_PROF_OK
namespace {
constexpr int kDepth = 48;
constexpr size_t kMaxSamples = 400000;
// each sample: a word with the depth, then the frames
std::vector<uintptr_t> g_buf;
std::atomic<size_t> g_used{0};
std::atomic<bool> g_on{false};
std::string g_path;
timer_t g_timer{};
uint64_t g_from = 0;
uint64_t g_samples = 0, g_dropped = 0;

void on_signal(int, siginfo_t*, void*) {
    if (!g_on.load(std::memory_order_relaxed)) return;
    void* frames[kDepth];
    const int n = backtrace(frames, kDepth);
    const size_t at = g_used.load(std::memory_order_relaxed);
    if (at + 1 + static_cast<size_t>(n) > g_buf.size()) {
        ++g_dropped;
        return;
    }
    g_buf[at] = static_cast<uintptr_t>(n);
    for (int i = 0; i < n; ++i) g_buf[at + 1 + static_cast<size_t>(i)] = reinterpret_cast<uintptr_t>(frames[i]);
    g_used.store(at + 1 + static_cast<size_t>(n), std::memory_order_relaxed);
    ++g_samples;
}
}  // namespace

// orig: (host)
void sample_prof_start() {
    const char* p = std::getenv("RAC1_SAMPLE_PROF");
    if (p == nullptr || *p == 0) return;
    g_path = p;
    if (const char* f = std::getenv("RAC1_SAMPLE_FROM")) g_from = std::strtoull(f, nullptr, 10);
    g_buf.assign(kMaxSamples * 8, 0);
    void* warm[4];
    backtrace(warm, 4);  // (the unwinder's library loaded now, not in the handler)
    struct sigaction sa {};
    sa.sa_sigaction = on_signal;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPROF, &sa, nullptr);
    struct sigevent ev {};
    ev.sigev_notify = SIGEV_THREAD_ID;
    ev.sigev_signo = SIGPROF;
    ev._sigev_un._tid = static_cast<pid_t>(syscall(SYS_gettid));
    if (timer_create(CLOCK_MONOTONIC, &ev, &g_timer) != 0) {
        std::fprintf(stderr, "[sample] timer_create failed\n");
        return;
    }
    struct itimerspec its {};
    its.it_interval.tv_nsec = 500000;
    its.it_value.tv_nsec = 500000;
    timer_settime(g_timer, 0, &its, nullptr);
    g_on = g_from == 0;
    std::printf("[sample] sampling the main thread every 0.5 ms into %s\n", g_path.c_str());
}

// orig: (host)
void sample_prof_frame(uint64_t frame) {
    if (!g_path.empty() && !g_on && frame >= g_from) g_on = true;
}

// orig: (host)
void sample_prof_stop() {
    if (g_path.empty()) return;
    g_on = false;
    timer_delete(g_timer);
    std::ofstream out(g_path, std::ios::binary);
    std::ifstream maps("/proc/self/maps");
    std::string line;
    out << "MAPS\n";
    while (std::getline(maps, line)) out << line << "\n";
    out << "SAMPLES " << g_samples << " dropped " << g_dropped << "\n";
    const size_t used = g_used.load();
    for (size_t i = 0; i < used;) {
        const size_t n = g_buf[i];
        for (size_t k = 0; k < n; ++k) {
            char b[24];
            std::snprintf(b, sizeof b, "%lx ", static_cast<unsigned long>(g_buf[i + 1 + k]));
            out << b;
        }
        out << "\n";
        i += 1 + n;
    }
    std::printf("[sample] %llu samples (%llu dropped) written to %s\n", static_cast<unsigned long long>(g_samples), static_cast<unsigned long long>(g_dropped), g_path.c_str());
    g_path.clear();
}
#else
void sample_prof_start() {}
void sample_prof_frame(uint64_t) {}
void sample_prof_stop() {}
#endif

}  // namespace rac1
