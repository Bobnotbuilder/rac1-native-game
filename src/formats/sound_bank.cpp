#include "sound_bank.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace rac1 {

namespace {

const uint16_t note_pitch_table[12] = {
    0x8000, 0x879C, 0x8FAC, 0x9837, 0xA145, 0xAADC,
    0xB504, 0xBFC8, 0xCB2F, 0xD744, 0xE411, 0xF1A1
};

const uint16_t fine_pitch_table[128] = {
    0x8000, 0x800E, 0x801D, 0x802C, 0x803B, 0x804A, 0x8058, 0x8067,
    0x8076, 0x8085, 0x8094, 0x80A3, 0x80B1, 0x80C0, 0x80CF, 0x80DE,
    0x80ED, 0x80FC, 0x810B, 0x811A, 0x8129, 0x8138, 0x8146, 0x8155,
    0x8164, 0x8173, 0x8182, 0x8191, 0x81A0, 0x81AF, 0x81BE, 0x81CD,
    0x81DC, 0x81EB, 0x81FA, 0x8209, 0x8218, 0x8227, 0x8236, 0x8245,
    0x8254, 0x8263, 0x8272, 0x8282, 0x8291, 0x82A0, 0x82AF, 0x82BE,
    0x82CD, 0x82DC, 0x82EB, 0x82FA, 0x830A, 0x8319, 0x8328, 0x8337,
    0x8346, 0x8355, 0x8364, 0x8374, 0x8383, 0x8392, 0x83A1, 0x83B0,
    0x83C0, 0x83CF, 0x83DE, 0x83ED, 0x83FD, 0x840C, 0x841B, 0x842A,
    0x843A, 0x8449, 0x8458, 0x8468, 0x8477, 0x8486, 0x8495, 0x84A5,
    0x84B4, 0x84C3, 0x84D3, 0x84E2, 0x84F1, 0x8501, 0x8510, 0x8520,
    0x852F, 0x853E, 0x854E, 0x855D, 0x856D, 0x857C, 0x858B, 0x859B,
    0x85AA, 0x85BA, 0x85C9, 0x85D9, 0x85E8, 0x85F8, 0x8607, 0x8617,
    0x8626, 0x8636, 0x8645, 0x8655, 0x8664, 0x8674, 0x8683, 0x8693,
    0x86A2, 0x86B2, 0x86C1, 0x86D1, 0x86E0, 0x86F0, 0x8700, 0x870F,
    0x871F, 0x872E, 0x873E, 0x874E, 0x875D, 0x876D, 0x877D, 0x878C
};

uint16_t ps_note_to_pitch(uint16_t center_note, uint16_t center_fine, uint16_t note, int16_t fine) {
    int fine_adjust, fine_idx, note_adjust, note_idx;
    int unk1, unk2, unk3;
    uint16_t pitch;

    fine_idx = fine + center_fine;

    fine_adjust = fine_idx;
    if (fine_idx < 0)
        fine_adjust = fine_idx + 0x7F;

    fine_adjust /= 128;
    note_adjust = note + fine_adjust - center_note;
    unk3 = note_adjust / 6;

    if (note_adjust < 0)
        unk3--;

    fine_idx -= fine_adjust * 128;

    if (note_adjust < 0)
        unk2 = -1;
    else
        unk2 = 0;
    if (unk3 < 0)
        unk3--;

    unk2 = (unk3 / 2) - unk2;
    unk1 = unk2 - 2;
    note_idx = note_adjust - (unk2 * 12);

    if ((note_idx < 0) || ((note_idx == 0) && (fine_idx < 0))) {
        note_idx += 12;
        unk1 = unk2 - 3;
    }

    if (fine_idx < 0) {
        note_idx = (note_idx - 1) + fine_adjust;
        fine_idx += (fine_adjust + 1) * 128;
    }

    pitch = (note_pitch_table[note_idx] * fine_pitch_table[fine_idx]) >> 16;

    if (unk1 < 0)
        pitch = (pitch + (1 << (-unk1 - 1))) >> -unk1;

    return pitch;
}

int spu2_note_to_pitch(int16_t note, int16_t fine, uint8_t center_note, uint8_t center_fine) {
    bool is_negative = (center_note >> 7) != 0;

    if (is_negative)
        center_note = 0x100 - center_note;

    int pitch = ps_note_to_pitch(center_note, center_fine, note, fine);
    if (pitch > 0x4000)
        pitch = 0x4000; // 192000 Hz max

    // orig: 0x00216788 (spu2 note to pitch - remove 44100/48000 host double scaling)
    return pitch;
}

std::uint32_t read_u32_le(const std::byte* ptr) {
    return (static_cast<std::uint32_t>(static_cast<std::uint8_t>(ptr[0]))) |
           (static_cast<std::uint32_t>(static_cast<std::uint8_t>(ptr[1])) << 8) |
           (static_cast<std::uint32_t>(static_cast<std::uint8_t>(ptr[2])) << 16) |
           (static_cast<std::uint32_t>(static_cast<std::uint8_t>(ptr[3])) << 24);
}

std::uint16_t read_u16_le(const std::byte* ptr) {
    return (static_cast<std::uint16_t>(static_cast<std::uint8_t>(ptr[0]))) |
           (static_cast<std::uint16_t>(static_cast<std::uint8_t>(ptr[1])) << 8);
}

} // namespace

