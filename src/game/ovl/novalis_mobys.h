// Novalis's (level 1) own moby classes: the update / draw functions of the classes only
// Novalis registers, ported at Novalis's own addresses
// (`// orig: L1:0x00xxxxxx`). They are registered per level (ovl_register_level_moby_fn): the
// indirect call of the original (`jalr` through moby +0x74) reaches them only while the played
// level's overlay is Novalis's own (RAC1_OWN_OVERLAY=1 RAC1_LEVEL=1), keyed by Novalis's address.
#ifndef RAC1_GAME_OVL_NOVALIS_MOBYS_H
#define RAC1_GAME_OVL_NOVALIS_MOBYS_H

#include <cstdint>

#include <game/ovl/ovl.h>
#include <game/ovl/variants_l1.h>
#include <game/ovl/variants_l2.h>

namespace rac1 {

// orig: (host) registers the port of the function at `addr` of level `level`'s own overlay for the
// indirect calls (ovl_call_moby_fn); consulted first, only while that level is the translation
// target (ovl_reloc_level() == level)
void ovl_register_level_moby_fn(int level, uint32_t addr, MobyFn fn);

// ---- the landing pad and what is near it (novalis_mobys.cpp)
void cave_door_update_l1(uint32_t m);    // L1:0x002f4348  class 641
void spawn_point_update_l1(uint32_t m);  // L1:0x00300220  class 805

void scripted_camera_nodes_l1();                       // L1:0x002fb4b8
void scripted_camera_trigger_update_l1(uint32_t m);   // L1:0x002fb5b0  class 737
void plant_bolts_drop_l1(uint32_t m);                 // L1:0x002787a0
void moby_clone_l1(uint32_t m, uint32_t cls);         // L1:0x00278e20
void tall_shroom_update_l1(uint32_t m);               // L1:0x0030d0f0  class 1813
void mushroom_underside_update_l1(uint32_t m);        // L1:0x00307c48  class 1042
void green_mushroom_update_l1(uint32_t m);            // L1:0x002fd9a0  class 754

// ---- the platforms, doors and lifts (novalis_platforms.cpp)
void moby_sound_release_l1(uint32_t m, int32_t slot);  // (helper) the inlined channel stop
int32_t moby_hero_on_l1(uint32_t m);                   // L1:0x00277fb8
void lift_update_l1(uint32_t m);                       // L1:0x002b9eb0  class 726

void door_half_update_l1(uint32_t m);                  // L1:0x002f4710  class 665
void door_update_l1(uint32_t m);                       // L1:0x002fed68  classes 768 / 769
void piston_platform_update_l1(uint32_t m);            // L1:0x002f95c0  classes 703 / 715
void fan_update_l1(uint32_t m);                        // L1:0x002f9bf0  class 705
void bridge_half_update_l1(uint32_t m);                // L1:0x002fb8a8  class 746

// ---- the enemies (novalis_enemies.cpp)
void moby_group_set_bc_l1(uint32_t idx, uint32_t value);              // L1:0x0026e090
void walker_record_init_l1(uint32_t rec);                             // L1:0x0026d930
int32_t cooldown_slot_claim_l1(uint32_t arr, int32_t n, int32_t delay);  // L1:0x002efbf8
int32_t path_next_point_l1(uint32_t m, uint32_t path, uint32_t target, uint32_t out);  // L1:0x0028b5e0
void peckbot_update_l1(uint32_t m);                                   // L1:0x002efc60  class 577

int32_t moby_state_is_1(uint32_t m);                  // orig: 0x002d8128 (Novalis 0x00300518)
int64_t moby_on_screen_l1(uint32_t m, float r);       // L1:0x00275690
void help_controller_update_l1(uint32_t m);           // L1:0x0030acb8  class 1341

// ---- the NPCs (novalis_npcs.cpp)
void moby_data_note_l1(uint32_t at, int32_t size, uint32_t m, int32_t kind, uint32_t list);  // L1:0x0029b0a0
void plumber_camera_nodes_l1(uint32_t m);                           // L1:0x002ff028
void hero_walk_to_l1(float r, uint32_t m);                          // L1:0x002783a8
void plumber_update_l1(uint32_t m);                                 // L1:0x002ff118  class 774

// ---- props (novalis_props.cpp)
float lily_spring_l1(float target, float k, float damp, float r, uint32_t pos, uint32_t vel);  // L1:0x002f81c8
void water_lily_update_l1(uint32_t m);                              // L1:0x002f8268  class 695

void zone_marker_update_l1(uint32_t m);                             // L1:0x002f2eb8  class 605
void zone_marker_update_l2(uint32_t m);                             // L2:0x002d74e0  class 605 (Aridia: counter 0x161a60)

void empty_update_2ece90_l1(uint32_t m);                            // L1:0x002ece90  class 531
void empty_update_30d200_l1(uint32_t m);                            // L1:0x0030d200  class 1816
void draw_hook_676_update_l1(uint32_t m);                           // L1:0x002f6128  class 676
void draw_hook_678_update_l1(uint32_t m);                           // L1:0x002f6180  class 678
void draw_hook_1225_update_l1(uint32_t m);                          // L1:0x00309c98  class 1225

// orig: (none) -- Novalis's registrations
void novalis_moby_fns_register();

}  // namespace rac1

#endif
