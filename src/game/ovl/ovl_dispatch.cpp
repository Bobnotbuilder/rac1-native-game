// Indirect calls of the original (`jalr` through a code pointer held in game data, e.g. the
// moby update function at moby +0x74). The EE address is looked up in a table of ported
// functions; addresses without a port are reported once (TODO_PORT) and skipped.
#include <game/ovl/nanotech_draw.h>
#include <game/ovl/cutscene.h>
#include <game/ovl/effect_quads.h>
#include <game/ovl/glows.h>
#include <game/ovl/aridia_mobys.h>
#include <game/ovl/kerwan_mobys.h>
#include <game/ovl/eudora_mobys.h>
#include <game/ovl/rilgar_mobys.h>
#include <game/ovl/blarg_hooks.h>
#include <game/ovl/umbris_hooks.h>
#include <game/ovl/gaspar_hooks.h>
#include <game/ovl/batalia_hooks.h>
#include <game/ovl/orxon_hooks.h>
#include <game/ovl/pokitaru_hooks.h>
#include <game/ovl/veldin_orbit_hooks.h>
#include <game/ovl/veldin_finale_hooks.h>
#include <game/ovl/particle_types4_tr.h>
#include <game/ovl/novalis_mobys.h>
#include <game/ovl/novalis_level_fns.h>
#include <game/ovl/novalis_water.h>
#include <game/ovl/ovl.h>
#include <game/ovl/weapons.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/pause_menu.h>
#include <game/ovl/shared_classes_tr.h>
#include <game/ovl/veldin_mobys.h>
#include <game/ovl/ship_draw_cbs.h>
#include <game/ovl/veldin_extras.h>
#include <game/ovl/empty_class_updates.h>
#include <game/ovl/weapon_classes_tr.h>
#include <game/port_stubs.h>

#include <vector>

namespace rac1 {

namespace {
struct MobyFnEntry {
    uint32_t addr;
    MobyFn fn;
};
// A level's own (level-only) function: keyed by the level's address, used only on that level.
struct LevelMobyFnEntry {
    int level;
    uint32_t addr;
    MobyFn fn;
};
static std::vector<LevelMobyFnEntry>& level_moby_fns() {
    static std::vector<LevelMobyFnEntry> table;
    static bool registered = false;
    if (!registered) {
        registered = true;
        novalis_moby_fns_register();
        aridia_moby_fns_register();
        kerwan_moby_fns_register();
        eudora_moby_fns_register();
        rilgar_moby_fns_register();
        blarg_moby_fns_register();
        umbris_moby_fns_register();
        gaspar_moby_fns_register();
        batalia_moby_fns_register();
        orxon_moby_fns_register();
        pokitaru_moby_fns_register();
        veldin_orbit_moby_fns_register();
        veldin_finale_moby_fns_register();
        novalis_level_fns_register();
        novalis_water_fns_register();
    }
    return table;
}
static MobyFn level_moby_fn(uint32_t addr) {
    const int level = ovl_reloc_level();
    if (level == 0) return nullptr;
    for (const LevelMobyFnEntry& e : level_moby_fns()) {
        if (e.level == level && e.addr == addr) return e.fn;
    }
    return nullptr;
}
static std::vector<MobyFnEntry>& moby_fns() {
    static std::vector<MobyFnEntry> table;
    static bool registered = false;
    if (!registered) {
        registered = true;
        moby_class_fns_register();
        hud_element_fns_register();
        quick_select_fns_register();
        gadgebot_cmd_fns_register();
        bomb_glove_fns_register();
        pyrocitor_fns_register();
        blaster_fns_register();
        weapons_fns_register();
        particle_fns_register();
        effect_light_fns_register();
        glow_fns_register();
        effect_quad_fns_register();
        pause_menu_fns_register();
        talk_fns_register();
        veldin_moby_fns_register();
        nanotech_draw_fns_register();
        shared_classes_tr_register();
        shared_fx_tr_register();
        ship_draw_cbs_register();
        empty_class_updates_register();
        veldin_extras_register();
        weapon_classes_register();
        particle_types4_register();
    }
    return table;
}
}  // namespace

// orig: (none) — registers the port of the EE function at `addr` for indirect calls.
void ovl_register_moby_fn(uint32_t addr, MobyFn fn) {
    for (MobyFnEntry& e : moby_fns()) {
        if (e.addr == addr) {
            e.fn = fn;
            return;
        }
    }
    MobyFnEntry entry;
    entry.addr = addr;
    entry.fn = fn;
    moby_fns().push_back(entry);
}

// orig: (host) see novalis_mobys.h
void ovl_register_level_moby_fn(int level, uint32_t addr, MobyFn fn) {
    for (LevelMobyFnEntry& e : level_moby_fns()) {
        if (e.level == level && e.addr == addr) {
            e.fn = fn;
            return;
        }
    }
    LevelMobyFnEntry entry;
    entry.level = level;
    entry.addr = addr;
    entry.fn = fn;
    level_moby_fns().push_back(entry);
}

// orig: (none) — whether the EE function at `addr` has a registered port.
bool ovl_moby_fn_ported(uint32_t addr) {
    if (level_moby_fn(addr) != nullptr) return true;  // (the level's own function)
    const uint32_t v = ovl_code_to_veldin(addr);  // (the registry holds Veldin's addresses)
    for (const MobyFnEntry& e : moby_fns()) {
        if (e.addr == v) return true;
    }
    return false;
}

// orig: (none) — `jalr fn` with a0 = moby.
void ovl_call_moby_fn(uint32_t fn, uint32_t moby) {
    if (const MobyFn own = level_moby_fn(fn); own != nullptr) {  // (a function only the level has)
        own(moby);
        return;
    }
    const uint32_t v = ovl_code_to_veldin(fn);  // (the level's code address as Veldin's: its port)
    for (const MobyFnEntry& e : moby_fns()) {
        if (e.addr == v) {
            e.fn(moby);
            return;
        }
    }
    TODO_PORT(fn, "moby callback")(moby);
}

}  // namespace rac1
