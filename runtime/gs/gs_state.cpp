#include <runtime/gs/gs_state.h>
#include <runtime/kernel/ee_memory.h>
#include <cstring>
#include <vector>
#include <iostream>

namespace rac1 {

GsState* g_active_gs_state = nullptr;
bool g_parsing_sky = false;

void GsState::reset() {
    prim = 0; rgbaq = 0; st = 0; uv = 0; fog = 0;
    tex0_1 = 0; tex0_2 = 0; clamp_1 = 0; clamp_2 = 0;
    tex1_1 = 0; tex1_2 = 0; tex2_1 = 0; tex2_2 = 0;
    xyoffset_1 = 0x710000006c00ULL; xyoffset_2 = 0x710000006c00ULL; prmodecont = 1; prmode = 0;
    texclut = 0; scanmsk = 0; miptbp1_1 = 0; miptbp1_2 = 0;
    miptbp2_1 = 0; miptbp2_2 = 0; texa = 0; fogcol = 0;
    texflush = 0; scissor_1 = 0; scissor_2 = 0;
    alpha_1 = 0; alpha_2 = 0; dimx = 0; dthe = 0;
    colclamp = 0; test_1 = 0; test_2 = 0; pabe = 0;
    fba_1 = 0; fba_2 = 0; frame_1 = 0; frame_2 = 0;
    zbuf_1 = 0; zbuf_2 = 0; bitbltbuf = 0; trxpos = 0;
    trxreg = 0; trxdir = 0; hwreg = 0; signal = 0;
    finish = 0; label = 0;

    vertex_queue.clear();
    primitives.clear();
}

static void capture_prim_state(const GsState& state, GsPrimitive& prim) {
    int ctxt = 0;
    if ((state.prmodecont & 1) != 0) {
        ctxt = (state.prim >> 9) & 1;
    } else {
        ctxt = (state.prmode >> 9) & 1;
    }

    prim.prim_reg = state.prim;
    prim.tex0_reg = (ctxt == 0) ? state.tex0_1 : state.tex0_2;
    prim.alpha_reg = (ctxt == 0) ? state.alpha_1 : state.alpha_2;
    prim.scissor_reg = (ctxt == 0) ? state.scissor_1 : state.scissor_2;
    prim.test_reg = (ctxt == 0) ? state.test_1 : state.test_2;
    prim.frame_reg = (ctxt == 0) ? state.frame_1 : state.frame_2;
    prim.zbuf_reg = (ctxt == 0) ? state.zbuf_1 : state.zbuf_2;
    prim.is_sky = g_parsing_sky;
}

static void assemble_primitive(GsState& state, PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Point: {
            if (state.vertex_queue.size() >= 1) {
                GsPrimitive prim;
                prim.type = type;
                prim.vertices = { state.vertex_queue.back() };
                capture_prim_state(state, prim);
                state.primitives.push_back(prim);
                state.vertex_queue.clear();
            }
            break;
        }
        case PrimitiveType::Line: {
            if (state.vertex_queue.size() >= 2) {
                GsPrimitive prim;
                prim.type = type;
                prim.vertices = { state.vertex_queue[state.vertex_queue.size() - 2], state.vertex_queue.back() };
                capture_prim_state(state, prim);
                state.primitives.push_back(prim);
                state.vertex_queue.clear();
            }
            break;
        }
        case PrimitiveType::LineStrip: {
            if (state.vertex_queue.size() >= 2) {
                const auto& v_prev = state.vertex_queue[state.vertex_queue.size() - 2];
                const auto& v_curr = state.vertex_queue.back();
                if (v_curr.adc) {
                    state.vertex_queue = { v_curr };
                } else {
                    GsPrimitive prim;
                    prim.type = type;
                    prim.vertices = { v_prev, v_curr };
                    capture_prim_state(state, prim);
                    state.primitives.push_back(prim);
                    state.vertex_queue = { v_curr };
                }
            }
            break;
        }
        case PrimitiveType::Triangle: {
            if (state.vertex_queue.size() >= 3) {
                GsPrimitive prim;
                prim.type = type;
                prim.vertices = {
                    state.vertex_queue[state.vertex_queue.size() - 3],
                    state.vertex_queue[state.vertex_queue.size() - 2],
                    state.vertex_queue.back()
                };
                capture_prim_state(state, prim);
                state.primitives.push_back(prim);
                state.vertex_queue.clear();
            }
            break;
        }
        case PrimitiveType::TriangleStrip: {
            if (state.vertex_queue.size() >= 3) {
                const auto& v_curr = state.vertex_queue.back();
                if (v_curr.adc) {
                    state.vertex_queue = { v_curr };
                } else {
                    size_t sz = state.vertex_queue.size();
                    GsPrimitive prim;
                    prim.type = type;
                    prim.vertices = {
                        state.vertex_queue[sz - 3],
                        state.vertex_queue[sz - 2],
                        v_curr
                    };
                    capture_prim_state(state, prim);
                    state.primitives.push_back(prim);
                    state.vertex_queue = { state.vertex_queue[sz - 2], v_curr };
                }
            }
            break;
        }
        case PrimitiveType::TriangleFan: {
            if (state.vertex_queue.size() >= 3) {
                const auto& v_curr = state.vertex_queue.back();
                if (v_curr.adc) {
                    state.vertex_queue = { v_curr };
                } else {
                    size_t sz = state.vertex_queue.size();
                    GsPrimitive prim;
                    prim.type = type;
                    prim.vertices = {
                        state.vertex_queue[0],
                        state.vertex_queue[sz - 2],
                        v_curr
                    };
                    capture_prim_state(state, prim);
                    state.primitives.push_back(prim);
                    state.vertex_queue = { state.vertex_queue[0], v_curr };
                }
            }
            break;
        }
        case PrimitiveType::Sprite: {
            if (state.vertex_queue.size() >= 2) {
                GsPrimitive prim;
                prim.type = type;
                prim.vertices = { state.vertex_queue[state.vertex_queue.size() - 2], state.vertex_queue.back() };
                capture_prim_state(state, prim);
                state.primitives.push_back(prim);
                state.vertex_queue.clear();
            }
            break;
        }
        default:
            break;
    }
}

