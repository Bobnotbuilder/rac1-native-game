#ifndef RAC1_FORMATS_VAG_H
#define RAC1_FORMATS_VAG_H

#include <vector>
#include <string>
#include <span>
#include <cstdint>

namespace rac1 {

struct VagHeader {
    std::uint32_t version;
    std::uint32_t data_size;
    std::uint32_t sample_rate;
    std::string name;
};

// Parses a VAG file header.
// Returns true on success, false if the file is too small or does not start with "VAGp".
bool parse_vag_header(std::span<const std::byte> data, VagHeader& header);

// Decodes VAG ADPCM audio data into 16-bit signed PCM samples.
// The data span should point to the raw ADPCM payload (excluding the 48-byte header).
// Returns the decoded 16-bit PCM samples.
std::vector<std::int16_t> decode_vag_adpcm(std::span<const std::byte> adpcm_data);

} // namespace rac1

#endif // RAC1_FORMATS_VAG_H
