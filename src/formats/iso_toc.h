#ifndef RAC1_FORMATS_ISO_TOC_H
#define RAC1_FORMATS_ISO_TOC_H

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace rac1 {

// One level's entry in the R&C1 disc's table of contents.
//
// The table of contents sits at sector 1500 of the disc (sectors are 0x800 bytes). It starts with
// two words, a magic value of 1 and the table's size in bytes, and goes on with 8-byte rows whose
// first word is the sector of a level's header (or 0 for an unused row). A level's header is 0x2434
// bytes and bundles every range the level needs: its data (which holds the overlay), the gameplay
// tables, the audio, the music and the scenes. Its second word is its own size, 0x2434, which is
// how a header is told from any other row target. All sector numbers on the disc are absolute.
//
// Only the fields the setup tool needs are read here; the others can be added the same way.
struct Rac1LevelTocEntry {
    std::int32_t id;                    // logical level index (0..21)
    std::uint32_t header_lba;           // ISO LBA where the amalgamated header lives
    std::uint32_t data_lba;             // ISO LBA where the level data WAD content starts
    std::uint32_t data_sectors;         // size of level data WAD content in sectors (0x800)
    std::uint32_t gameplay_ntsc_lba;     // ISO LBA where the NTSC gameplay blob starts
    std::uint32_t gameplay_ntsc_sectors; // size of NTSC gameplay blob in sectors (0x800)
};

// Parses the R&C1 ISO table of contents starting at LBA 1500 and returns one
// Rac1LevelTocEntry per level found. Throws std::runtime_error on malformed
// TOC (bad magic, unreasonable size).
//
// `iso_data` should be a full memory view of the ISO file. Only the TOC
// region (~10KB starting at byte 1500*0x800) is read.
std::vector<Rac1LevelTocEntry> read_rac1_iso_toc(std::span<const std::byte> iso_data);

} // namespace rac1

#endif // RAC1_FORMATS_ISO_TOC_H
