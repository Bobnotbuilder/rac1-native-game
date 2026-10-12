#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace rac1::setup {

// The discs the setup knows. To support another release, add its row with supported = true once the
// port runs on it (a release other than the NTSC-U one also needs its region table,
// data/regions/<serial>.txt); the rest of the setup reads this table.
struct KnownDisc {
    const char* boot_file;     // the boot executable in the disc's root directory, e.g. "SCUS_971.99"
    const char* serial;        // as printed on the case, e.g. "SCUS-97199"
    const char* region;        // "NTSC-U", "PAL", "NTSC-J"
    uint64_t redump_size;      // a good dump's size in bytes (0: unknown)
    bool supported;
};
const KnownDisc* find_known_disc(const std::string& boot_file);

struct IsoValidation {
    bool valid = false;
    const KnownDisc* disc = nullptr;  // the release found (also set when it is not supported)
    std::string serial;               // the boot executable found in the root directory, if any
    uint64_t size = 0;
    std::string message;              // why it is not valid, or a warning when it is
};

IsoValidation validate_iso(const std::string& iso_path);
// the disc's boot executable (SCUS_971.99 on the NTSC-U disc, SCES_509.16 on the PAL one)
bool extract_boot_elf(std::span<const std::byte> iso, const std::string& out_path, bool force = false,
                      const std::string& boot_file = "SCUS_971.99");
bool extract_level_wads(std::span<const std::byte> iso, const std::string& out_dir, bool force = false);

} // namespace rac1::setup
