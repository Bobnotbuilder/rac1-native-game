// Veldin's engine functions first reached unported on Rilgar (see veldin_extras.h).
#include <game/ovl/veldin_extras.h>

#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/particle_types_tr.h>
#include <game/ovl/shared_classes_tr.h>

namespace rac1 {

// orig: 0x002d7f38
void class775_update(uint32_t m) {
    const float f0 = fpu_mul(lwc1(m + 0x2c), u2f(0x3f833333u));  // lui at,0x3f83; ori at,at,0x3333; mul.s f0,f0,f1
    const uint32_t v0 = static_cast<uint32_t>(lbu(m + 0x23)) - 3u;  // addiu v0,v0,-3
    sb(m + 0x23, v0);
    swc1(m + 0x2c, f0);  // (the branch's delay slot)
    if ((v0 & 0xffu) < 4u) moby_retire(m);  // sltiu v1,v1,4; beqz; jal 0x24eb68
}

// orig: (none) — registers the functions for the indirect calls (ovl_call_moby_fn)
void veldin_extras_register() {
    ovl_register_moby_fn(0x002d7f38, class775_update);
    ovl_register_moby_fn(0x00274518, particle_43_update);  // (particle type 0x43)
    ovl_register_moby_fn(0x00271c88, particle_30_update);  // (particle type 0x30)
    ovl_register_moby_fn(0x00271df8, particle_32_update);  // (particle type 0x32)
    ovl_register_moby_fn(0x00271ec8, particle_33_update);  // (particle type 0x33)
    ovl_register_moby_fn(0x00273448, particle_3d_update);  // (particle type 0x3d)
    ovl_register_moby_fn(0x00273ee0, particle_41_update);  // (particle type 0x41)
    ovl_register_moby_fn(0x00272bc0, particle_3a_update);  // (particle type 0x3a)
    ovl_register_moby_fn(0x002b4198, class167_update_tr);  // (class 167, the Thruster-Pack's jets)
    ovl_register_moby_fn(0x002b3628, class167_draw_tr);    // (its draw-list callback)
}

}  // namespace rac1
