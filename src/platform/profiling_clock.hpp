// Profiling only, never a game clock. Retain the original x86 TSC measurement;
// ARM64 uses monotonic nanoseconds instead of an unavailable x86 intrinsic.
#pragma once
#include <chrono>
#include <cstdint>
namespace rac1::platform {
inline uint64_t profiling_ticks() noexcept {
#if defined(__i386__) || defined(__x86_64__)
    return __builtin_ia32_rdtsc();
#else
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}
inline constexpr bool profiling_ticks_are_nanoseconds =
#if defined(__i386__) || defined(__x86_64__)
    false;
#else
    true;
#endif
} // namespace rac1::platform
