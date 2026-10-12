// The journal of EE memory (ee_memory.h ee_journal_begin) ended keeping its writes.
#ifndef RAC1_RUNTIME_KERNEL_EE_JOURNAL_H
#define RAC1_RUNTIME_KERNEL_EE_JOURNAL_H

#include <runtime/kernel/ee_memory.h>

#include <vector>

namespace rac1 {

// A run of EE bytes a journaled stretch of code changed: their address, the bytes before and after.
struct EeByteRun {
    u32 addr = 0;
    std::vector<u8> before, after;
};
// Ends the journal begun by ee_journal_begin keeping what was written: the runs of RAM bytes that changed since begin (and the
// scratchpad's, at 0x70000000 + offset) appended to `out`; returns the number of runs.
size_t ee_journal_commit(std::vector<EeByteRun>& out);

}  // namespace rac1

#endif
