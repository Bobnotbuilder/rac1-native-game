// No game bytes: platform and floating-point contracts, NOT an oracle/gameplay substitute.
#ifdef NDEBUG
#error Runtime contracts require assertions enabled
#endif
#include <platform/platform.h>
#include <runtime/ee/ee_thread.h>
#include <runtime/ee/ps2_fpu.h>
#include <cassert>
#include <cfenv>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <stdexcept>

namespace {
uint32_t bits(float v) { uint32_t u; std::memcpy(&u, &v, sizeof u); return u; }
float from_bits(uint32_t u) { float v; std::memcpy(&v, &u, sizeof v); return v; }
void fp_test() {
    rac1::ps2fpu::enter_ee_rounding();
    assert(std::fegetround() == FE_TOWARDZERO);
    volatile float value = 0, step = 0.1f;
    int adds = 0;
    while (value < 1.0f) { value = value + step; assert(++adds < 20); }
    assert(adds == 11);
    assert(bits(value) == 0x3f8ccccb);
    while (value > 0) value = value - step;
    assert(value > -1e-6f && value < 0);
    volatile float tiny = from_bits(1);
    volatile float one = 1;
    assert(bits(one - tiny) == bits(one)); // input denormal must be zero, not a one-ULP subtraction
    volatile float normal = from_bits(0x00800000);
    volatile float half = 0.5f;
    assert(bits(normal * half) == 0); // output subnormal must flush too
    volatile float a = 1, b = 10;
    assert(bits(a / b) == 0x3dcccccc);
    assert(bits(rac1::ps2fpu::div_nearest(a, b)) == 0x3dcccccd);
    assert(bits(rac1::ps2fpu::sqrt_nearest(2)) == 0x3fb504f3);
}
}
int main() {
    fp_test();
    int phase = 0;
    assert(!rac1::ee_thread_run([&] {
        volatile uint64_t live_on_stack = 0x123456789abcdef0ULL;
        assert(rac1::ee_thread_inside());
        for (int i = 0; i < 10000; ++i) {
            assert(std::fegetround() == FE_TOWARDZERO);
            phase = i + 1;
            rac1::ee_thread_vsync(rac1::EeVsyncFrame::Scene);
            assert(live_on_stack == 0x123456789abcdef0ULL);
        }
    }));
    for (int i = 1; i <= 10000; ++i) {
        assert(phase == i);
        assert(rac1::ee_thread_suspended());
        std::fesetround(FE_TONEAREST); // coroutine must retain its own chop mode, host its nearest mode
        const bool done = rac1::ee_thread_run([] { assert(false); }); // ignored while suspended
        assert(std::fegetround() == FE_TONEAREST);
        assert(done == (i == 10000));
    }
    bool caught = false;
    try { rac1::ee_thread_run([] { throw std::runtime_error("expected test exception"); }); }
    catch (const std::runtime_error&) { caught = true; }
    assert(caught);
    assert(rac1::ee_thread_run([] {})); // reusable after completion/exception
    std::printf("PASS: FP contracts, 10000 coroutine yields, per-context FP, stack, exceptions, restart\n");
    assert(!rac1::platform::config_dir().empty());
    assert(!rac1::platform::data_dir().empty());
    assert(rac1::platform::data_dir().string().find("RAC1NativeSaves") != std::string::npos);
    assert(!rac1::platform::executable_path().empty());
    assert(rac1::platform::run_quiet({"python3"}) == -1);
    std::printf("PASS: sandbox paths and forbidden subprocess fail-closed\n");
}
