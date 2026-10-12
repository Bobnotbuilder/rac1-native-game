#ifndef RAC1_FORMATS_VIF_STREAM_H
#define RAC1_FORMATS_VIF_STREAM_H

// A VIF1 command list (the PS2's VU interface DMA stream), split into its commands. Shared by the
// world decoders (tfrag, shrub). The command sizes are the VIF's own (EE User's Manual, VIF
// chapter): one 32-bit code word, then the command's data.

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace rac1 {

struct VifCommand {
    std::uint32_t op;                  // the code's bits 24..30 (the interrupt bit dropped)
    std::uint32_t num;                 // bits 16..23, 0 meaning 256
    std::uint32_t format;              // bits 24..27: for an UNPACK, vn << 2 | vl
    std::uint32_t address;             // bits 0..9: for an UNPACK, the VU memory address
    std::vector<std::uint8_t> payload; // the words after the code

    bool is_unpack() const { return (op & 0x60) == 0x60; }
};

// The commands of `list`, in order, up to the first one that does not fit in it.
std::vector<VifCommand> split_vif_commands(std::span<const std::byte> list);

// Only the UNPACK commands of `commands`.
std::vector<VifCommand> vif_unpacks(const std::vector<VifCommand>& commands);

} // namespace rac1

#endif // RAC1_FORMATS_VIF_STREAM_H
