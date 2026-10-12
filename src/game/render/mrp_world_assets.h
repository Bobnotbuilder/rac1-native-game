// The MRP world assets (src/formats/world_mrp.h) as the renderer and the proof use them: read from
// assets/generated/mrp/world/level<N>/, the VU1 images the packets leave in VU1 memory derived from their data with the VIF
// (runtime/vu: the same UNPACK the frame chain does). Host code.
#ifndef RAC1_GAME_RENDER_MRP_WORLD_ASSETS_H
#define RAC1_GAME_RENDER_MRP_WORLD_ASSETS_H

#include <formats/world_mrp.h>

#include <string>

namespace rac1::mrpw {

// A shrub packet's image: its VIF list run from buffer base 0 (to the last qword written).
std::vector<uint8_t> shrub_image(const ShrubPacket& p);
// A tie packet's image (VU1 addresses 0..0x123): the chain's UNPACKs of the packet's header, its texture slots (the class's
// prims as the file stores them), its control qwords, vertices and colour index lists (0x295fa0's packet part).
std::vector<uint8_t> tie_image(const TieClass& c, const TiePacket& p);
// A tfrag entry's image for `lod` (0 far, 1 mid, 2 near): its lists (the chain's REFs, each after STCYCL 4 / 4), which qwords
// they wrote.
void tfrag_image(const TfragEntry& t, int lod, TfragImage& out);
// All of a world's images (after read_world: the file does not keep them).
void derive_images(World& w);

// the boot module's title level (its own container, TOC +0x14e8): its assets under <dir>/title/ (the levels'
// under <dir>/level<N>/), the renderers' level id while the boot runs
constexpr int kTitleLevel = 100;
constexpr int kFlightLevel = 101;  // the loading screen's scene: <dir>/flight/
std::string level_dir_name(int level);

// Reads <dir>/level<N>/world.mrpw (the title: <dir>/title/world.mrpw) and derives its images.
bool load_world(const std::string& dir, int level, World& w);

}  // namespace rac1::mrpw

#endif
