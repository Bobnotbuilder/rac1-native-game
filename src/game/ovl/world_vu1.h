// The world's VU1 micro programs, ported: the tfrag, tie and shrub passes' sets (world_vu1_<set>.cpp), registered with the
// VU1 backend by code hash so that the frame chain the
// world passes build (RAC1_WORLD_CHAIN=1 / RAC1_WORLD_FAITHFUL=1) runs on ported code and not the VU interpreter.
#ifndef RAC1_GAME_OVL_WORLD_VU1_H
#define RAC1_GAME_OVL_WORLD_VU1_H

#include <runtime/vu/vu_interpreter.h>

#include <cstdint>

namespace rac1 {

// orig: 0x00103570 (tfrag pass, 0x74c instructions)
bool world_tfrag_a_step(VUState& s, VuPipe& p);
bool world_tfrag_a_fast(VUState& s, VuPipe& p);
// orig: 0x00107020 (tfrag pass, 0x3ba instructions)
bool world_tfrag_b_step(VUState& s, VuPipe& p);
bool world_tfrag_b_fast(VUState& s, VuPipe& p);
// orig: 0x00108e20 (tie pass, 0x3d9 instructions)
bool world_tie_a_step(VUState& s, VuPipe& p);
bool world_tie_a_fast(VUState& s, VuPipe& p);
// orig: 0x0010ad20 (tie pass, 0x6ec instructions)
bool world_tie_b_step(VUState& s, VuPipe& p);
bool world_tie_b_fast(VUState& s, VuPipe& p);
// orig: 0x00101760 (shrub pass, 0x16c instructions)
bool world_shrub_a_step(VUState& s, VuPipe& p);
bool world_shrub_a_fast(VUState& s, VuPipe& p);
// orig: 0x001022e0 (shrub pass, 0x24d instructions)
bool world_shrub_b_step(VUState& s, VuPipe& p);
bool world_shrub_b_fast(VUState& s, VuPipe& p);

// One of the world's programs: its micro address range, the hash the backend matches (fnv1a64 of the instructions),
// the step functions (exact / fast).
struct WorldVu1Program {
    const char* name;
    uint32_t n;
    uint64_t hash;
    VuNativeStep exact;
    VuNativeStep fast;
};
// The six programs (testing: the oracle's proof runs each of them against the interpreter).
const WorldVu1Program* world_vu1_programs(int& count);

// orig: (none) registers them with the VU1 backend
void world_vu1_register();

}  // namespace rac1

#endif
