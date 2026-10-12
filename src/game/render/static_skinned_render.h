#ifndef RAC1_STATIC_SKINNED_RENDER_H
#define RAC1_STATIC_SKINNED_RENDER_H

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

namespace rac1 {

struct StaticSkinnedModel {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint textures[16] = {0};
    int joint_count = 0;
    std::vector<glm::mat4> inverse_bind_matrices;
    std::vector<int> joint_parent;
    int index_count = 0;
};

// The GS state a moby is drawn with by the game's moby pass 0x2559c0 (its ALPHA_1 / TEST_1 writes
// and the alpha of its colour block), for the GL draw. Inactive: the host's older draw state.
struct MobyGsState {
    bool active = false;
    float alpha = 1.0f;          // the vertex alpha: (fade * moby +0x23) >> 7, over 0x80
    bool additive = false;       // ALPHA_1 0x48 (Cs * As + Cd), moby flag 0x200; else 0x80_00000044
    float aref = 96.0f / 128.0f; // TEST_1 AREF (0x60, or the moby's flag 0x8 / fade bit), over 0x80
};

// Draws with `gs`: blended (normal or additive), the pixels passing the alpha test with depth
// writes, then the failing ones without (TEST_1 AFAIL RGB_ONLY). `draw` issues the draw calls.
template <typename Draw>
void gs_draw_passes(GLuint shader, const MobyGsState* gs, Draw draw) {
    const GLint mode = glGetUniformLocation(shader, "u_GsMode");
    if (gs == nullptr || !gs->active) {
        if (mode != -1) glUniform1i(mode, 0);
        draw();
        return;
    }
    glUniform1f(glGetUniformLocation(shader, "u_MobyAlpha"), gs->alpha);
    glUniform1f(glGetUniformLocation(shader, "u_AlphaRef"), gs->aref);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, gs->additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
    glUniform1i(mode, 1);
    glDepthMask(GL_TRUE);
    draw();
    glUniform1i(mode, 2);
    glDepthMask(GL_FALSE);
    draw();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUniform1i(mode, 0);
}

bool load_static_skinned_model(const std::string& glb_path, StaticSkinnedModel& out);

void render_static_skinned_model(const StaticSkinnedModel& m, const glm::mat4& model_xform,
                                 const glm::mat4& view_proj, const std::vector<glm::mat4>& joint_matrices,
                                 const MobyGsState* gs = nullptr);

} // namespace rac1

#endif // RAC1_STATIC_SKINNED_RENDER_H
