// Veldin's remaining moby callbacks: the class-1545 update (the crash site's smoke in
// the opening cutscene's scenes 2..4), the class-1440 enemy's muzzle-flash billboard (a draw
// callback), the ship's update 0x28e180 (classes 0x213..0x215: engine glow, boarding prompt) and
// the ships' reflective canopy (draw callback 0x2935b8, classes 0x212..0x215), and the helpers
// they call.,
#ifndef RAC1_GAME_OVL_VELDIN_MOBYS_H
#define RAC1_GAME_OVL_VELDIN_MOBYS_H

#include <cstdint>

#include <runtime/kernel/vu0.h>  // rac1::Vec4

namespace rac1 {

void scene_moby_smoke(uint32_t m);  // orig: 0x002637f8
void smoke_burst_v(const Vec4& a, const Vec4& b, const Vec4& vel, float size_a, float size_b);  // orig: 0x00263b70
void smoke_burst(uint32_t a, uint32_t b, uint32_t vel, float size_a, float size_b);  // orig: 0x00263b70
void moby_1545_update(uint32_t m);  // orig: 0x002e3800
void e1440_flash_draw(uint32_t m);  // orig: 0x002e1c78
int32_t hud_message_release(int32_t who);  // orig: 0x002644e0
void movie_start(uint32_t sector, uint32_t size, uint32_t arg);  // orig: 0x002995d0
void movie_play(int32_t n);  // orig: 0x00299460
void ship_update(uint32_t m);  // orig: 0x0028e180
void ship_canopy_draw(uint32_t m);  // orig: 0x002935b8
void veldin_moby_fns_register();  // orig: (none) — the callbacks above for the indirect calls

}  // namespace rac1

#endif
