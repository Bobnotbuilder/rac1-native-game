// A sampling profiler of the host's main thread (see sample_prof.cpp). Host code.
#ifndef RAC1_TOOLS_RUNTIME_HOST_SAMPLE_PROF_H
#define RAC1_TOOLS_RUNTIME_HOST_SAMPLE_PROF_H

#include <cstdint>

namespace rac1 {
// orig: (host) RAC1_SAMPLE_PROF=<file>: sampling starts (RAC1_SAMPLE_FROM=<frame>: from that frame)
void sample_prof_start();
// orig: (host) the host's frame counter (for RAC1_SAMPLE_FROM)
void sample_prof_frame(uint64_t frame);
// orig: (host) the samples written
void sample_prof_stop();
}  // namespace rac1

#endif
