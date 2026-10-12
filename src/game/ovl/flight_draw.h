// The loading screen's drawing: game mode 6 step 4, the ship's flight through space between two planets
// (the level exit's loops run it every frame, 0x292e08 through 0x291d00). The same function in every level's overlay and
// in the boot module (front 0x22f288: the flight to Veldin after New Game). Was once misnamed "hero_update" (it is not the hero's update).
#ifndef RAC1_GAME_OVL_FLIGHT_DRAW_H
#define RAC1_GAME_OVL_FLIGHT_DRAW_H

#include <cstdint>

namespace rac1 {

// The flight's frame: the view, the sky (its field of view at least 0.63), the planet's picture, the mobys (the ship, its
// riders), the ship's trails and canopy, the planet's name, the memory card's icon and spinner while a save runs, the fade.
void flight_draw();             // orig: 0x002900a0
// The flight's sky: the sky's shells (the transition's sky), each turned (pitch / yaw from the camera's 0x1604c4) and scaled 1 .. 2 about the
// camera, through the sky's own shell routine.
void flight_sky();              // orig: 0x0028b440
// The next planet's picture: a textured quad (the picture's TEX0 0x160640) about the point 0x1605b0, sized per planet.
void flight_planet_picture();   // orig: 0x0028f6e0
// The next planet's name (TEX0 0x160648, the picture loading_screen_setup uploaded in the language 0x15ed88; 256 x 32) at alpha `a` (0..0x80).
void flight_banner(int32_t a);  // orig: 0x0028f820

}  // namespace rac1

#endif
