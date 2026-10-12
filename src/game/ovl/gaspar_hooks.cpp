// See gaspar_hooks.h.
#include <game/ovl/gaspar_hooks.h>

namespace rac1 {

// orig: (host) see gaspar_hooks.h
bool gaspar_sound_emitter_call(uint32_t fn, uint32_t em) {
    switch (fn) {
    case L9_ADDR(0x003158e8): gas_emitter1_l9(em); return true;
    case L9_ADDR(0x00315c88): gas_emitter3_l9(em); return true;
    default: return false;
    }
}

// orig: (host) see gaspar_hooks.h
bool gaspar_camera_hook_call(uint32_t fn) {
    if (fn != L9_ADDR(0x00314220)) return false;
    gas_cam0_hook_l9(0);
    return true;
}

// orig: (host) see gaspar_hooks.h
bool gaspar_camera_callback_call(uint32_t fn) {
    if (fn != L9_ADDR(0x00314280)) return false;
    gas_cam_callback_l9(0);
    return true;
}

}  // namespace rac1
