#include <formats/vif_stream.h>

#include <cstring>

namespace rac1 {

namespace {

// The command's total length in 32-bit words, its code word included.
std::size_t command_words(std::uint32_t code, std::uint32_t op, std::uint32_t num) {
    switch (op) {
    case 0x20: return 2;                                // STMASK
    case 0x30: case 0x31: return 5;                     // STROW, STCOL
    case 0x4a: return 1 + std::size_t{num} * 2;         // MPG: num 64-bit instructions
    case 0x50: case 0x51: {                             // DIRECT, DIRECTHL: imm quadwords (0: 65536)
        std::size_t qwords = code & 0xffff;
        if (qwords == 0) qwords = 0x10000;
        return 1 + qwords * 4;
    }
    default: break;
    }
    if ((op & 0x60) == 0x60) {                          // UNPACK: num elements of vn + 1 components
        const std::uint32_t vl = op & 3, vn = (op >> 2) & 3;
        const std::uint32_t bits = (32u >> vl) * (vn + 1);
        const std::size_t bytes = std::size_t{num} * (bits / 8);
        return 1 + (bytes + 3) / 4;
    }
    return 1;
}

} // namespace

std::vector<VifCommand> split_vif_commands(std::span<const std::byte> list) {
    std::vector<VifCommand> out;
    std::size_t at = 0;
    while (list.size() - at >= 4) {
        std::uint32_t code;
        std::memcpy(&code, list.data() + at, 4);
        VifCommand c;
        c.op = (code >> 24) & 0x7f;
        c.num = (code >> 16) & 0xff;
        if (c.num == 0) c.num = 256;
        c.format = (code >> 24) & 0xf;
        c.address = code & 0x3ff;
        const std::size_t bytes = command_words(code, c.op, c.num) * 4;
        if (bytes > list.size() - at) break;
        const auto* p = reinterpret_cast<const std::uint8_t*>(list.data() + at);
        c.payload.assign(p + 4, p + bytes);
        out.push_back(std::move(c));
        at += bytes;
    }
    return out;
}

std::vector<VifCommand> vif_unpacks(const std::vector<VifCommand>& commands) {
    std::vector<VifCommand> out;
    for (const VifCommand& c : commands) {
        if (c.is_unpack()) out.push_back(c);
    }
    return out;
}

} // namespace rac1
