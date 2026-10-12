// See pokitaru_hooks.h.
#include <game/ovl/pokitaru_hooks.h>

namespace rac1 {

// orig: (host) see pokitaru_hooks.h
bool pokitaru_sound_emitter_call(uint32_t fn, uint32_t em) {
    switch (fn) {
    case L11_ADDR(0x00328b50): pok_emitter1_l11(em); return true;
    case L11_ADDR(0x00328ef0): pok_emitter5_l11(em); return true;
    default: return false;
    }
}

// orig: (host) see pokitaru_hooks.h
bool pokitaru_camera_hook_call(uint32_t fn) {
    if (fn != L11_ADDR(0x00327a90)) return false;
    pok_cam0_hook_l11(0);
    return true;
}

// orig: (host) see pokitaru_hooks.h
bool pokitaru_camera_callback_call(uint32_t fn) {
    if (fn != L11_ADDR(0x00327af0)) return false;
    pok_cam_callback_l11(0);
    return true;
}

}  // namespace rac1
