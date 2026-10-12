#ifndef RAC1_FORMATS_SOUND_BANK_H
#define RAC1_FORMATS_SOUND_BANK_H

#include <cstdint>
#include <vector>
#include <span>
#include <string>

#include <unordered_map>

namespace rac1 {

struct SoundBankEntry {
    int grain_index = -1;
    std::uint8_t center_note = 0;
    std::uint8_t center_fine = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t stream_offset = 0;
    std::uint32_t stream_size = 0;
    std::uint16_t stream_flags = 0;
    std::vector<std::byte> adpcm_data;
};

struct RacSoundBank {
    std::uint32_t container_version = 0;
    std::uint32_t sblk_version = 0;
    std::vector<SoundBankEntry> entries;
    std::unordered_map<std::uint32_t, std::uint32_t> cue_to_grain;
};

// Parses a 989snd container from a raw byte span.
// Returns a parsed RacSoundBank structure, or throws std::runtime_error on invalid format.
RacSoundBank parse_sound_bank(std::span<const std::byte> data);

// Helper function to calculate play rate for note and fine offset.
int calculate_spu2_pitch(int16_t note, int16_t fine, std::uint8_t center_note, std::uint8_t center_fine);

// Helper to generate a VAGp header for an entry
std::vector<std::byte> generate_vag_header(const SoundBankEntry& entry, const std::string& name);

} // namespace rac1

#endif // RAC1_FORMATS_SOUND_BANK_H