int calculate_spu2_pitch(int16_t note, int16_t fine, std::uint8_t center_note, std::uint8_t center_fine) {
    return spu2_note_to_pitch(note, fine, center_note, center_fine);
}

RacSoundBank parse_sound_bank(std::span<const std::byte> data) {
    if (data.size() < 32) {
        throw std::runtime_error("989snd container is too small to contain a header");
    }

    RacSoundBank sb;
    sb.container_version = read_u32_le(data.data() + 0);
    if (sb.container_version != 1 && sb.container_version != 3) {
        throw std::runtime_error("Unsupported 989snd container version: " + std::to_string(sb.container_version));
    }

    std::uint32_t sblk_offset = read_u32_le(data.data() + 8);
    std::uint32_t data_offset = read_u32_le(data.data() + 16);

    if (sblk_offset + 32 > data.size()) {
        throw std::runtime_error("SBlk offset is out of bounds");
    }

    // Check SBlk Magic "SBlk" or "SBv2"
    bool valid_magic = (data[sblk_offset + 0] == std::byte('S') && data[sblk_offset + 1] == std::byte('B') &&
                        data[sblk_offset + 2] == std::byte('l') && data[sblk_offset + 3] == std::byte('k')) ||
                       (data[sblk_offset + 0] == std::byte('S') && data[sblk_offset + 1] == std::byte('B') &&
                        data[sblk_offset + 2] == std::byte('v') && data[sblk_offset + 3] == std::byte('2'));
    if (!valid_magic) {
        throw std::runtime_error("Invalid SBlk magic (expected SBlk or SBv2)");
    }

    sb.sblk_version = read_u32_le(data.data() + sblk_offset + 4);
    if (sb.sblk_version != 1) {
        throw std::runtime_error("Unsupported SBlk version: " + std::to_string(sb.sblk_version));
    }

    // Parse SBlk Table 1 (cues) mapping cue_id -> grain_index
    // SBlk+0x16 holds sounds_entries count, SBlk+0x1c holds table1_offset (relative to sblk).
    // Each cue entry is 12 bytes. Offset 8 is the 32-bit byte offset of the grain in Table 2.
    std::uint16_t sounds_entries = read_u16_le(data.data() + sblk_offset + 0x16);
    std::uint32_t table1_offset = read_u32_le(data.data() + sblk_offset + 0x1c);
    std::uint32_t table1_abs = sblk_offset + table1_offset;

    if (sounds_entries > 0 && table1_offset > 0) {
        if (table1_abs + sounds_entries * 12 > data.size()) {
            throw std::runtime_error("Cues table is out of bounds");
        }
        for (int i = 0; i < sounds_entries; ++i) {
            std::uint32_t entry_offset = table1_abs + i * 12;
            std::uint32_t grain_offset = read_u32_le(data.data() + entry_offset + 8);
            sb.cue_to_grain[i] = grain_offset / 40; // 40 (0x28) is Table 2 grain stride
        }
    }

    std::uint16_t grains_entries = read_u16_le(data.data() + sblk_offset + 0x18);
    std::uint32_t table2_offset = read_u32_le(data.data() + sblk_offset + 0x20);

    std::uint32_t table2_abs = sblk_offset + table2_offset;
    if (table2_abs + grains_entries * 0x28 > data.size()) {
        throw std::runtime_error("Grains table is out of bounds");
    }

    for (int i = 0; i < grains_entries; ++i) {
        std::uint32_t entry_offset = table2_abs + i * 0x28;
        std::uint32_t t2_type = read_u32_le(data.data() + entry_offset);
        if (t2_type == 1) { // Sound grain type
            std::uint32_t sndh_offset = entry_offset + 8;
            std::uint8_t center_note = static_cast<std::uint8_t>(data[sndh_offset + 2]);
            std::uint8_t center_fine = static_cast<std::uint8_t>(data[sndh_offset + 3]);
            std::uint16_t stream_flags = read_u16_le(data.data() + sndh_offset + 14);
            std::uint32_t stream_offset = read_u32_le(data.data() + sndh_offset + 16);

            SoundBankEntry entry;
            entry.grain_index = i;
            entry.center_note = center_note;
            entry.center_fine = center_fine;
            entry.stream_flags = stream_flags;
            entry.stream_offset = stream_offset;

            // Calculate sample rate via SPU2 pitch calculation
            int pitch = calculate_spu2_pitch(60, 0, center_note, center_fine);
            entry.sample_rate = (48000 * pitch) / 4096;

            // Stream Scanning for Size
            std::uint32_t start_abs = data_offset + stream_offset;
            std::uint32_t stream_size = 0;
            if (start_abs < data.size()) {
                stream_size = 16; // minimum one block
                for (std::uint32_t offset = start_abs + 16; offset + 16 <= data.size(); offset += 16) {
                    const std::uint8_t* block = reinterpret_cast<const std::uint8_t*>(data.data() + offset);

                    // 1. All-zeros block
                    bool all_zeros = true;
                    for (int k = 0; k < 16; ++k) {
                        if (block[k] != 0) {
                            all_zeros = false;
                            break;
                        }
                    }
                    if (all_zeros) {
                        break;
                    }

                    stream_size += 16;

                    // 2. Loop end / terminator frames:
                    // 00 07 77 77 77 77 77 77 77 77 77 77 77 77 77 77
                    // or 00 07 00 00 00 00 00 00 00 00 00 00 00 00 00 00
                    bool is_terminator = false;
                    if (block[0] == 0x00 && block[1] == 0x07) {
                        bool all_77 = true;
                        bool all_00 = true;
                        for (int k = 2; k < 16; ++k) {
                            if (block[k] != 0x77) all_77 = false;
                            if (block[k] != 0x00) all_00 = false;
                        }
                        if (all_77 || all_00) {
                            is_terminator = true;
                        }
                    }
                    if (is_terminator) {
                        break;
                    }

                    // 3. SPU block flags has End Flag set (block[1] & 0x01)
                    if (block[1] & 0x01) {
                        break;
                    }
                }
            }

            entry.stream_size = stream_size;
            if (start_abs < data.size() && stream_size > 0) {
                std::uint32_t final_size = std::min(stream_size, static_cast<std::uint32_t>(data.size() - start_abs));
                entry.adpcm_data.resize(final_size);
                std::copy(data.data() + start_abs, data.data() + start_abs + final_size, entry.adpcm_data.data());
            }

            sb.entries.push_back(entry);
        }
    }

    return sb;
}

