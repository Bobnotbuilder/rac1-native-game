#include "vag.h"
#include <algorithm>

namespace rac1 {

bool parse_vag_header(std::span<const std::byte> data, VagHeader& header) {
    if (data.size() < 48) {
        return false;
    }
    
    // Check magic "VAGp"
    if (data[0] != std::byte('V') || data[1] != std::byte('A') ||
        data[2] != std::byte('G') || data[3] != std::byte('p')) {
        return false;
    }
    
    auto read_u32_be = [](const std::byte* ptr) -> std::uint32_t {
        return (static_cast<std::uint32_t>(static_cast<std::uint8_t>(ptr[0])) << 24) |
               (static_cast<std::uint32_t>(static_cast<std::uint8_t>(ptr[1])) << 16) |
               (static_cast<std::uint32_t>(static_cast<std::uint8_t>(ptr[2])) << 8) |
               (static_cast<std::uint32_t>(static_cast<std::uint8_t>(ptr[3])));
    };
    
    header.version = read_u32_be(data.data() + 4);
    header.data_size = read_u32_be(data.data() + 12);
    header.sample_rate = read_u32_be(data.data() + 16);
    
    // Extract name (offset 0x20, size 16)
    const char* name_ptr = reinterpret_cast<const char*>(data.data() + 32);
    
    // Find length of NUL-padded string up to 16 characters
    std::size_t name_len = 0;
    while (name_len < 16 && name_ptr[name_len] != '\0') {
        name_len++;
    }
    header.name = std::string(name_ptr, name_len);
    
    return true;
}

std::vector<std::int16_t> decode_vag_adpcm(std::span<const std::byte> adpcm_data) {
    // Coefficients scaled by 64
    static constexpr std::int32_t K0[] = { 0, 60, 115, 98, 122 };
    static constexpr std::int32_t K1[] = { 0, 0, -52, -55, -60 };

    std::vector<std::int16_t> decoded_samples;
    
    // Each 16-byte block yields 28 samples
    std::size_t num_blocks = adpcm_data.size() / 16;
    decoded_samples.reserve(num_blocks * 28);
    
    std::int32_t s1 = 0; // hist1
    std::int32_t s2 = 0; // hist2
    
    for (std::size_t block_idx = 0; block_idx < num_blocks; ++block_idx) {
        const std::uint8_t* block = reinterpret_cast<const std::uint8_t*>(adpcm_data.data() + block_idx * 16);
        
        std::uint8_t shift = block[0] & 0x0F;
        std::uint8_t filter = (block[0] >> 4) & 0x0F;
        
        // SPU ADPCM has 5 filters (0..4). If filter > 4, default to 0.
        if (filter > 4) {
            filter = 0;
        }
        
        // Shift factor is capped at 12. If shift > 12, default to 9.
        if (shift > 12) {
            shift = 9;
        }
        
        std::int32_t coef0 = K0[filter];
        std::int32_t coef1 = K1[filter];
        
        for (int i = 0; i < 28; ++i) {
            // Data bytes are at offset 2..15 in the block
            int byte_offset = 2 + (i / 2);
            std::uint8_t b = block[byte_offset];
            
            // Extract nibble: even index gets low nibble, odd gets high nibble
            std::int8_t nibble = (i % 2 == 0) ? (b & 0x0F) : (b >> 4);
            
            // Sign-extend 4-bit nibble to 8-bit signed integer
            if (nibble & 0x08) {
                nibble |= 0xF0;
            }
            
            // Scale nibble by shift factor
            std::int32_t sample = static_cast<std::int32_t>(nibble) << (12 - shift);
            
            // Apply filter (using previous samples and coefficients scaled by 64)
            std::int32_t prediction = (sample << 6) + (s1 * coef0) + (s2 * coef1);
            
            // Round and divide by 64
            std::int32_t decoded = (prediction + 32) >> 6;
            
            // Clamp to 16-bit signed integer range
            if (decoded > 32767) {
                decoded = 32767;
            } else if (decoded < -32768) {
                decoded = -32768;
            }
            
            decoded_samples.push_back(static_cast<std::int16_t>(decoded));
            
            // Update history
            s2 = s1;
            s1 = decoded;
        }
    }
    
    return decoded_samples;
}

} // namespace rac1
