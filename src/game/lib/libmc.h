// libmc, the memory cards: host stand-ins. On the PS2 the base ELF's libmc
// (0x1233f0..0x124800) sends each call to the IOP's mcserv through SIF RPC (client 0x159a00,
// send buffer 0x159a80, reply word 0x15afc0, the command running at 0x132da8, a semaphore
// *(0x132dac)) and sceMcSync (0x123c30) reports it finished: *cmd the command (1 GetInfo, 2 Open,
// 3 Close, 4 Seek, 5 Read, 6 Write, 0xb Mkdir, 0xc ChDir, 0xd GetDir, 0xf Delete, 0x10 Format,
// 0x11 Unformat), *result the reply word. The host stands in at the calls (as libcdvd's do): a
// command is done at once, its result there at the next sceMcSync; a call made while one is
// pending is refused (-200, the semaphore's poll failing; -100 when the client is not bound is
// not met).
//
// The card: port 0, slot 0 (the game polls port 0 only) is a virtual 8 MB PS2 card held in
// memory, optionally backed by a host directory (written through: each write, mkdir and delete
// reaches the directory at once; the card's files are the files the game's code wrote, nothing
// added). No card inserted: every call on it answers -10 (sceMcResNoCard). Port 1 never has a
// card.
#ifndef RAC1_GAME_LIB_LIBMC_H
#define RAC1_GAME_LIB_LIBMC_H

#include <cstdint>
#include <string>
#include <vector>

namespace rac1 {

// sceMcInit (0x1233f0): 0.
int32_t mc_init();                                                                             // orig: (host) 0x001233f0 sceMcInit
// sceMcGetInfo(port, slot, &type, &free, &format): 0 accepted. Result: 0 the same card, -1 a
// card newly seen (the first call after it was inserted), -2 unformatted, -10 no card; type 2
// (PS2 card), free clusters (KB), format 1 / 0.
int32_t mc_get_info(int32_t port, int32_t slot, uint32_t type, uint32_t free, uint32_t format);  // orig: (host) 0x00123d68 sceMcGetInfo
// sceMcOpen(port, slot, name, mode): mode 1 read, 2 write, 3 both, 0x200 create (an existing
// file is not truncated), 0x40 a directory (sceMcMkdir). Result: a descriptor (>= 0) or -2
// unformatted, -3 no room, -4 not found (or the directory exists), -5 denied, -7 too many open.
int32_t mc_open(int32_t port, int32_t slot, uint32_t name, int32_t mode);                       // orig: (host) 0x001235b0 sceMcOpen
int32_t mc_mkdir(int32_t port, int32_t slot, uint32_t name);                                    // orig: (host) 0x001236d8 sceMcMkdir
int32_t mc_close(int32_t fd);                                                                   // orig: (host) 0x00123710 sceMcClose
// sceMcSeek(fd, offset, origin 0 start / 1 current / 2 end): the new position.
int32_t mc_seek(int32_t fd, int32_t offset, int32_t origin);                                    // orig: (host) 0x001237c8 sceMcSeek
// sceMcRead / sceMcWrite(fd, EE buffer, size): the bytes moved.
int32_t mc_read(int32_t fd, uint32_t buf, int32_t size);                                        // orig: (host) 0x00123930 sceMcRead
int32_t mc_write(int32_t fd, uint32_t buf, int32_t size);                                       // orig: (host) 0x00123a48 sceMcWrite
// sceMcGetDir(port, slot, pattern, mode 0 first / 1 next, max, table of 64-byte entries): the
// entries written (+0 created, +8 modified (sceMcStDateTime), +0x10 size, +0x14 attributes,
// +0x20 the name).
int32_t mc_get_dir(int32_t port, int32_t slot, uint32_t name, int32_t mode, int32_t max, uint32_t table);  // orig: (host) 0x00123ef0 sceMcGetDir
// sceMcChdir(port, slot, dir, &current or 0): 0, or -4 no such directory.
int32_t mc_chdir(int32_t port, int32_t slot, uint32_t dir, uint32_t cur);                       // orig: (host) 0x001240c0 sceMcChdir
int32_t mc_format(int32_t port, int32_t slot);                                                  // orig: (host) 0x00124200 sceMcFormat
// sceMcDelete(port, slot, name): 0, -4 not found, -5 denied, -6 a directory not empty.
int32_t mc_delete(int32_t port, int32_t slot, uint32_t name);                                   // orig: (host) 0x001242d0 sceMcDelete
int32_t mc_unformat(int32_t port, int32_t slot);                                                // orig: (host) 0x001243e8 sceMcUnformat
// sceMcSync(mode, &cmd, &result): 1 the command finished (*cmd, *result written), -1 none.
int32_t mc_sync(int32_t mode, uint32_t cmd, uint32_t result);                                  // orig: (host) 0x00123c30 sceMcSync

// The host's card: inserted backed by `dir` (created when missing; empty: in memory only), or
// removed. The default card directory: $RAC1_MEMCARD_DIR, else $XDG_DATA_HOME/rac1-native/memcard,
// else ~/.local/share/rac1-native/memcard.
void mc_card_insert(const std::string& dir);
void mc_card_remove();
bool mc_card_present();
std::string mc_default_card_dir();
// testing: the stand-in's state cleared (no card, no command pending), or a command pending set
void mc_reset();
void mc_set_pending(bool on, int32_t cmd, int32_t result);
void mc_snapshot_take();     // the card and the pending command kept / put back
void mc_snapshot_restore();
void mc_set_trace(uint32_t ee_word);  // a hash of the calls (and the data written) kept in this EE word
void mc_card_put(const std::string& path, const std::vector<uint8_t>& data);  // "DIR/FILE"
std::vector<uint8_t> mc_card_get(const std::string& path);
// The host's own files on the card (the PC version's per-save settings next to the
// game's files, which stay as the game wrote them): "DIR/FILE" written (through to the backing
// directory) when the card is present and DIR exists, else false; read: false when absent.
bool mc_card_file_write(const std::string& path, const std::vector<uint8_t>& data);
bool mc_card_file_read(const std::string& path, std::vector<uint8_t>& out);

}  // namespace rac1

#endif
