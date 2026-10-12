#include <runtime/kernel/bootstrap.h>

namespace rac1 {

void FlushCache(s32 operation) {
    (void)operation;
    // In our native port, cache management is handled by the host hardware/OS.
    // We can safely treat FlushCache as a no-op.
}

} // namespace rac1
