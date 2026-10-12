// The EE's main thread as a coroutine of the host (ADR 0012): see ee_thread.h.
#include "ee_thread.h"

#include <sys/mman.h>
#include <ucontext.h>

#include <cstdio>
#include <cstdlib>
#include <exception>

namespace rac1 {

namespace {
// 64 MB of address space (committed as it is touched), a guard page below it: the game's frames
// (the transliterations' register files, their callees) as deep as on the host's main stack.
constexpr size_t kStack = 64u << 20;
constexpr size_t kGuard = 1u << 16;

struct Coroutine {
    ucontext_t host{};
    ucontext_t game{};
    void* stack = nullptr;
    bool running = false;    // fn started, not returned
    bool inside = false;     // the coroutine is the current context
    bool suspended = false;  // waiting in ee_thread_vsync
    EeVsyncFrame frame = EeVsyncFrame::Kicked;
    std::function<void()> fn;
    std::exception_ptr error;
};
Coroutine g_co;
void (*g_vsync_hook)(EeVsyncFrame) = nullptr;  // the host's per-vsync work (ee_thread_set_vsync_hook)

void entry() {
    try {
        g_co.fn();
    } catch (...) {
        g_co.error = std::current_exception();
    }
    g_co.running = false;
    g_co.inside = false;
    // (uc_link is the host's context: returning from here resumes it)
}
}  // namespace

// orig: (host) the EE thread's coroutine
bool ee_thread_run(const std::function<void()>& fn) {
    if (g_co.inside) {  // (already on the game's thread: a nested run is a plain call)
        fn();
        return true;
    }
    if (!g_co.suspended) {
        if (g_co.stack == nullptr) {
            void* p = mmap(nullptr, kStack + kGuard, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_STACK, -1, 0);
            if (p == MAP_FAILED) {
                std::fprintf(stderr, "[ee-thread] no stack: the game runs on the host's thread\n");
                fn();
                return true;
            }
            mprotect(p, kGuard, PROT_NONE);
            g_co.stack = p;
        }
        g_co.fn = fn;
        g_co.running = true;
        getcontext(&g_co.game);  // (the host thread's FPU modes, ADR 0011's rounding, carried into the coroutine)
        g_co.game.uc_stack.ss_sp = static_cast<char*>(g_co.stack) + kGuard;
        g_co.game.uc_stack.ss_size = kStack;
        g_co.game.uc_link = &g_co.host;
        makecontext(&g_co.game, entry, 0);
    }
    g_co.suspended = false;
    g_co.inside = true;
    swapcontext(&g_co.host, &g_co.game);
    g_co.inside = false;
    if (g_co.error) {
        std::exception_ptr e = g_co.error;
        g_co.error = nullptr;
        g_co.suspended = false;
        g_co.running = false;
        std::rethrow_exception(e);
    }
    if (!g_co.running) g_co.fn = nullptr;
    return !g_co.running;
}

// orig: (host) the EE thread's coroutine
bool ee_thread_suspended() { return g_co.suspended; }
// orig: (host) the EE thread's coroutine
EeVsyncFrame ee_thread_frame() { return g_co.frame; }
// orig: (host) the EE thread's coroutine
bool ee_thread_inside() { return g_co.inside; }

// orig: (host) the EE thread's coroutine
void ee_thread_set_vsync_hook(void (*hook)(EeVsyncFrame)) { g_vsync_hook = hook; }

// orig: (host) the vsync wait (the kernel's semaphore, 0x122298) inside a blocking loop
void ee_thread_vsync(EeVsyncFrame kind, bool shown) {
    if (g_vsync_hook != nullptr) g_vsync_hook(kind);
    if (!g_co.inside || !shown) return;
    g_co.frame = kind;
    g_co.suspended = true;
    g_co.inside = false;
    swapcontext(&g_co.game, &g_co.host);
    g_co.inside = true;
}

}  // namespace rac1
