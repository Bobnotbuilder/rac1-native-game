// The kick program of a moby packet, traced on the ported VU1 program (see kick_trace.cpp).
#ifndef RAC1_TOOLS_MOBY_MRP_KICK_TRACE_H
#define RAC1_TOOLS_MOBY_MRP_KICK_TRACE_H

#include <formats/moby_mrp.h>

#include <string>

namespace rac1::mrp {

// Fills p.kicks / p.kicks_ok / p.kicks_note; false (with the reason) when a kicked vertex could
// not be attributed to slots.
bool trace_kicks(Packet& p, std::string& note);

}  // namespace rac1::mrp

#endif
