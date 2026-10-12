#include "vif_unpack.h"
#include <cstring>
#include <iostream>

namespace rac1 {

static inline float uint_to_float(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

static inline uint32_t float_to_uint(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}

static uint32_t apply_mask(VifState& vif_state, uint32_t data, int component, int cycle, bool is_write, uint32_t original) {
    int mask_shift = ((cycle % 4) * 8) + (component * 2);
    uint32_t mask_val = (vif_state.mask >> mask_shift) & 3;
    if (!is_write) mask_val = 3; // If not written by unpack, it is masked out (or filled)

    switch (mask_val) {
        case 0: return data;
        case 1: 
            if (component == 0) return vif_state.r0;
            if (component == 1) return vif_state.r1;
            if (component == 2) return vif_state.r2;
            return vif_state.r3;
        case 2:
            if (component == 0) return vif_state.c0;
            if (component == 1) return vif_state.c1;
            if (component == 2) return vif_state.c2;
            return vif_state.c3;
        case 3:
        default:
            return original;
    }
}

void unpack_vifcode(VUState& vu_state, VifState& vif_state, const uint8_t* stream, size_t size) {
    size_t offset = 0;
    while (offset < size) {
        // Ensure we have at least a 32-bit VIFcode
        if (offset + 4 > size) break;
        
        uint32_t vifcode;
        std::memcpy(&vifcode, stream + offset, 4);
        offset += 4;
        
        uint32_t cmd = (vifcode >> 24) & 0xFF;
        uint32_t num = (vifcode >> 16) & 0xFF;
        uint32_t imm = vifcode & 0xFFFF;
        
        // STMASK
        if ((cmd & 0x7F) == 0x20) {
            if (offset + 4 <= size) {
                std::memcpy(&vif_state.mask, stream + offset, 4);
                offset += 4;
            }
            continue;
        }
        
        // STROW
        if ((cmd & 0x7F) == 0x30) {
            if (offset + 16 <= size) {
                std::memcpy(&vif_state.r0, stream + offset, 4);
                std::memcpy(&vif_state.r1, stream + offset + 4, 4);
                std::memcpy(&vif_state.r2, stream + offset + 8, 4);
                std::memcpy(&vif_state.r3, stream + offset + 12, 4);
                offset += 16;
            }
            continue;
        }
        
        // STCOL
        if ((cmd & 0x7F) == 0x31) {
            if (offset + 16 <= size) {
                std::memcpy(&vif_state.c0, stream + offset, 4);
                std::memcpy(&vif_state.c1, stream + offset + 4, 4);
                std::memcpy(&vif_state.c2, stream + offset + 8, 4);
                std::memcpy(&vif_state.c3, stream + offset + 12, 4);
                offset += 16;
            }
            continue;
        }
        
        // NOP
        if (cmd == 0x00) continue;
        
        // Check for UNPACK
        if ((cmd & 0x60) == 0x60) {
            bool m_bit = (cmd & 0x10) != 0;
            uint32_t vn = (cmd >> 2) & 3;
            uint32_t vl = cmd & 3;
            uint32_t addr = imm & 0x3FF;
            bool usn = (imm & 0x4000) != 0;
            
            if (num == 0) num = 256;
            
            for (uint32_t i = 0; i < num; ++i) {
                uint32_t dest_addr = (addr + i) * 16;
                if (dest_addr >= sizeof(vu_state.mem)) break;
                
                uint32_t* mem_ptr = reinterpret_cast<uint32_t*>(&vu_state.mem[dest_addr]);
                uint32_t orig[4] = {mem_ptr[0], mem_ptr[1], mem_ptr[2], mem_ptr[3]};
                uint32_t data[4] = {0, 0, 0, 0};
                bool write_mask[4] = {false, false, false, false};
                
                auto read_val = [&]() -> uint32_t {
                    if (vl == 0) { // 32-bit
                        if (offset + 4 > size) return 0;
                        uint32_t v; std::memcpy(&v, stream + offset, 4); offset += 4;
                        return v;
                    } else if (vl == 1) { // 16-bit
                        if (offset + 2 > size) return 0;
                        uint16_t v; std::memcpy(&v, stream + offset, 2); offset += 2;
                        return usn ? (uint32_t)v : (uint32_t)(int32_t)(int16_t)v;
                    } else if (vl == 2) { // 8-bit
                        if (offset + 1 > size) return 0;
                        uint8_t v; std::memcpy(&v, stream + offset, 1); offset += 1;
                        return usn ? (uint32_t)v : (uint32_t)(int32_t)(int8_t)v;
                    }
                    return 0; // V4-5 not supported
                };
                
                if (vn == 0) { // S
                    uint32_t val = read_val();
                    data[0] = data[1] = data[2] = data[3] = val;
                    write_mask[0] = write_mask[1] = write_mask[2] = write_mask[3] = true;
                } else if (vn == 1) { // V2
                    data[0] = read_val(); data[1] = read_val();
                    write_mask[0] = write_mask[1] = true;
                } else if (vn == 2) { // V3
                    data[0] = read_val(); data[1] = read_val(); data[2] = read_val();
                    write_mask[0] = write_mask[1] = write_mask[2] = true;
                } else if (vn == 3) { // V4
                    if (vl == 3) { // V4-5 special
                        if (offset + 2 > size) break;
                        uint16_t v; std::memcpy(&v, stream + offset, 2); offset += 2;
                        data[0] = (v & 0x001F) << 3;
                        data[1] = ((v >> 5) & 0x001F) << 3;
                        data[2] = ((v >> 10) & 0x001F) << 3;
                        data[3] = ((v >> 15) & 0x0001) << 7;
                    } else {
                        data[0] = read_val(); data[1] = read_val(); data[2] = read_val(); data[3] = read_val();
                    }
                    write_mask[0] = write_mask[1] = write_mask[2] = write_mask[3] = true;
                }
                
                for (int c = 0; c < 4; ++c) {
                    if (m_bit) {
                        mem_ptr[c] = apply_mask(vif_state, data[c], c, i, write_mask[c], orig[c]);
                    } else {
                        if (write_mask[c]) {
                            mem_ptr[c] = data[c];
                        } else {
                            // Default fill from ROW if not written and no MASK is applied
                            if (c == 0) mem_ptr[c] = vif_state.r0;
                            else if (c == 1) mem_ptr[c] = vif_state.r1;
                            else if (c == 2) mem_ptr[c] = vif_state.r2;
                            else mem_ptr[c] = vif_state.r3;
                        }
                    }
                }
            }
            
            // Align offset to 4 bytes after unpack
            while (offset % 4 != 0) offset++;
        }
    }
}

} // namespace rac1
