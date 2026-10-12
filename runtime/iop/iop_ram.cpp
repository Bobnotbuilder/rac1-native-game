#include <runtime/iop/iop_ram.h>

#include <vector>

namespace rac1 {

uint8_t* iop_ram() {
    static std::vector<uint8_t> ram(kIopRamSize, 0);
    return ram.data();
}

void iop_ram_reset() {
    uint8_t* r = iop_ram();
    for (uint32_t i = 0; i < kIopRamSize; ++i) r[i] = 0;
}

}  // namespace rac1
