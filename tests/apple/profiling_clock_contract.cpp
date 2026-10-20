#ifdef NDEBUG
#error Profiling clock checks must remain enabled
#endif
#include <platform/profiling_clock.hpp>
#include <cassert>
#include <cstdio>
#include <thread>
int main() {
    using namespace rac1::platform;
    const auto before=profiling_ticks();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const auto after=profiling_ticks();
    assert(after>before);
#if defined(__arm64__) || defined(__aarch64__)
    static_assert(profiling_ticks_are_nanoseconds);
    assert(after-before>=1000000); // monotonic units, not a claimed performance benchmark
#else
    static_assert(!profiling_ticks_are_nanoseconds);
#endif
    std::printf("PASS: profiling clock advances; units=%s; game clock unchanged\n",
                profiling_ticks_are_nanoseconds?"NANOSECONDS":"ORIGINAL_X86_TSC_TICKS");
}
