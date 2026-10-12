#pragma once

#include <cstddef>
#include <span>
#include <string>

namespace rac1::setup {

// The loading screen's scene: the ship's flight between planets, shown while the next level loads.
// Its moby classes and moby textures go to <moby_dir>/flight/, an empty world (the scene draws only
// its sky and its mobys) to <world_dir>/flight/. Files that exist are kept unless `force`.
bool export_flight(std::span<const std::byte> iso, const std::string& moby_dir, const std::string& world_dir, bool force = false);

} // namespace rac1::setup
