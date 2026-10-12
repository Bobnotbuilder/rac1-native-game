// The host renderer's mobys from the game's own moby list: the mobys 0x100 apart from
// *(0x15ffd8) up to the state 0xff that ends the list, as the moby draw loop 0x2559c0 walks them
// (state >= 0, the moby's zone bits +0x37 in the visible zones 0x173d00[+0x36], flags +0x34
// without 0x81; a zone from 0x80 (the hero's) reads past the 0x80-byte copy of the table on
// the scratchpad, into data the frame's other draw passes left there: the host draws it); its
// view culling and level of detail are the host GL's. The class models come from the host's level assets
// (keyed by the class number +0xa6); the skinned classes (the hero, class 0, and the level's classes in
// g_moby_glb_dir) are drawn with their models exported from the ISO and the joint matrices of
// the game's pose evaluation 0x254108; without one, the class model is drawn rigid. Host code: the PS2 draws
// these with the VU.
#include <game/render/orig_moby_draw.h>

#include <game/ovl/ovl_reloc.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/render/moby_render.h>
#include <game/render/render_view.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace rac1 {

namespace {

// The model matrix: the rows +0xc0 / +0xd0 / +0xe0 as the basis, scaled by the moby's scale
// +0x2c over the class scale +0x24 (the host's models are in class units), the position +0x10.
// orig: (host) the moby's model matrix
glm::mat4 moby_model_matrix(uint32_t m) {
    glm::mat4 mat(1.0f);
    const uint32_t cls = lwu(m + 0x24);
    const float cs = cls != 0 ? lwc1(cls + 0x24) : 0.0f;
    const float k = cs != 0.0f ? lwc1(m + 0x2c) / cs : 1.0f;
    for (int c = 0; c < 3; ++c) {
        for (int i = 0; i < 3; ++i) mat[c][i] = k * lwc1(m + 0xc0 + static_cast<uint32_t>(c) * 0x10 + static_cast<uint32_t>(i) * 4);
    }
    mat[3] = glm::vec4(lwc1(m + 0x10), lwc1(m + 0x14), lwc1(m + 0x18), 1.0f);
    return mat;
}

// The joint matrices of the game's pose evaluation 0x254108 (all joints of the class, on the
// scratchpad), the translations scaled from the class's units (+0x24 / 1024) to the model's.
// orig: (host) the joint matrices of 0x00254108 for the GL draw
bool moby_joint_matrices(uint32_t m, std::vector<glm::mat4>& out) {
    const uint32_t cls = lwu(m + 0x24);
    if (cls == 0) return false;
    const uint32_t n = lbu(cls + 8);
    if (n == 0 || n > 0x7f) return false;
    constexpr OvConst<0x00196e00u> MASK{};
    for (uint32_t o = 0; o < 0x80; o += 8) sd(MASK + o, 0);
    for (uint32_t j = 0; j + 1 < n; ++j) sb(MASK + j, 1);
    sb(MASK + n - 1, 0xff);
    sb(MASK + 0x7f, n);
    moby_pose_matrices_masked(m, MASK);
    const float k = lwc1(cls + 0x24) / 1024.0f;
    out.assign(n, glm::mat4(1.0f));
    for (uint32_t j = 0; j < n; ++j) {
        float f[16];
        std::memcpy(f, ee_ram_ptr(0x70000000u + j * 0x40), sizeof(f));
        f[12] *= k;
        f[13] *= k;
        f[14] *= k;
        f[3] = f[7] = f[11] = 0.0f;
        f[15] = 1.0f;
        std::memcpy(&out[j][0][0], f, sizeof(f));
    }
    return true;
}

// The GS state the moby pass 0x2559c0 draws a moby with. Its colour block (packet +0xb0) carries
// the alpha (min(fade, 0x80) * +0x23) >> 7, the fade being the distance fade-out (0x80 short of the
// moby's draw distance +0x132; the host keeps its own culling, so 0x80 here). With t6 =
// (flags +0x34 & 0x208) | (fade < 0x80 ? 8 : 0) non-zero, the moby's packets are wrapped in GS
// writes from the table 0x1c2ec0 (copied to the scratchpad 0x3f00): flag 0x200 sets ALPHA_1 to
// 0x48 (additive, Cs * As + Cd), otherwise TEST_1 gets AREF t6 (the default TEST_1 0x5360b:
// alpha test GEQUAL 0x60, AFAIL RGB_ONLY, depth test GEQUAL; the default ALPHA_1 0x80_00000044:
// (Cs - Cd) * As + Cd).
// orig: (host) the GS state of the moby draw loop 0x002559c0
MobyGsState moby_gs_state(uint32_t m) {
    MobyGsState gs;
    gs.active = true;
    constexpr uint32_t fade = 0x80;
    const uint32_t a = (fade * lbu(m + 0x23)) >> 7;
    gs.alpha = static_cast<float>(a) / 128.0f;
    const uint32_t t6 = (lhu(m + 0x34) & 0x208u) | (fade < 0x80 ? 8u : 0u);
    uint32_t aref = (lwu(OV(0x001c2ef0)) >> 4) & 0xffu;  // TEST_1 of the table: AREF 0x60
    if ((t6 & 0x200u) != 0) {
        gs.additive = true;
    } else if (t6 != 0) {
        aref = t6;  // the table's TEST_1 + t6 * 16 - 0x600
    }
    gs.aref = static_cast<float>(aref) / 128.0f;
    return gs;
}

}  // namespace

// orig: (host) the moby pass of the frame's drawing (the PS2's moby draw loop 0x2559c0)
void orig_moby_draw() {
    const uint32_t first = lwu(OV(0x0015ffd8));
    if (first == 0) return;
    std::vector<glm::mat4> joints;
    for (uint32_t m = first;; m += 0x100) {
        const int8_t state = static_cast<int8_t>(lbu(m + 0x20));
        if (state == -1) break;
        if (state < 0) continue;
        const uint32_t zone = lbu(m + 0x36);
        if (zone < 0x80 && (lbu(m + 0x37) & lbu(OV(0x00173d00) + zone)) == 0) continue;
        if ((lhu(m + 0x34) & 0x81u) != 0) continue;
        const int16_t cls = lh(m + 0xa6);
        const bool has_joints = moby_joint_matrices(m, joints);
        if (std::getenv("RAC1_MOBY_DEBUG") && m == first) {
            const glm::mat4 mm = moby_model_matrix(m);
            std::printf("[moby] hero joints %d n %zu j0 %.2f %.2f %.2f %.2f model %.2f %.2f %.2f / %.2f %.2f %.2f / %.2f %.2f %.2f t %.2f %.2f %.2f\n", has_joints, joints.size(),
                        joints[0][0][0], joints[0][1][1], joints[0][3][0], joints[0][3][2], mm[0][0], mm[0][1], mm[0][2], mm[1][0], mm[1][1], mm[1][2], mm[2][0], mm[2][1], mm[2][2], mm[3][0], mm[3][1], mm[3][2]);
        }
        if (!has_joints) joints.assign(1, glm::mat4(1.0f));
        const MobyGsState gs = moby_gs_state(m);
        moby_draw_class(cls, moby_model_matrix(m), joints, has_joints, &gs);
    }
}

}  // namespace rac1
