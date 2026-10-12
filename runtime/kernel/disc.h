// The disc: the game's CD / DVD reads served from the user's ISO image (the host stands in for
// the IOP's CDVD driver). Sectors are 2048 bytes.
#pragma once

#include <cstdint>
#include <string>

namespace rac1 {

// The ISO's path: $RAC1_ISO, else assets/iso/rac1-ntsc-u.iso.
std::string disc_path();
// The disc's boot ELF as its SYSTEM.CNF names it (BOOT2 = cdrom0:\XXXX_NNN.NN): "SCUS_971.99" on the
// NTSC-U disc (SCUS-97199), "SCES_509.16" on the PAL one (SCES-50916); "SCUS_971.99" when the disc cannot be read.
// Read once, through its own handle (the game's reads are not touched).
const std::string& disc_boot_elf();
// "assets/dumps/" + disc_boot_elf(): the boot ELF setup.sh extracted from that disc.
std::string disc_boot_elf_path();
// The disc is not the NTSC-U build the ports are written against (the PAL disc): the ports' overlay
// addresses go through the region's maps on every level, Veldin's included (src/game/ovl/ovl_reloc.cpp,
//).
bool disc_other_region();
// Opens the ISO (default assets/iso/rac1-ntsc-u.iso, or $RAC1_ISO). False if it cannot be read.
bool disc_open(const std::string& path = "");
bool disc_is_open();
// Reads `sectors` sectors from `lba` to the host buffer `dst`. False past the end of the image.
bool disc_read(uint32_t lba, uint32_t sectors, void* dst);
// Reads to EE memory at `ee_dst`.
bool disc_read_ee(uint32_t ee_dst, uint32_t lba, uint32_t sectors);
// The host asks its OS to read these sectors ahead (into its page cache, in the background): a load about
// to read them (a level's files at the level exit's start). Nothing is read into the game's memory.
void disc_will_need(uint32_t lba, uint32_t sectors);

}  // namespace rac1
