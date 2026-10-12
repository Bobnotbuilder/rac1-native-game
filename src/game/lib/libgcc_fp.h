#ifndef RAC1_GAME_LIB_LIBGCC_FP_H
#define RAC1_GAME_LIB_LIBGCC_FP_H

// Ports of the boot ELF's soft-float support routines (libgcc's fp-bit.c, single precision to double) that game code
// calls (Eudora's enemy helper L4:0x293128 converts a distance to double for a debug print). The unpacked
// numbers are fp-bit's `fp_number_type`, in EE memory as the original keeps them on its stack:
//   float:  +0 class (0 / 1 NaN, 2 zero, 3 number, 4 infinity), +4 sign, +8 the unbiased exponent, +0xc the fraction (32 bits)
//   double: the same, the fraction a doubleword at +0x10.

#include <cstdint>

namespace rac1 {

// orig: 0x001203b0 — __unpack_f: the float at `src` into the float record at `dst` (an exponent of 0 is zero: no denormals).
void libgcc_unpack_f(uint32_t src, uint32_t dst);

// orig: 0x0011f738 — __pack_d: the double record at `src` packed into an IEEE double (the doubleword returned).
// `a2` is the caller's a2 register, which the original masks into the result (every bit of it cleared on the way).
uint64_t libgcc_pack_d(uint32_t src, uint64_t a2);

// orig: 0x00120370 — __make_dp(class, sign, exp, frac): the double record built on the stack, then __pack_d.
uint64_t libgcc_make_dp(uint32_t cls, uint32_t sign, int32_t exp, uint64_t frac);

// orig: 0x00120478 — __extendsfdf2: the float `f` as an IEEE double (its bits).
uint64_t libgcc_extendsfdf2(float f);

}  // namespace rac1

#endif
