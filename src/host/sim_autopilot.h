// (testing) RAC1_AUTOPILOT: the left stick steered toward a target for headless runs (the hoverboard race).
#pragma once

#include <cstdint>

namespace rac1 {

// orig: (host) RAC1_AUTOPILOT=<first>-<last>:<mode>[|<first>-<last>:<mode>...], frames of the run (last exclusive). Modes:
//   class:<c>[:<n>]   steer toward the n-th live moby of class c (0 = the first), aiming ahead of it by its motion;
//   wp:x,y[,j]/...    follow the closed line of the waypoints (pure pursuit: the point 6 units on from the line's nearest point to
//                     the hero; j = 1: cross pressed once the hero passes that waypoint, a jump); after a respawn (the hero moved 10
//                     units in a frame) the stick is let go (ahead only) until the hero is within 3 units of the line (another route
//                     after a checkpoint);
// go:x,y/... walk to the waypoints in order and let the stick go at the last (within 0.6 units of each): the
//                     stick camera-relative (the view's heading from its position 0x166dc0 to the hero: a walking hero, Clank);
// Otherwise ly is held at -1 (full ahead) and lx turns the hero's heading (taken from its motion) toward the aim. RAC1_AUTOPILOT_TRACE=<n>
// prints the hero, the aim and the stick every n frames; RAC1_AUTOPILOT_LOG=<c> prints the live mobys of class c every 10 frames
// (to record a racer's line as waypoints).
void sim_autopilot(uint64_t frame, uint16_t& held, float& lx, float& ly);

}  // namespace rac1
