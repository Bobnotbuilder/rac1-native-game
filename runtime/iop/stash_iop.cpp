#include <runtime/iop/stash_iop.h>

#include <runtime/iop/iop_ram.h>
#include <runtime/kernel/ee_memory.h>
#include <runtime/sif/sif_rpc.h>

#include <cstring>

namespace rac1 {

// The daemon's RPC function (irx.bin IOP_stash_daemon +0x1ec): the reply comes from IOP RAM.
void stash_iop_start() {
    sif_rpc_register(0x11, [](uint32_t fno, uint32_t send, uint32_t ssize, uint32_t recv, uint32_t rsize) {
        (void)ssize;
        if (recv == 0 || rsize == 0) return;
        if (fno == 2) {
            // the buffer's two words are written into the RPC's receive buffer, which is answered
            const uint32_t w[4] = {kStashBuffer, 0x00094000, 0, 0};
            for (uint32_t i = 0; i < rsize && i < 16; i += 4) ee_write<uint32_t>(recv + i, w[i / 4]);
            return;
        }
        if (fno == 1) {
            const uint32_t a = ee_read<uint32_t>(send);
            for (uint32_t i = 0; i < rsize; ++i) ee_write<uint8_t>(recv + i, a + i < kIopRamSize ? iop_ram()[a + i] : 0);
            return;
        }
        for (uint32_t i = 0; i < rsize; ++i) ee_write<uint8_t>(recv + i, ee_read<uint8_t>(send + i));
    });
}

}  // namespace rac1
