// Minimal compiler/runtime environment regression, not a game oracle or physical iPhone test.
#ifdef NDEBUG
#error FP environment assertions must remain enabled
#endif
#pragma STDC FENV_ACCESS ON
#include <runtime/ee/ps2_fpu.h>
#include <cstring>
#include <cassert>
#include <cfenv>
#include <cstdint>
#include <cstdio>
template<class To,class From> To representation(From value) {
    static_assert(sizeof(To)==sizeof(From));
    To result;
    std::memcpy(&result,&value,sizeof(result));
    return result;
}

__attribute__((noinline)) float fp_constant_positive() { return 1.0f + 0x1.8p-24f; }
__attribute__((noinline)) float fp_constant_negative() { return -1.0f - 0x1.8p-24f; }
__attribute__((noinline)) float fp_variable_add(float a,float b) { return a+b; }
__attribute__((noinline)) float fp_no_contract(float a,float b,float c) { return a*b+c; }
__attribute__((noinline)) float fp_narrow(double a) { return static_cast<float>(a); }

int main() {
    fenv_t original;
    assert(std::fegetenv(&original)==0);
    struct Case { int mode; uint32_t positive,negative; };
    const Case cases[]={{FE_TONEAREST,0x3f800001,0xbf800001}, {FE_TOWARDZERO,0x3f800000,0xbf800000},
                        {FE_UPWARD,0x3f800001,0xbf800000}, {FE_DOWNWARD,0x3f800000,0xbf800001}};
    unsigned checked=0;
    for(const auto& c:cases) {
        assert(std::fesetround(c.mode)==0);
        assert(representation<uint32_t>(fp_constant_positive())==c.positive);
        assert(representation<uint32_t>(fp_constant_negative())==c.negative);
        assert(representation<uint32_t>(fp_variable_add(1.f,0x1.8p-24f))==c.positive);
        assert(representation<uint32_t>(fp_narrow(1.0+0x1.8p-24))==c.positive);
        checked+=4;
    }
    rac1::ps2fpu::enter_ee_rounding();
    assert(std::fegetround()==FE_TOWARDZERO);
    const float next=representation<float>(uint32_t{0x3f800001});
    assert(fp_no_contract(next,next,representation<float>(uint32_t{0xbf800002}))==0);
    assert(representation<uint32_t>(fp_variable_add(1.f,-representation<float>(uint32_t{1})))==0x3f800000);
    assert(representation<uint32_t>(rac1::ps2fpu::div_nearest(1.f,3.f))==0x3eaaaaab);
    assert(representation<uint32_t>(rac1::ps2fpu::sqrt_nearest(2.f))==0x3fb504f3);
    assert(std::fesetenv(&original)==0);
    std::printf("PASS: %u dynamic rounding/constant/narrowing cases; FTZ, non-contraction and upstream nearest helpers\n",checked);
    std::puts("GAME_ORACLE=NOT_RUN; PHYSICAL_IPHONE=NOT_TESTED");
}
