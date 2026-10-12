// Registration of the weapon ports for the indirect (moby class / function table) calls.
#include <game/ovl/fx1471.h>
#include <game/ovl/suck_cannon.h>
#include <game/ovl/novalis_hooks.h>
#include <game/ovl/ryno.h>
#include <game/ovl/metal.h>
#include <game/ovl/hydro.h>
#include <game/ovl/morph.h>
#include <game/ovl/decoy.h>
#include <game/ovl/tesla.h>
#include <game/ovl/visibomb.h>
#include <game/ovl/devastator.h>
#include <game/ovl/swingshot.h>
#include <game/ovl/grapple.h>
#include <game/ovl/rocket.h>
#include <game/ovl/dev_271450.h>
#include <game/ovl/visibomb_ball.h>
#include <game/ovl/tesla_ball.h>
#include <game/ovl/decoy_obj.h>
#include <game/ovl/metal_obj.h>
#include <game/ovl/fx_26a5b8.h>
#include <game/ovl/fx_26ee48.h>
#include <game/ovl/morph_fx.h>
#include <game/ovl/morph_draw.h>
#include <game/ovl/ryno_fx.h>
#include <game/ovl/decoy_fx.h>
#include <game/ovl/doomlet.h>
#include <game/ovl/weapons.h>

namespace rac1 {

// orig: (none) — one call for every weapon file of.
void weapons_fns_register() {
    mine_glove_fns_register();
    gadgets_fns_register();
    taunter_fns_register();
    doom_glove_fns_register();
    doom_agent_fns_register();
    suck_cannon_fns_register();
    novalis_hooks_fns_register();
    ryno_fns_register();
    metal_fns_register();
    hydro_fns_register();
    morph_fns_register();
    decoy_fns_register();
    tesla_fns_register();
    visibomb_fns_register();
    devastator_fns_register();
    swingshot_fns_register();
    grapple_fns_register();
    rocket_fns_register();
    dev_271450_fns_register();
    visibomb_ball_fns_register();
    tesla_ball_fns_register();
    decoy_obj_fns_register();
    metal_obj_fns_register();
    fx_26a5b8_fns_register();
    fx_26ee48_fns_register();
    morph_fx_fns_register();
    morph_draw_fns_register();
    ryno_fx_fns_register();
    fx1471_fns_register();
    decoy_fx_fns_register();
    doomlet_fns_register();
    walloper_fns_register();
    weapon_draw_fns_register();
}

}  // namespace rac1
