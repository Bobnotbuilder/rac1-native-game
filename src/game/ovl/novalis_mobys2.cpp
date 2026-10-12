// Novalis's own moby classes, part 2: the registrations. The ports are in novalis_*.cpp.
#include <game/ovl/novalis_mobys2.h>
#include <game/ovl/variants_l1.h>

namespace rac1 {

// orig: (none) -- Novalis's registrations, part 2
void novalis_moby_fns_register2() {
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x003021b8), beehive_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002e1ac0), gold_case_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f4428), jet_fighter_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f7728), jet_bomber_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f4960), jet_ship_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002edca0), amoeboid_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002e6bf0), birdbot_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x00308bd8), teleporter_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f8c58), nov_class700_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f6328), nov_class679_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002bd100), nov_class27_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f9080), nov_class701_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f9810), nov_class704_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002feb58), nov_class761_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002fa800), nov_class729_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002fad68), nov_class730_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x0030b618), nov_class1504_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f3120), nov_class613_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f9d80), nov_class709_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x0030f208), nov_class1848_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f2b68), nov_class604_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002ff860), nov_class778_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x00302648), nov_class832_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002eda00), nov_class567_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002fa4c0), nov_class722_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002ffb28), nov_class779_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002ffdc0), nov_class787_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002c7530), nov_class156_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x0030c190), nov_class1546_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f6a30), nov_class686_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002f8718), nov_class696_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x0030ba18), nov_class1510_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x0030c9a8), nov_class1633_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002fd0e8), nov_class751_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x0030df40), nov_class1818_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002e3f20), nov_class437_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x002e0c68), nov_class280_update_l1);
    ovl_register_level_moby_fn(NOVALIS, L1_ADDR(0x00307ca0), nov_class1134_update_l1);
}

}  // namespace rac1
