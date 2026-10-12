// The memory card's machine's operations and the save's helpers: see
// memory_card_ops.cpp. (card_machine 0x24b690 itself is
// declared in memory_card.h.)
#ifndef RAC1_GAME_OVL_MEMORY_CARD_OPS_H
#define RAC1_GAME_OVL_MEMORY_CARD_OPS_H

#include <cstdint>

namespace rac1 {

void save_slot_summary(uint32_t p, int32_t port, int32_t n);  // orig: 0x0024d118
void save_blocks_write(uint32_t p);                           // orig: 0x0024ce68
void save_names_init(uint32_t cnf);                           // orig: 0x0024b410
void card_lib_init();                                         // orig: 0x0024cf10

}  // namespace rac1

#endif
