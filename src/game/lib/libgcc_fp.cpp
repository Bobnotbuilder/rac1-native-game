#include <game/lib/libgcc_fp.h>

#include <game/ovl/ovl_common.h>
#include <runtime/kernel/ee_memory.h>

#include <cstring>

namespace rac1 {

// orig: 0x001203b0
void libgcc_unpack_f(uint32_t src, uint32_t dst) {
    const uint32_t v = ee_read<uint32_t>(src);
    const uint32_t frac = v & 0x007fffffu;
    const uint32_t exp = (v >> 23) & 0xffu;
    ee_write<uint32_t>(dst + 4, v >> 31);  // (the sign: stored in the branch's delay slot, on every path)
    if (exp == 0) {
        ee_write<uint32_t>(dst, 2);  // zero (a denormal too: its fraction is not looked at)
        return;
    }
    if (exp == 0xff) {
        if (frac == 0) {
            ee_write<uint32_t>(dst, 4);  // infinity
            return;
        }
        ee_write<uint32_t>(dst, (frac & 0x00100000u) != 0 ? 1u : 0u);  // NaN (the class by bit 20 of the fraction)
        ee_write<uint32_t>(dst + 0xc, frac);
        return;
    }
    ee_write<uint32_t>(dst + 0xc, (frac << 7) | 0x40000000u);
    ee_write<uint32_t>(dst + 8, exp - 0x7fu);
    ee_write<uint32_t>(dst, 3);
}

// orig: 0x0011f738
uint64_t libgcc_pack_d(uint32_t src, uint64_t a2) {
    const uint32_t cls = ee_read<uint32_t>(src);
    uint32_t a3 = 0;  // the biased exponent
    const uint32_t sign = ee_read<uint32_t>(src + 4);
    uint64_t a1 = ee_read<uint64_t>(src + 0x10);  // the fraction
    if (cls < 2) {  // NaN: the quiet bit set
        a3 = 0x7ff;
        a1 |= uint64_t{0x8000} << 36;
    } else if (cls == 4) {  // infinity
        a3 = 0x7ff;
        a1 = 0;
    } else if (cls == 2) {  // zero
        a1 = 0;
    } else if (a1 != 0) {
        const int32_t exp = ee_read<int32_t>(src + 8);
        if (exp < -0x3fe) {  // a denormal
            const int32_t sh = -0x3fe - exp;
            a1 >>= static_cast<uint32_t>(sh) & 63u;
            if (!(sh < 0x39)) a1 = 0;
            a1 >>= 8;
        } else {
            a3 = static_cast<uint32_t>(exp + 0x3ff);
            if (!(exp < 0x400)) {  // too large: infinity
                a3 = 0x7ff;
                a1 = 0;
            } else {
                if ((a1 & 0xff) != 0x80) a1 += 0x7f;  // round to nearest, ties to even
                else if ((a1 & 0x100) != 0) a1 += 0x80;
                if (~uint64_t{0} >> 3 < a1) {
                    a1 >>= 1;
                    ++a3;
                }
                a1 >>= 8;
            }
        }
    }
    a2 = (a2 & (uint64_t{0xfff0} << 48)) | (a1 & (~uint64_t{0} >> 12));
    a2 &= 0x800fffffffffffffull;
    a2 |= static_cast<uint64_t>(a3 & 0x7ffu) << 52;
    a2 &= ~uint64_t{0} >> 1;
    return a2 | (static_cast<uint64_t>(sign) << 63);
}

// orig: 0x00120370
uint64_t libgcc_make_dp(uint32_t cls, uint32_t sign, int32_t exp, uint64_t frac) {
    const uint32_t sp = ee_stack_alloc(0x30);
    ee_write<uint32_t>(sp, cls);
    ee_write<uint32_t>(sp + 4, sign);
    ee_write<uint32_t>(sp + 8, static_cast<uint32_t>(exp));
    ee_write<uint64_t>(sp + 0x10, frac);
    const uint64_t r = libgcc_pack_d(sp, static_cast<uint64_t>(static_cast<int64_t>(exp)));  // (a2 still the exponent)
    ee_stack_free(0x30);
    return r;
}

// orig: 0x00120478
uint64_t libgcc_extendsfdf2(float f) {
    const uint32_t sp = ee_stack_alloc(0x30);
    uint32_t bits;
    std::memcpy(&bits, &f, 4);
    ee_write<uint32_t>(sp + 0x10, bits);
    libgcc_unpack_f(sp + 0x10, sp);
    const uint64_t frac = static_cast<uint64_t>(ee_read<uint32_t>(sp + 0xc)) << 30;  // (dsll32 then dsrl 2)
    const uint64_t r = libgcc_make_dp(ee_read<uint32_t>(sp), ee_read<uint32_t>(sp + 4), ee_read<int32_t>(sp + 8), frac);
    ee_stack_free(0x30);
    return r;
}

}  // namespace rac1
