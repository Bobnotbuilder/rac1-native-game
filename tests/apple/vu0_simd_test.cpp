// Differential tests of the ACTUAL upstream VU0 helpers: SIMD vs upstream scalar helpers.
// No game data or generated game code. Same fixture runs with native SSE and native NEON.
#ifdef NDEBUG
#error VU contracts require assertions enabled
#endif
#include <game/ovl/vu0_regs.h>
#include <runtime/ee/ps2_fpu.h>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace rac1::vu0::detail { Vu0 g_instance{}; }
using namespace rac1;
using namespace rac1::vu0m;

static uint32_t rng = 0x12345678;
static uint32_t next() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
static uint64_t digest = 14695981039346656037ULL;
static void record(uint32_t word) {
    for (int i = 0; i < 4; ++i) { digest ^= (word >> (8 * i)) & 255; digest *= 1099511628211ULL; }
}
int main() {
    ps2fpu::enter_ee_rounding();
    size_t cases = 0;
    const uint32_t edge[] = {0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x00800000,
                            0x3f800000, 0xbf800000, 0x7f7fffff, 0x7f800000, 0x7fffffff, 0xff800000};
    for (int iteration = 0; iteration < 1024; ++iteration) {
        Vf raw{}, a{}, b{};
        for (int i = 0; i < 4; ++i) {
            raw.v[i] = u2f(iteration < 12 ? edge[(iteration + i) % 12] : next());
            a.v[i] = u2f(next());
            b.v[i] = u2f(next());
        }
        Vf clamped{};
        _mm_store_si128(reinterpret_cast<__m128i*>(&clamped), cop4(ld4(raw)));
        for (int i = 0; i < 4; ++i) { assert(f2u(clamped.v[i]) == f2u(vuc(raw.v[i]))); record(f2u(clamped.v[i])); }
        for (int mask = 0; mask < 16; ++mask) {
            Regs vector_flags{}, scalar_flags{};
            vector_flags.statusflag = scalar_flags.statusflag = 0xfc0;
            const auto value = fin_m(vector_flags, ld4(raw), mask);
            Vf got{}, want{};
            _mm_store_si128(reinterpret_cast<__m128i*>(&got), value);
            uint32_t flags = 0;
            for (int i = 0; i < 4; ++i) {
                want.v[i] = vuc(raw.v[i]);
                if ((mask >> i) & 1) mac_lane_in(flags, i, raw.v[i]);
                assert(f2u(got.v[i]) == f2u(want.v[i]));
                record(f2u(got.v[i]));
            }
            assert(vector_flags.macflag == flags);
            assert(vector_flags.statusflag == status_of(flags, scalar_flags.statusflag));
            record(vector_flags.macflag);
            record(vector_flags.statusflag);
            Vf stored = a;
            st4_m(stored, ld4(raw), mask);
            for (int i = 0; i < 4; ++i) assert(f2u(stored.v[i]) == f2u((mask >> i & 1) ? raw.v[i] : a.v[i]));
            for (int op = 0; op < 3; ++op) {
                const auto x = cop4(ld4(a)), y = cop4(ld4(b));
                const auto result = op == 0 ? vadd4(x, y) : op == 1 ? vsub4(x, y) : vmul4(x, y);
                _mm_store_si128(reinterpret_cast<__m128i*>(&got), result);
                for (int i = 0; i < 4; ++i) {
                    volatile float av = vuc(a.v[i]), bv = vuc(b.v[i]);
                    const float expected = op == 0 ? av + bv : op == 1 ? av - bv : av * bv;
                    assert(f2u(got.v[i]) == f2u(expected));
                    record(f2u(got.v[i]));
                }
            }
            ++cases;
        }
    }
    Vf original{};
    for (int i = 0; i < 4; ++i) original.v[i] = float(i + 1);
    Vf shuffled{};
    _mm_store_si128(reinterpret_cast<__m128i*>(&shuffled), _mm_shuffle_epi32(ld4(original), _MM_SHUFFLE(3, 0, 2, 1)));
    assert(shuffled.v[0] == 2 && shuffled.v[1] == 3 && shuffled.v[2] == 1 && shuffled.v[3] == 4);
    std::printf("PASS: %zu VU0 SIMD mask/flag/arithmetic cases vs upstream scalar helpers\n", cases);
    std::printf("VU0_SIMD_REFERENCE_DIGEST=%016llx\n", static_cast<unsigned long long>(digest));
}
