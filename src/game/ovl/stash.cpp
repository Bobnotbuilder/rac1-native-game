// Ported overlay code: the EE side of the IOP texture stash (the IOP_stash_daemon, server 0x11,
// hosted by runtime/iop/stash_iop). 0x1c12a0 the IOP buffer, +4 its size, +0x30 the next free
// IOP address, +0x34 the entries; the 0x40 entries at 0x1c12d8 (16 bytes): +0 the IOP address,
// +4 the size (qwords), +8 the EE address the data belongs to.
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/ovl_common.h>
#include <runtime/sif/sif_rpc.h>

namespace rac1 {

namespace {
constexpr OvConst<0x001c12a0u> kStash{};
constexpr OvConst<0x001c12a8u> kClient{};
constexpr OvConst<0x001c12d8u> kEntries{};
}  // namespace

// The stash daemon bound (retried until it answers) and asked for its buffer; the entries
// emptied.
// orig: 0x00293b18
void stash_init() {
    for (;;) {
        while (sif_bind_rpc(kClient, 0x11, 1) < 0) {
        }
        while (sif_check_stat_rpc(kClient) != 0) {
        }
        if (lw(kClient + 0x24) != 0) break;
        // the console waits 0xffff loops before binding again
    }
    const uint32_t r = ee_stack_alloc(0x20);
    sif_call_rpc(kClient, 2, 0, 0, 0, r, 0x10, 0, 0);
    const uint32_t base = lwu(r), size = lwu(r + 4);
    ee_stack_free(0x20);
    sw(kStash, base);
    sw(kStash + 4, size);
    sw(kStash + 0x30, base);
    sw(kStash + 0x34, 0);
    for (uint32_t i = 0; i < 0x40; ++i) {
        sw(kEntries + i * 16, 0);
        sw(kEntries + i * 16 + 4, 0);
    }
}

// `qwc` quadwords at `src` sent into the stash (`n` quadwords of them by DMA) for the EE address
// `dest`: the entry, -1 when the buffer is full, -2 when the entries are.
// orig: 0x00293c58
int32_t stash_put(uint32_t src, int32_t n, int32_t qwc, uint32_t dest) {
    const uint32_t cur = lwu(kStash + 0x30);
    if (static_cast<int32_t>(lwu(kStash + 4) - (cur - lwu(kStash))) < qwc * 16) return -1;
    const int32_t count = lw(kStash + 0x34);
    if (count == 0x40) return -2;
    const uint32_t t = ee_stack_alloc(0x10);
    sw(t, src);
    sw(t + 4, cur);
    sw(t + 8, static_cast<uint32_t>(n) << 4);
    sw(t + 0xc, 0);
    sif_set_dma(t, 1);
    ee_stack_free(0x10);
    const uint32_t e = kEntries + static_cast<uint32_t>(count) * 16;
    sw(e, cur);
    sw(e + 4, static_cast<uint32_t>(qwc));
    sw(e + 8, dest);
    sw(kStash + 0x30, cur + static_cast<uint32_t>(qwc) * 16);
    sw(kStash + 0x34, static_cast<uint32_t>(count + 1));
    return count;
}

// `qwc` quadwords at `src` written over entry `idx` from quadword `off`: 0, -1 past its end, -3
// for no entry.
// orig: 0x00293d38
int32_t stash_write(uint32_t src, uint32_t idx, int32_t off, int32_t qwc) {
    if (idx >= 0x40 || lw(kEntries + idx * 16 + 4) == 0) return -3;
    if (lw(kEntries + idx * 16 + 4) < off + qwc) return -1;
    const uint32_t t = ee_stack_alloc(0x10);
    sw(t, src);
    sw(t + 4, lwu(kEntries + idx * 16) + static_cast<uint32_t>(off) * 16);
    sw(t + 8, static_cast<uint32_t>(qwc) << 4);
    sw(t + 0xc, 0);
    sif_set_dma(t, 1);
    ee_stack_free(0x10);
    return 0;
}

// The size (qwords) of entry `idx`; -3 past the table.
// orig: 0x00293ed0
int32_t stash_size(uint32_t idx) {
    if (idx >= 0x40) return -3;
    return lw(kEntries + idx * 16 + 4);
}

// `qwc` quadwords (-1: all) of entry `idx` from quadword `off` read back to `dst` (RPC 1, in
// pieces of up to 0xffff quadwords, RPC mode `mode`): 0, -1 past its end, -3 for no entry.
// orig: 0x00293db8
int32_t stash_read(uint32_t dst, uint32_t idx, int32_t off, int32_t qwc, uint32_t mode) {
    if (qwc == -1) qwc = stash_size(idx);
    if (idx >= 0x40 || lw(kEntries + idx * 16 + 4) == 0) return -3;
    if (lw(kEntries + idx * 16 + 4) < off + qwc) return -1;
    const uint32_t t = ee_stack_alloc(0x10);
    sw(t, lwu(kEntries + idx * 16) + static_cast<uint32_t>(off) * 16);
    while (qwc != 0) {
        const int32_t n = qwc < 0x10000 ? qwc : 0xffff;
        sif_call_rpc(kClient, 1, mode, t, 0x10, dst, static_cast<uint32_t>(n) * 16, 0, 0);
        dst += static_cast<uint32_t>(n) * 16;
        sw(t, lwu(t) + static_cast<uint32_t>(n) * 16);
        qwc -= n;
    }
    ee_stack_free(0x10);
    return 0;
}

}  // namespace rac1