static void handle_vertex_kick(GsState& state, uint64_t val, bool kick, bool is_fog_format) {
    int ctxt = 0;
    if ((state.prmodecont & 1) != 0) {
        ctxt = (state.prim >> 9) & 1;
    } else {
        ctxt = (state.prmode >> 9) & 1;
    }

    uint64_t xyoffset = (ctxt == 0) ? state.xyoffset_1 : state.xyoffset_2;
    uint16_t ofx = (uint16_t)(xyoffset & 0xFFFF);
    uint16_t ofy = (uint16_t)((xyoffset >> 32) & 0xFFFF);

    uint16_t reg_x = (uint16_t)(val & 0xFFFF);
    uint16_t reg_y = (uint16_t)((val >> 16) & 0xFFFF);

    float x = (float)(int16_t)(reg_x - ofx) / 16.0f;
    float y = (float)(int16_t)(reg_y - ofy) / 16.0f;

    float z = 0.0f;
    float f = (float)state.fog;
    bool adc = false;

    if (is_fog_format) {
        uint32_t z_val = (val >> 32) & 0x00FFFFFF;
        z = (float)z_val;
        f = (float)((val >> 56) & 0xFF);
    } else {
        uint32_t z_val = (val >> 32) & 0x7FFFFFFF;
        z = (float)z_val;
        adc = (val & (1ULL << 63)) != 0;
    }

    uint8_t r = state.rgbaq & 0xFF;
    uint8_t g = (state.rgbaq >> 8) & 0xFF;
    uint8_t b = (state.rgbaq >> 16) & 0xFF;
    uint8_t a = (state.rgbaq >> 24) & 0xFF;

    float s = 0.0f;
    float t = 0.0f;
    std::memcpy(&s, &state.st, sizeof(float));
    uint32_t t_bits = (state.st >> 32) & 0xFFFFFFFF;
    std::memcpy(&t, &t_bits, sizeof(float));

    float u = (float)(state.uv & 0x3FFF);
    float v = (float)((state.uv >> 14) & 0x3FFF);

    GsVertex vertex;
    vertex.x = x;
    vertex.y = y;
    vertex.z = z;
    vertex.f = f;
    vertex.r = r;
    vertex.g = g;
    vertex.b = b;
    vertex.a = a;
    vertex.s = s;
    vertex.t = t;
    vertex.u = u;
    vertex.v = v;
    vertex.adc = adc;

    state.vertex_queue.push_back(vertex);

    if (kick) {
        uint32_t prim_type = 0;
        if ((state.prmodecont & 1) != 0) {
            prim_type = state.prim & 7;
        } else {
            prim_type = state.prmode & 7;
        }
        assemble_primitive(state, (PrimitiveType)prim_type);
    }
}

