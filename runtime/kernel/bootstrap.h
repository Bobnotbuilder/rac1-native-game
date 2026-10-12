#ifndef RAC1_RUNTIME_BOOTSTRAP_H
#define RAC1_RUNTIME_BOOTSTRAP_H

#include <runtime/kernel/ee_types.h>

namespace rac1 {

// The EE kernel's cache flush: a no-op on the host (the legacy host's simulated boot loop,
// Engine / poll / vsync / overlay-loader mocks, was removed by).
// orig: 0x00118a80
void FlushCache(s32 operation);

} // namespace rac1

#endif // RAC1_RUNTIME_BOOTSTRAP_H
