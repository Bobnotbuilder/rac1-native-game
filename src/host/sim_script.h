// (testing) RAC1_SIM_INPUT=seq:<script>: a timed pad script for headless runs.
#pragma once

#include <cstdint>

namespace rac1 {

// orig: (host) the script's segments `<first>-<last>:<input>[+<input>...]` separated by ';' (frames of the run, last
// exclusive; `<first>+<count>` also works, a lone `<first>` is one frame). Inputs: the buttons x o t s (cross circle
// triangle square), st (Start), se (Select), l1 l2 r1 r2 l3 r3, u d l r (d-pad), and the sticks lx= ly= rx= ry= (a
// value -1..1, ly=-1 forward). Example: "seq:30-400:ly=-1;100+4:x;200-260:r1".
void sim_script(const char* script, uint64_t frame, uint16_t& held, float& lx, float& ly, float& rx, float& ry);

}  // namespace rac1
