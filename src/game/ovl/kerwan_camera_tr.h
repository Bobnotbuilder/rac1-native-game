// Kerwan (level 3) camera types 1 and 14 (accept / init / update / exit), type 18's accept, the follow camera's hook 0x2eb408 and the callback it queues 0x2eb468, transliterated: its own camera code, which was TODO_PORT.
#ifndef RAC1_GAME_OVL_KERWAN_CAMERA_TR_H
#define RAC1_GAME_OVL_KERWAN_CAMERA_TR_H

#include <cstdint>

#include <game/ovl/novalis_mobys2.h>

namespace rac1 {

TrRet ker_cam_type1_accept_u(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002e8870
TrRet lv3_2e8628_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002e8628
void ker_cam_type1_init_l3(uint32_t a0);  // L3:0x002e8940
TrRet lv3_24e6f8_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x0024e6f8
TrRet lv3_2e8730_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002e8730
void ker_cam_type1_update_l3(uint32_t a0);  // L3:0x002e8960
void ker_cam_type1_exit_l3(uint32_t a0);  // L3:0x002e8980
TrRet ker_cam_type14_accept_u(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002ebd70
TrRet lv3_2eb7e0_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002eb7e0
void ker_cam_type14_init_l3(uint32_t a0);  // L3:0x002eb978
TrRet lv3_2ebaf0_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002ebaf0
TrRet lv3_2ebb00_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002ebb00
TrRet lv3_2ebc58_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002ebc58
void ker_cam_type14_update_l3(uint32_t a0);  // L3:0x002ebdb0
void ker_cam_type14_exit_l3(uint32_t a0);  // L3:0x002ebde8
TrRet lv3_2e6f58_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002e6f58
TrRet lv3_2e6f80_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002e6f80
TrRet lv3_2ec9a8_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002ec9a8
TrRet lv3_2ecae8_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002ecae8
TrRet lv3_2ecd28_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002ecd28
TrRet ker_cam_type18_accept_u(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x002ed5f8
TrRet lv3_1e4a88_l3(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19);  // L3:0x001e4a88
void ker_cam0_hook_l3(uint32_t a0);  // L3:0x002eb408
void ker_cam_callback_l3(uint32_t a0);  // L3:0x002eb468

}  // namespace rac1

#endif
