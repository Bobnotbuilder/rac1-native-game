// Novalis on its own overlay by default: the level-only functions its tables reach that are not moby
// class updates (its camera type, its sound emitters) and the small class 775, ported at Novalis's addresses
// (`// orig: L1:0x00xxxxxx`), plus the dispatch hooks the engine's shared dispatchers ask before their TODO_PORT.

#ifndef RAC1_GAME_OVL_NOVALIS_LEVEL_FNS_H
#define RAC1_GAME_OVL_NOVALIS_LEVEL_FNS_H

#include <cstdint>

namespace rac1 {

// orig: L1:0x00315dd8 -- a camera type function of Novalis's camera table: returns 0
int32_t camera_type_315dd8_l1();
// orig: L1:0x002ff810 -- class 775: grows (+0x2c x 1.025) and fades (+0x23 - 3) each frame, retired when faded
void class775_update_l1(uint32_t m);

// orig: (host) a level's own camera type function (the table's code address of the played level): true when ported,
// its result in `ret`
bool level_camera_type_call(uint32_t fn, uint32_t cam, uint32_t other, int32_t& ret);
// orig: (host) a level's own sound emitter update (+4 of an emitter): true when ported
bool level_sound_emitter_call(uint32_t fn, uint32_t em);

// orig: (none) -- Novalis's registrations of this part (level moby functions)
void novalis_level_fns_register();
// orig: (none) -- the shared effects' registrations (particle types 0x06 / 0x08 / 0x38 / 0x39, the Vendor's draw: Veldin's addresses)
void shared_fx_tr_register();

}  // namespace rac1

#endif
