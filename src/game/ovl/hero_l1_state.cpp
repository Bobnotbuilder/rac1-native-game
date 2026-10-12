// Novalis's (level 1) variants of the hero's mode / state helpers. Addresses are
// Novalis's; the code is Veldin's minus the features Novalis's build does not have.
#include <game/ovl/hero_l1.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>

namespace rac1 {

namespace {
constexpr uint32_t P = g::PLAYER;
}  // namespace

// Enter the special movement mode `mode` (Veldin's 0x210a08), without the mode 2 setup (+0x1630 = 200):
// the held moby released, the slots updated, the hero moby's children retired and its effects reset,
// `moby` becomes the hero moby (+0x2080) and the ridden one (+0xa84, flags 6), +0x22cc the entry state;
// mode 1 swaps the counter +0x22a8 with +0x22ac (saving it to +0x22b0).
// orig: L1:0x00231348 L2:0x0021fd38
void hero_mode_enter_l1(uint32_t mode, uint32_t state, uint32_t moby) {
    hero_hold_release();
    sb(P + 0x20a6, 1);
    hero_slots_update(P);
    sw(P + 0x2080, lwu(P + 0xa88));
    retire_children(P + 0x1670);
    sw(lwu(P + 0x2080) + 0x98, 0xffffffffu);
    sub_00232fe8();
    sub_00206c08();
    sub_00205538();
    sw(P + 0x2080, moby);
    sb(P + 0x20a4, mode);
    sw(P + 0xa84, moby);
    hero_gait_update();
    swc1(P + 0x224, lwc1(P + 0x22c));
    swc1(P + 0x220, lwc1(P + 0x228));
    const uint32_t hm = lwu(P + 0x2080);
    sh(hm + 0x34, lhu(hm + 0x34) | 6u);
    swc1(P + 0xa94, 1.0f);
    swc1(P + 0xa90, 1.0f);
    sub_00232628();
    moby_update_light(lwu(P + 0x2080));
    sub_00207a58();
    sh(P + 0x22cc, state);
    if (mode == 1) {
        sh(P + 0x22b0, lhu(P + 0x22a8));
        sw(P + 0x22a8, lwu(P + 0x22ac));
    }
    hero_set_state(state, 1);
}

// Leave the special movement mode (Veldin's 0x210b30), without the mode 1 / mode 2 cleanups: effects
// reset, the mode cleared, the ridden moby +0xa84 loses flags 1/2, the extra moby +0xa8c is retired,
// the hero moby +0xa88 is restored at the hero's position (+0xa94 = 1) and the hero re-enters state 0
// (not on levels 2 / 6 in state 0x64).
// orig: L1:0x00231450 L2:0x0021fe40
void hero_mode_exit_l1() {
    sub_00205538();
    sb(P + 0x20a4, 0);
    const uint32_t ride = lwu(P + 0xa84);
    if (ride != 0) sh(ride + 0x34, lhu(ride + 0x34) & 0xfff9u);
    sw(P + 0xa84, 0);
    if (lwu(P + 0xa8c) != 0) {
        moby_retire(lwu(P + 0xa8c));
        sw(P + 0xa8c, 0);
    }
    sub_00206c08();
    const uint32_t hm = lwu(P + 0xa88);
    sw(P + 0x2080, hm);
    V(hm + 0x10) = V(P + 0x80);
    sw(hm + 0x98, 0);
    swc1(P + 0xa94, 1.0f);
    sub_00232628();
    if (lw(P + 0x2084) == 0x64) {
        const int32_t lvl = lw(OV(0x0015f5c4));
        if (lvl == 2 || lvl == 6) return;
    }
    hero_set_state(0, 1);
}

}  // namespace rac1
