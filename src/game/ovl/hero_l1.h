// Novalis's (level 1) variants of the hero controller functions: see variants_l1.h. Each one is
// the function at Novalis's own address; the Veldin function of the same name dispatches to it
// while the translation target is Novalis's overlay (`on_novalis()`).
#ifndef RAC1_GAME_OVL_HERO_L1_H
#define RAC1_GAME_OVL_HERO_L1_H

#include <cstdint>

#include <game/math/vecmath.h>
#include <game/ovl/variants_l1.h>
#include <game/ovl/variants_l2.h>
#include <game/ovl/variants_l3.h>
#include <game/ovl/rilgar_hero.h>
#include <game/ovl/variants_l5.h>
#include <game/ovl/blarg_call.h>
#include <game/ovl/batalia_call.h>
#include <game/ovl/pokitaru_call.h>
#include <game/ovl/hoven_call.h>
#include <game/ovl/variants_l6.h>

namespace rac1 {

void hero_mobys_set_flag1_l1();    // L1:0x002488e8 (Veldin 0x233210)
void hero_mobys_clear_flag1_l1();  // L1:0x00248900 (Veldin 0x233248)

uint32_t sub_00205000_l1(int32_t flag);                        // L1:0x00226f10 (Veldin 0x205000)
void hero_footstep_l1(int32_t a, uint32_t foot, int32_t sound);  // L1:0x00227e48 (Veldin 0x205f60)
void sub_002076a8_l1();                                        // L1:0x00228e38 (Veldin 0x2076a8)
void hero_frame_update_l1();                                   // L1:0x00228870 (Veldin 0x2070d0)

void hero_surface_update_l1();  // L1:0x0022cd48 (Veldin 0x20b960)

int32_t hero_ledge_check_v_l1(const Vec4& pos, const float* ref_yaw);  // L1:0x0022d838 (Veldin 0x20c758)

void hero_ledge_grab_check_l1();  // L1:0x0022d090 (Veldin 0x20bed0)

void hero_mode_enter_l1(uint32_t mode, uint32_t state, uint32_t moby);  // L1:0x00231348 (Veldin 0x210a08)

void hero_mode_exit_l1();  // L1:0x00231450 (Veldin 0x210b30)

int32_t hero_hurt_l1(int32_t knock);  // L1:0x00231580 (Veldin 0x210ce8)

void hero_gait_update_l1();  // L1:0x00231ae0 (Veldin 0x211380)

void hero_stick_speed_l1();  // L1:0x00232290 (Veldin 0x211be8)

void hero_wall_slide_l1(int32_t facing);  // L1:0x00232978 (Veldin 0x212318)

void hero_ground_update_l1();  // L1:0x00232dc0 (Veldin 0x2127b8)

int32_t hero_collide_push_l1(int32_t mode);  // L1:0x00233940 (Veldin 0x2133a8)

void hero_move_integrate_l1();  // L1:0x00233de0 (Veldin 0x213880)

void sub_00213e68_l1(float dt);  // L1:0x00234358 (Veldin 0x213e68)

void hero_horizontal_velocity_l1();  // L1:0x00234b40 (Veldin 0x214658)

void hero_timers_update_l1();  // L1:0x0023c710 (Veldin 0x2215c8)

void hero_enter_idle_l1();  // L1:0x002405a0 (Veldin 0x227638)

void hero_land_l1();  // L1:0x00242198 (Veldin 0x2293a8)

int32_t hero_item_use_l1();  // L1:0x00240ed8 L2:0x0022ee00 (Veldin 0x227fa0)
void hero_item_swingshot(uint32_t mask);  // (helper) Veldin's item_swingshot (hero_input.cpp), Aridia's build's case 0xc

void hero_grind_update_v();  // (helper) Veldin's state 0x74 update block (hero_state_update.cpp), Kerwan's L3:0x00213874
void hero_state_update_l1();  // L1:0x002370b8 (Veldin 0x217970)

int32_t hero_set_state_l1(uint32_t state, int32_t mode);  // L1:0x0023cf98 (Veldin 0x2223f8)

void hero_input_update_l1();  // L1:0x00242930 (Veldin 0x229b70)

}  // namespace rac1

#endif
