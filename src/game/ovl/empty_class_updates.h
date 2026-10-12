// Class updates that are only `jr ra`: the gadgets' classes 433 Sonic Summoner, 618 Armor Magnetizer and 1289 O2 Mask
// (class 1289 is spawned by the ship's landing sequence 0x290590).
#ifndef RAC1_GAME_OVL_EMPTY_CLASS_UPDATES_H
#define RAC1_GAME_OVL_EMPTY_CLASS_UPDATES_H

#include <cstdint>

namespace rac1 {

void sonic_summoner_update(uint32_t m);  // orig: 0x002cc358  class 433
void armor_magnetizer_update(uint32_t m);  // orig: 0x002d3830  class 618
void o2_mask_update(uint32_t m);  // orig: 0x002e0398  class 1289
// orig: (none) — registers them for the indirect calls (ovl_call_moby_fn)
void empty_class_updates_register();

}  // namespace rac1
#endif
