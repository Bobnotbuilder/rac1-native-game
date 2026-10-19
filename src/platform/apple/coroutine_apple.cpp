// MIT: the existing platform coroutine interface on Apple ARM64. No ucontext, JIT, or extra thread.
#include <platform/platform.h>
#include "arm64_context.h"
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <cfenv>
#include <cstdlib>
#include <limits>

namespace rac1::platform {
namespace {
using apple::Context;
struct Coroutine {
    Context host{}, game{};
    void* mapping = nullptr;
    size_t bytes = 0, guard = 0;
    pthread_t owner{};
    void (*entry)() = nullptr;
    bool active = false;
} g_co;
void require(bool condition) {
    if (!condition) std::abort(); // programmer error: never silently run a blocking game loop on the host
}
void owner_check() { require(g_co.mapping && pthread_equal(g_co.owner, pthread_self())); }
[[noreturn]] void trampoline() {
    g_co.entry(); // ee_thread.cpp catches exceptions before they reach the stack switch
    g_co.active = false;
    apple::rac1_arm64_context_swap(&g_co.game, &g_co.host);
    std::abort(); // a completed coroutine may only be restarted, never resumed
}
}
bool coroutine_create(size_t bytes) {
    if (g_co.mapping) {
        owner_check();
        return true;
    }
    const long page = sysconf(_SC_PAGESIZE);
    if (page <= 0 || bytes == 0) return false;
    const size_t p = static_cast<size_t>(page);
    if (bytes > std::numeric_limits<size_t>::max() - 3 * p) return false;
    bytes = ((bytes + p - 1) / p) * p;
    // Reserve a private, non-executable stack with guards at BOTH ends. Physical pages are
    // demand-paged; unlike Linux, Darwin needs neither MAP_STACK nor MAP_NORESERVE.
    void* memory = mmap(nullptr, bytes + 2 * p, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (memory == MAP_FAILED) return false;
    if (mprotect(static_cast<char*>(memory) + p, bytes, PROT_READ | PROT_WRITE)) {
        munmap(memory, bytes + 2 * p);
        return false;
    }
    g_co.mapping = memory;
    g_co.bytes = bytes;
    g_co.guard = p;
    g_co.owner = pthread_self();
    return true;
}
void coroutine_start(void (*entry)()) {
    owner_check();
    require(!g_co.active && entry);
    g_co.entry = entry;
    g_co.game = {};
    g_co.game.sp = reinterpret_cast<uintptr_t>(g_co.mapping) + g_co.guard + g_co.bytes;
    g_co.game.x19_x30[11] = reinterpret_cast<uintptr_t>(&trampoline);
    __asm__ volatile("mrs %0, fpcr" : "=r"(g_co.game.fpcr));
    __asm__ volatile("mrs %0, fpsr" : "=r"(g_co.game.fpsr));
    g_co.active = true;
}
void coroutine_resume() {
    owner_check();
    require(g_co.active);
    apple::rac1_arm64_context_swap(&g_co.host, &g_co.game);
}
void coroutine_yield() {
    owner_check();
    require(g_co.active);
    apple::rac1_arm64_context_swap(&g_co.game, &g_co.host);
}
} // namespace rac1::platform
