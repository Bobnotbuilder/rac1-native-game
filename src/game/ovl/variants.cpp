// See variants.h.
#include <game/ovl/variants.h>
#include <game/ovl/variants_l1.h>
#include <game/ovl/variants_l2.h>
#include <game/ovl/variants_l3.h>
#include <game/ovl/variants_l4.h>
#include <game/ovl/variants_l5.h>
#include <game/ovl/variants_l6.h>
#include <game/ovl/variants_l7.h>
#include <game/ovl/variants_l9.h>
#include <game/ovl/variants_l8.h>
#include <game/ovl/variants_l10.h>
#include <game/ovl/variants_l11.h>
#include <game/ovl/variants_l12.h>

namespace rac1 {

// orig: (host) see variants.h
const VariantRow* ovl_variants(int level, size_t* count) {
    switch (level) {
    case NOVALIS: return ovl_variants_l1(count);
    case ARIDIA: return ovl_variants_l2(count);
    case KERWAN: return ovl_variants_l3(count);
    case EUDORA: return ovl_variants_l4(count);
    case RILGAR: return ovl_variants_l5(count);
    case BLARG: return ovl_variants_l6(count);
    case UMBRIS: return ovl_variants_l7(count);
    case GASPAR: return ovl_variants_l9(count);
    case BATALIA: return ovl_variants_l8(count);
    case ORXON: return ovl_variants_l10(count);
    case POKITARU: return ovl_variants_l11(count);
    case HOVEN: return ovl_variants_l12(count);
    default: break;
    }
    *count = 0;
    return nullptr;
}

}  // namespace rac1