static void write_gs_register(GsState& state, uint32_t reg, uint64_t val) {
    switch (reg) {
        case 0x00: state.prim = val; break;
        case 0x01: state.rgbaq = val; break;
        case 0x02: state.st = val; break;
        case 0x03: state.uv = val; break;
        case 0x04: // XYZF2
            state.fog = (val >> 56) & 0xFF;
            handle_vertex_kick(state, val, true, true);
            break;
        case 0x05: // XYZ2
            handle_vertex_kick(state, val, true, false);
            break;
        case 0x06: state.tex0_1 = val; break;
        case 0x07: state.tex0_2 = val; break;
        case 0x08: state.clamp_1 = val; break;
        case 0x09: state.clamp_2 = val; break;
        case 0x0a: state.fog = val & 0xFF; break;
        case 0x0c: // XYZF3
            state.fog = (val >> 56) & 0xFF;
            handle_vertex_kick(state, val, false, true);
            break;
        case 0x0d: // XYZ3
            handle_vertex_kick(state, val, false, false);
            break;
        case 0x14: state.tex1_1 = val; break;
        case 0x15: state.tex1_2 = val; break;
        case 0x16: state.tex2_1 = val; break;
        case 0x17: state.tex2_2 = val; break;
        case 0x18: state.xyoffset_1 = val; break;
        case 0x19: state.xyoffset_2 = val; break;
        case 0x1a: state.prmodecont = val; break;
        case 0x1b: state.prmode = val; break;
        case 0x1c: state.texclut = val; break;
        case 0x22: state.scanmsk = val; break;
        case 0x34: state.miptbp1_1 = val; break;
        case 0x35: state.miptbp1_2 = val; break;
        case 0x36: state.miptbp2_1 = val; break;
        case 0x37: state.miptbp2_2 = val; break;
        case 0x3b: state.texa = val; break;
        case 0x3d: state.fogcol = val; break;
        case 0x3f: state.texflush = val; break;
        case 0x40: state.scissor_1 = val; break;
        case 0x41: state.scissor_2 = val; break;
        case 0x42: state.alpha_1 = val; break;
        case 0x43: state.alpha_2 = val; break;
        case 0x44: state.dimx = val; break;
        case 0x45: state.dthe = val; break;
        case 0x46: state.colclamp = val; break;
        case 0x47: state.test_1 = val; break;
        case 0x48: state.test_2 = val; break;
        case 0x49: state.pabe = val; break;
        case 0x4a: state.fba_1 = val; break;
        case 0x4b: state.fba_2 = val; break;
        case 0x4c: state.frame_1 = val; break;
        case 0x4d: state.frame_2 = val; break;
        case 0x4e: state.zbuf_1 = val; break;
        case 0x4f: state.zbuf_2 = val; break;
        case 0x50: state.bitbltbuf = val; break;
        case 0x51: state.trxpos = val; break;
        case 0x52: state.trxreg = val; break;
        case 0x53: state.trxdir = val; break;
        case 0x54: state.hwreg = val; break;
        case 0x60: state.signal = val; break;
        case 0x61: state.finish = val; break;
        case 0x62: state.label = val; break;
        default:
            break;
    }
}

