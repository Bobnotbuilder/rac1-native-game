// The EE side of the SIF remote procedure calls (libkernel sceSifBindRpc / sceSifCallRpc /
// sceSifCheckStatRpc). The IOP is not emulated: its modules (989snd's command and loader
// servers, the game's data server) are host code registered here by server id, and a call
// runs its server synchronously, so the EE always finds the call complete.
#ifndef RAC1_RUNTIME_SIF_RPC_H
#define RAC1_RUNTIME_SIF_RPC_H

#include <cstdint>
#include <functional>

namespace rac1 {

// A host IOP server: (function number, send buffer, send size, receive buffer, receive size),
// the buffers as EE addresses.
using SifRpcServer = std::function<void(uint32_t fno, uint32_t send, uint32_t ssize, uint32_t recv, uint32_t rsize)>;

void sif_rpc_register(uint32_t sid, SifRpcServer server);
void sif_rpc_reset();  // forget the bound clients (a fresh EE)

// sceSifInitRpc(mode): the SIF RPC layer up (the host's is always).
void sif_init_rpc(uint32_t mode);
// sceSifBindRpc(client, sid, mode): the client (an sceSifClientData in EE RAM) bound to
// server sid; its +0x24 (serve) is set non-zero for a server the host has. 0, or -1.
int32_t sif_bind_rpc(uint32_t client, uint32_t sid, uint32_t mode);
// sceSifCallRpc(client, fno, mode, send, ssize, recv, rsize, end, end_param): runs the server.
int32_t sif_call_rpc(uint32_t client, uint32_t fno, uint32_t mode, uint32_t send, uint32_t ssize, uint32_t recv, uint32_t rsize, uint32_t end,
                     uint32_t end_param);
// sceSifCheckStatRpc(client): 1 while a call is in flight (never, here).
int32_t sif_check_stat_rpc(uint32_t client);
// sceSifSetDma(list, count): `count` transfers {EE src, IOP dest, bytes, attr} from EE memory
// into the host's IOP RAM, done at once. The transfer id (non-zero).
uint32_t sif_set_dma(uint32_t list, int32_t count);

}  // namespace rac1

#endif
