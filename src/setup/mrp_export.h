#pragma once

#include <cstddef>
#include <span>
#include <string>

namespace rac1::setup {

bool export_mrp_mobys(std::span<const std::byte> iso, const std::string& out_dir, const std::string& level_arg = "all", bool force = false);
bool export_mrp_world(std::span<const std::byte> iso, const std::string& out_dir, const std::string& level_arg = "all", bool force = false);

} // namespace rac1::setup
