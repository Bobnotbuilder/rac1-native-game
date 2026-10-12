#ifndef RAC1_RUNTIME_VU_VIF_UNPACK_H
#define RAC1_RUNTIME_VU_VIF_UNPACK_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include "vu_interpreter.h"

namespace rac1 {

struct VifState {
    uint32_t mask;
    uint32_t r0, r1, r2, r3;
    uint32_t c0, c1, c2, c3;
    uint32_t mode;

    VifState() : mask(0), r0(0), r1(0), r2(0), r3(0), c0(0), c1(0), c2(0), c3(0), mode(0) {}
};

/**
 * unpack_vifcode
 * Parses a VIFcode stream containing UNPACK, STMASK, STROW, STCOL commands
 * and writes the unpacked data directly into the provided VUState memory.
 */
void unpack_vifcode(VUState& vu_state, VifState& vif_state, const uint8_t* stream, size_t size);

} // namespace rac1

#endif // RAC1_RUNTIME_VU_VIF_UNPACK_H
