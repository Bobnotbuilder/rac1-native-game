#pragma once
// _devtools/item_give_quiet.h — the debug / testing item give (not a port).
//
// The debug menu's item give and RAC1_GIVE_ITEMS call the game's own item_give (0x260860), which shows the item's
// acquisition message (table 0x1b0340) through the HUD message 0x263d40 (text 0x179118, frames 0x15f640). A first
// acquisition message is only in the text table of the level the game gives that item on (e.g. the
// Hydrodisplacer's on Nebula G34, the Swingshot's on Kerwan), so an item given anywhere else shows the game's own
// the placeholder message for a missing text (0x1796a8): the original does the same. The tools take that message back: the
// HUD message is left as it was when the level's table lacks the item's text. The item, its ammo and its slots are the
// game's own give.

#include <game/ovl/ovl.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/ovl_common.h>

#include <cstdint>

namespace rac1::devtools {

// orig: (none) — debug / testing: item_give(id, equip) without a message the level's text table lacks
inline void item_give_quiet(int32_t id, int32_t equip) {
    const uint32_t text = OV(0x00179118), timer = OV(0x0015f640);
    const uint32_t owned = 0x0013d4c0 + static_cast<uint32_t>(id);
    const int32_t msg = lw(OV(0x001b0340) + static_cast<uint32_t>(lbu(owned) == 0 ? id : id + 0x25) * 4);
    uint8_t saved[0x50];
    for (uint32_t i = 0; i < 0x50; ++i) saved[i] = static_cast<uint8_t>(lbu(text + i));
    const uint32_t saved_timer = lwu(timer);
    item_give(id, equip);
    if (msg != -1 && sub_00203a68(msg) < 0) {
        for (uint32_t i = 0; i < 0x50; ++i) sb(text + i, saved[i]);
        sw(timer, saved_timer);
    }
}

}  // namespace rac1::devtools
