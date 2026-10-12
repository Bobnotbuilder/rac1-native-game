#ifndef RAC1_RUNTIME_VU_XGKICK_H
#define RAC1_RUNTIME_VU_XGKICK_H

#include "vu_interpreter.h"
#include <vector>

namespace rac1 {

/**
 * vu_execute_xgkick
 * Parses the GIF tag at the given VU memory address and extracts GS primitives
 * from the packed data. Appends them to the provided vector.
 */
void vu_execute_xgkick(VUState& state, uint32_t addr, std::vector<GsPrimitive>& primitives);

} // namespace rac1

#endif // RAC1_RUNTIME_VU_XGKICK_H
