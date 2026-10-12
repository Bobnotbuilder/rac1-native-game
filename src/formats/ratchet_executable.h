#ifndef RAC1_FORMATS_RATCHET_EXECUTABLE_H
#define RAC1_FORMATS_RATCHET_EXECUTABLE_H

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace rac1 {

// The "Ratchet Executable" is Insomniac's own executable container. It holds the per-level overlays
// (and the boot module's overlay) in the global WAD. The base ELF SCUS_971.99 has stubs (jr ra; nop)
// at 0x001e93f0..0x001e9488; the overlay loader copies each block of a Ratchet Executable to the
// address its header names, then patches those stubs with jumps into the overlay.
//
// On disc it is a plain sequence of blocks, each a 16-byte header followed by the block's bytes:
//
//   +0  s32  load_address   where in EE memory the bytes are copied to
//   +4  s32  byte_count     how many bytes follow the header
//   +8  s32  elf_type       the ELF section type the block stands for (1 = PROGBITS, 8 = NOBITS, ...)
//   +12 s32  entry          the overlay's entry point, repeated in every block
//
// The sequence has no count and no end marker. Every block repeats the same entry point, so the
// game's own loader (and this parser) stops at the first block whose entry point differs from the
// first block's, or when too few bytes are left for another header. The game's loader reads one
// header past the end before it notices; this parser does not.

struct OverlayBlockHeader {
    std::int32_t load_address;
    std::int32_t byte_count;
    std::int32_t elf_type;
    std::int32_t entry;
};

static_assert(sizeof(OverlayBlockHeader) == 16);

struct RatchetSection {
    int index;                              // position of the block in the input, from 0
    OverlayBlockHeader header;              // the block's header as stored
    std::vector<std::uint8_t> data;         // the block's bytes (header.byte_count of them)
};

struct RatchetExecutable {
    std::int32_t entry_point = 0;           // the entry point of the first block (all blocks agree)
    std::vector<RatchetSection> sections;
};

// Parses a Ratchet Executable from raw bytes. Throws std::runtime_error when a block claims a
// negative size or more bytes than the input has left.
RatchetExecutable parse_ratchet_executable(std::span<const std::byte> data);

// Builds a minimal ELF32 little-endian MIPS file from the parsed overlay, so that Ghidra or any
// ELF tool can load it at the addresses the PS2 loader would have used. Each block becomes one
// PT_LOAD segment at its load address and one section (.rac1_overlay_N) of its ELF type. The file
// is ET_EXEC with program headers: an ET_REL file makes Ghidra rebase the whole image.
std::vector<std::uint8_t> reconstruct_elf(const RatchetExecutable& exe);

} // namespace rac1

#endif // RAC1_FORMATS_RATCHET_EXECUTABLE_H
