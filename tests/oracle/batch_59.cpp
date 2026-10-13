// Oracle batch 59.

#include "ovl_oracle.h"

#include <game/ovl/ovl_reloc.h>
namespace rac1::oracle {

void batch_59() {
    if (g_only_batch != 59) return;
    auto boot = [] { veldin_boot(); };
    g_batch = 59;
    g_ignored_words = vcross_lanes();
    check("level_setup_body", 0x00244110, 1, Ret::Int32, [boot] { boot(); return Args{{1, 0}, {}}; }, [](A a) {
        level_setup_body(static_cast<int32_t>(a.i[0]), static_cast<int32_t>(a.i[1]));
        w32(OV(0x0015f5cc), ee_read<uint32_t>(OV(0x0015f5cc)) + 1);
        return uint64_t{0};
    });
}

}  // namespace rac1::oracle
