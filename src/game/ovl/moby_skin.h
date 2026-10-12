// The moby software skinning (ADR 0007): per skinning record, after its joints' pose (0x250fe0),
// the moby's vertices skinned, lit and packed by the EE with a VU0 micro program into the moby's
// VIF1 chain.
#ifndef RAC1_GAME_OVL_MOBY_SKIN_H
#define RAC1_GAME_OVL_MOBY_SKIN_H

#include <cstdint>

namespace rac1 {

// The record at `rec` (the ring the draw loop fills), `work` the joints' RAM buffer; returns
// the next record.
// orig: 0x001ee858
uint32_t moby_skin(uint32_t rec, uint32_t work);

// orig: (host) an observer of the poses (no effect): called with the record `rec` just after its
// joints' pose is on the scratchpad (0x40 bytes per joint from 0x70000000), before the skinning
// reads it. Used by the MRP scene state (render/mrp_scene.h).
using MobyPoseObserver = void (*)(uint32_t rec, uint32_t work);
extern MobyPoseObserver g_moby_pose_observer;

// orig: (host) the MRP skip: the record after `rec` when the skinning is skipped, 0 when unknown
// (the skinning then runs). Set by the MRP scene state.
using MobySkipNext = uint32_t (*)(uint32_t rec);
extern MobySkipNext g_moby_skip_next;

// The ring's records from `rec` (to the one whose first word is 0): each one's joints posed
// (0x250fe0), then skinned (0x1ee858).
// orig: 0x002558e0
void moby_skin_all(uint32_t rec, uint32_t work);
// The pass's joints: the scratchpad's 0x3800 from 0x166080, the ring from its start 0x15f5f8.
// orig: 0x0024fd80
void moby_joints();

}  // namespace rac1

#endif
