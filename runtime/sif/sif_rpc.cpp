#include <runtime/sif/sif_rpc.h>

#include <runtime/iop/iop_ram.h>
#include <runtime/kernel/ee_memory.h>

#include <cstdio>
#include <map>

namespace rac1 {

namespace {
std::map<uint32_t, SifRpcServer>& servers() {
    static std::map<uint32_t, SifRpcServer> s;
    return s;
}
std::map<uint32_t, uint32_t>& clients() {  // client address -> server id
    static std::map<uint32_t, uint32_t> c;
    return c;
}
}  // namespace

void sif_rpc_register(uint32_t sid, SifRpcServer server) { servers()[sid] = std::move(server); }

void sif_rpc_reset() { clients().clear(); }

// orig: 0x0011ab20 (libkernel sceSifInitRpc; nothing to set up on the host)
void sif_init_rpc(uint32_t mode) { (void)mode; }

// orig: 0x0011aff8 (libkernel sceSifBindRpc; the IOP answers at once)
int32_t sif_bind_rpc(uint32_t client, uint32_t sid, uint32_t mode) {
    (void)mode;
    ee_write<uint32_t>(client + 0x10, 0);
    ee_write<uint32_t>(client + 0x24, 0);
    ee_write<uint32_t>(client + 0x00, 0);
    if (servers().count(sid) == 0) {
        std::fprintf(stderr, "[sif] no host IOP server 0x%x\n", sid);
        return 0;  // bound to nothing: the caller retries until serve is set
    }
    clients()[client] = sid;
    ee_write<uint32_t>(client + 0x24, sid);  // serve: non-zero once bound (the IOP's server data)
    return 0;
}

// orig: 0x0011b1c8 (libkernel sceSifCallRpc; the host server runs synchronously)
int32_t sif_call_rpc(uint32_t client, uint32_t fno, uint32_t mode, uint32_t send, uint32_t ssize, uint32_t recv, uint32_t rsize, uint32_t end,
                     uint32_t end_param) {
    (void)mode;
    ee_write<uint32_t>(client + 0x1c, end);
    ee_write<uint32_t>(client + 0x20, end_param);
    ee_write<uint32_t>(client + 0x00, 0);  // no packet in flight
    const auto c = clients().find(client);
    if (c == clients().end()) return -1;
    const auto s = servers().find(c->second);
    if (s == servers().end()) return -1;
    s->second(fno, send, ssize, recv, rsize);
    if (end != 0) std::fprintf(stderr, "[sif] end function 0x%08x not run (host)\n", end);
    return 0;
}

// orig: 0x0011b3b8 (libkernel sceSifCheckStatRpc)
int32_t sif_check_stat_rpc(uint32_t client) {
    (void)client;
    return 0;
}

// orig: 0x00118b20 (libkernel sceSifSetDma; the host copies at once)
uint32_t sif_set_dma(uint32_t list, int32_t count) {
    static uint32_t id = 0;
    for (int32_t i = 0; i < count; ++i) {
        const uint32_t t = list + static_cast<uint32_t>(i) * 16;
        const uint32_t src = ee_read<uint32_t>(t), dst = ee_read<uint32_t>(t + 4), n = ee_read<uint32_t>(t + 8);
        for (uint32_t k = 0; k < n; ++k)
            if (dst + k < kIopRamSize) iop_ram()[dst + k] = ee_read<uint8_t>(src + k);
    }
    return ++id;
}

}  // namespace rac1
