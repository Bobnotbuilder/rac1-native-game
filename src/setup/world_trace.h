// The world exporter's decoding and kick traces: see world_trace.cpp.
#ifndef RAC1_TOOLS_WORLD_MRP_EXPORT_WORLD_TRACE_H
#define RAC1_TOOLS_WORLD_MRP_EXPORT_WORLD_TRACE_H

#include <formats/level_core.h>
#include <formats/world_mrp.h>

#include <cstdint>
#include <string>
#include <vector>

namespace rac1::mrpw {

struct LevelSource {
    int32_t level = -1;
    std::vector<uint8_t> core_index, core_data;
    DecodedLevelCoreIndex decoded;
};

bool export_level(const LevelSource& src, World& w, std::string& err, bool dump);
// the boot module's title level: its decompressed file (TOC +0x14e8)
bool export_title(const std::vector<uint8_t>& file, World& w, std::string& err, bool dump);

// The kick trace of one shrub packet (the ported shrub A program run on inputs that name their sources).
bool trace_shrub_packet(ShrubPacket& p, std::string& note);
// The kick trace of one tie packet (the ported tie A program).
bool trace_tie_packet(TiePacket& p, std::string& note);

}  // namespace rac1::mrpw

#endif