std::vector<std::byte> generate_vag_header(const SoundBankEntry& entry, const std::string& name) {
    std::vector<std::byte> header(48, std::byte{0});

    // Magic VAGp
    header[0] = std::byte{'V'};
    header[1] = std::byte{'A'};
    header[2] = std::byte{'G'};
    header[3] = std::byte{'p'};

    // Version 4 (Big Endian)
    header[7] = std::byte{4};

    // Data size (Big Endian)
    std::uint32_t data_size = entry.adpcm_data.size();
    header[12] = static_cast<std::byte>((data_size >> 24) & 0xFF);
    header[13] = static_cast<std::byte>((data_size >> 16) & 0xFF);
    header[14] = static_cast<std::byte>((data_size >> 8) & 0xFF);
    header[15] = static_cast<std::byte>(data_size & 0xFF);

    // Sample rate (Big Endian)
    std::uint32_t rate = entry.sample_rate;
    header[16] = static_cast<std::byte>((rate >> 24) & 0xFF);
    header[17] = static_cast<std::byte>((rate >> 16) & 0xFF);
    header[18] = static_cast<std::byte>((rate >> 8) & 0xFF);
    header[19] = static_cast<std::byte>(rate & 0xFF);

    // Name (max 15 characters, nul-terminated)
    std::size_t copy_len = std::min(name.size(), std::size_t{15});
    for (std::size_t i = 0; i < copy_len; ++i) {
        header[32 + i] = static_cast<std::byte>(name[i]);
    }

    return header;
}

} // namespace rac1
