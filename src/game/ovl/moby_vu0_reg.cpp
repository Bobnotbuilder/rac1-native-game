// The registration of the moby pass's VU0 programs (split from the generated moby_vu0.cpp): the code hashes of the
// uploaded sets and the generated ports that run them (the fast ones; the exact step functions with RAC1_VU_EXACT=1). Hand-written.
#include <game/ovl/moby_vu0.h>

#include <runtime/vu/vu0.h>

#include <cstdlib>

namespace rac1 {

// orig: (none) the ports' registration with the runtime VU0 (code hashes of the uploaded sets)
void moby_vu0_register() {
    const bool exact = std::getenv("RAC1_VU_EXACT") != nullptr || std::getenv("RAC1_VU0_EXACT") != nullptr;  // testing: the exact step-by-step ports
    vu0::register_program(0x000, 0x148, 0x007ee420f0ef0448ull, exact ? &moby_vu0_skin_step : &moby_vu0_skin_fast, "moby skinning");
    vu0::register_program(0x190, 0x05e, 0x115121753a51f441ull, exact ? &moby_vu0_shadow_step : &moby_vu0_shadow_fast, "drop shadows");
}

}  // namespace rac1
