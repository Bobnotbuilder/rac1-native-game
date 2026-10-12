#ifndef RAC1_GAME_MOBY_RENDER_H
#define RAC1_GAME_MOBY_RENDER_H

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <game/render/static_skinned_render.h>

namespace rac1 {

// Where the skinned class models of the level are (setup.sh); empty: none.
extern std::string g_moby_glb_dir;
// The class models loaded so far released (before another level's are drawn).
void moby_models_release();

// The GL draw of one moby class model (model matrix, joint matrices, the GS state of the game's
// moby pass when given).
// orig: (host) the GL draw of a moby model
void moby_draw_class(int16_t cls, const glm::mat4& matrix, const std::vector<glm::mat4>& joint_matrices, bool has_joints,
                     const MobyGsState* gs = nullptr);

} // namespace rac1

#endif // RAC1_GAME_MOBY_RENDER_H