uint32_t parse_dma_packet(u32 vaddr, GsState& state) {
    // 1. Read DMA tag at vaddr
    uint32_t dma_tag_w0 = ee_read<uint32_t>(vaddr + 0);
    uint32_t qwc = dma_tag_w0 & 0xFFFF;
    (void)qwc;
    
    uint32_t id = (dma_tag_w0 >> 28) & 0xF;
    if (id == 2 || dma_tag_w0 == 0x20000000) {
        return 0; // Terminate parsing on END/sentinel tags
    }
    
    // The GIFtag starts 16 bytes after the DmaTag
    u32 giftag_addr = vaddr + 16;
    
    uint32_t w0 = ee_read<uint32_t>(giftag_addr + 0);
    uint32_t w1 = ee_read<uint32_t>(giftag_addr + 4);
    uint32_t w2 = ee_read<uint32_t>(giftag_addr + 8);
    uint32_t w3 = ee_read<uint32_t>(giftag_addr + 12);

    uint32_t nloop = w0 & 0x7FFF;
    bool pre = (w1 & 0x4000) != 0;
    uint32_t prim_val = (w1 >> 15) & 0x7FF;
    uint32_t flg = (w1 >> 26) & 3;
    uint32_t nreg = (w1 >> 28) & 0xF;
    uint64_t regs = ((uint64_t)w3 << 32) | w2;

    uint32_t num_regs = (nreg == 0) ? 16 : nreg;



    if (pre) {
        state.prim = prim_val;
    }

    u32 curr_addr = giftag_addr + 16;

    if (flg == 0) { // PACKED
        for (uint32_t l = 0; l < nloop; ++l) {
            for (uint32_t r = 0; r < num_regs; ++r) {
                uint32_t reg_nibble = (regs >> (r * 4)) & 0xF;
                uint64_t val = ee_read<uint64_t>(curr_addr);
                uint64_t upper = ee_read<uint64_t>(curr_addr + 8);
                uint32_t target_reg = reg_nibble;
                if (reg_nibble == 0x0E) { // A_D
                    target_reg = upper & 0xFF;
                }

                if (target_reg != 0x0F) { // NOP
                    write_gs_register(state, target_reg, val);
                }
                curr_addr += 16;
            }
        }
    } else if (flg == 1) { // REGLIST
        uint32_t reg_index = 0;
        uint64_t current_qw_lower = 0;
        uint64_t current_qw_upper = 0;

        for (uint32_t l = 0; l < nloop; ++l) {
            for (uint32_t r = 0; r < num_regs; ++r) {
                uint32_t reg_nibble = (regs >> (r * 4)) & 0xF;
                uint64_t val = 0;

                if ((reg_index % 2) == 0) {
                    current_qw_lower = ee_read<uint64_t>(curr_addr);
                    current_qw_upper = ee_read<uint64_t>(curr_addr + 8);
                    val = current_qw_lower;
                } else {
                    val = current_qw_upper;
                    curr_addr += 16;
                }
                reg_index++;

                uint32_t target_reg = reg_nibble;
                if (reg_nibble == 0x0E) {
                    target_reg = 0x0F;
                }

                if (target_reg != 0x0F) {
                    write_gs_register(state, target_reg, val);
                }
            }
        }
        if ((reg_index % 2) != 0) {
            curr_addr += 16;
        }
    } else if (flg == 2 || flg == 3) { // IMAGE
        curr_addr += nloop * 16;
    }

    return curr_addr - vaddr;
}

} // namespace rac1
