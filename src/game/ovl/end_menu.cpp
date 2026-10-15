// The widget functions of the end-of-game menu's pages (end_menu_tr.cpp): the dispatch from menu_widget_call (see end_menu_tr.h).
#include <game/ovl/end_menu_tr.h>

namespace rac1 {

// orig: (host) see end_menu_tr.h
bool end_menu_widget_call(uint32_t veldin, uint32_t w, int32_t a1, int32_t& r) {
    const uint32_t a = static_cast<uint32_t>(a1);
    switch (veldin) {
        case 0x00283530: r = static_cast<int32_t>(end_menu_panel_update_tr(w, a)); return true;
        case 0x002838b0: r = static_cast<int32_t>(end_menu_panel_draw_tr(w, a)); return true;
        case 0x002833d0: r = static_cast<int32_t>(end_menu_panel_focus_in_tr(w, a)); return true;
        case 0x002834a8: r = static_cast<int32_t>(end_menu_panel_focus_out_tr(w, a)); return true;
        case 0x0027fed8: r = static_cast<int32_t>(end_menu_choice_update_tr(w, a)); return true;
        case 0x002806e0: r = static_cast<int32_t>(end_menu_choice_draw_tr(w, a)); return true;
        case 0x00280038: r = static_cast<int32_t>(end_menu_page23_update_tr(w, a)); return true;
        case 0x002803b0: r = static_cast<int32_t>(end_menu_page23_draw_tr(w, a)); return true;
        case 0x00280020: r = static_cast<int32_t>(end_menu_page23_focus_in_tr(w, a)); return true;
        default: return false;
    }
}

}  // namespace rac1
