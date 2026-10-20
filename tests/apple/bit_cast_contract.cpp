#ifdef NDEBUG
#error Representation checks must remain enabled
#endif
#include <platform/bit_cast.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
static_assert(rac1::portable_bit_cast<uint32_t>(1.f)==0x3f800000);
static_assert(rac1::portable_bit_cast<float>(uint32_t{0xbf800000})==-1.f);
int main() {
    uint32_t x=0x97199;
    for(unsigned i=0;i<1000000;++i) {
        x^=x<<13; x^=x>>17; x^=x<<5;
        const float f=rac1::portable_bit_cast<float>(x);
        assert(rac1::portable_bit_cast<uint32_t>(f)==x);
    }
    for(uint32_t edge:{0u,0x80000000u,1u,0x007fffffu,0x7f800000u,0xff800000u,0x7fc00001u,0x7f800001u})
        assert(rac1::portable_bit_cast<uint32_t>(rac1::portable_bit_cast<float>(edge))==edge);
    std::puts("PASS: 1000000 representation round trips and signed-zero/subnormal/infinity/NaN edges; constexpr checks");
}
