// The checkpoint (the level's state saved when the hero touches a spawn point; its restore is in
// level_setup.cpp): the shared helpers the level's spawn-point mobys call.
#ifndef RAC1_GAME_OVL_CHECKPOINT_H
#define RAC1_GAME_OVL_CHECKPOINT_H

#include <cstdint>

#include <runtime/kernel/vu0.h>  // rac1::Vec4

namespace rac1 {

void checkpoint_mark_collected(uint32_t idx);                        // orig: 0x002502a0
void checkpoint_save_v(const Vec4& pos, uint32_t rot);               // orig: 0x00284e50
void checkpoint_save(uint32_t pos, uint32_t rot);                    // orig: 0x00284e50

}  // namespace rac1

#endif
