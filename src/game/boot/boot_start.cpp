// The boot module's start and its boot pictures: its own code (its own image, see
// front_image.h), ported at its own addresses (`// orig: front:0x...`); the engine code it shares
// with the levels is the ports' (src/game/ovl) relocated onto it. The addresses below are the
// boot module's.
#include <game/boot/boot_start.h>

#include <game/boot/level_read.h>
#include <game/lib/libgraph.h>
#include <game/lib/libmc.h>
#include <game/lib/snd989.h>
#include <game/math/vecmath.h>
#include <game/ovl/moby_pass.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/port_stubs.h>
#include <runtime/ee/ee_timer.h>
#include <runtime/kernel/disc.h>

namespace rac1 {

namespace {
constexpr uint32_t TOC = 0x00137b80;
}  // namespace

// The texture base of the frame's uploads back to the texture base, the HUD's counter 0.
// orig: front:0x001e9128
void boot_tex_base() {
    sw(0x0015ef20, 0);
    sw(0x0015ee74, lwu(0x0015ee8c));
}

// A 256-entry table at 0x18e740: entry i (bits 3 and 4 swapped) holds (i >> 1) << 24 (the
// CLUT's storage order of an 8-bit palette: CSM1).
// orig: front:0x001f7a30
void boot_clut_order() {
    for (uint32_t i = 0; i < 0x100; ++i) {
        uint32_t k = i & 0xe7;
        if ((i & 8) != 0) k |= 0x10;
        if ((i & 0x10) != 0) k |= 8;
        sw(0x0018e740 + k * 4, (i >> 1) << 24);
    }
}

// The boot module's start: the IOP's modules, the GS, the disc's region (NTSC unless the
// sector 0x121's byte 0x33 is not 'N'), the table of contents, the boot data (table of contents
// +0x12c0) read below 0x1ff8000 and unpacked to 0x500000 (its IOP modules loaded from it), the
// memory map, the screen and projection, the sound, an 8 x 8 white texture at 0x3ffb, the debug
// font, the CLUT order table, the console's language.
// orig: front:0x00201650
void boot_module_start() {
    // the IOP: reset, its modules loaded, SIF RPC, the disc, the pad: the host's (IOP stand-ins)
    TODO_PORT(0x001204b8, "libgraph: sceGsResetPath")();
    TODO_PORT(0x00123008, "front: the IOP's reboot and modules (sceSifRebootIop, sceSifLoadModule, sceCdInit ...)")();
    // the disc's region: sector 0x121 (0x12f208), its byte 0x33 'N' (NTSC); the host's disc is
    // the user's NTSC-U ISO
    {
        uint8_t sec[2048] = {};
        const uint32_t pal = disc_read(0x121, 1, sec) && sec[0x33] != 'N' ? 1u : 0u;
        sw(0x0015ee90, pal);
        sw(0x0015ed80, pal);
    }
    TODO_PORT(0x00209030, "front: the memory card's start (the overlays' 0x24b410)")();
    TODO_PORT(0x00121878, "libgraph: sceGsResetGraph")(0, 1, lw(0x0015ed80) == 0 ? 2 : 3, 0);
    TODO_PORT(0x0020b418, "front: the overlays' 0x24d6d0")();
    TODO_PORT(0x001f34e8, "front: the GS start (the overlays' 0x1f80d0; the host's gs_memory_and_draw_envs)")();
    vif0_send(0x0010e4c0);  // the VU0 programs
    toc_read();
    {  // the boot data: read below 0x1ff8000, unpacked to 0x500000
        const uint32_t n = lwu(TOC + 0x12c4);
        const uint32_t src = 0x01ff8000u - (n << 11);
        constexpr uint32_t B = 0x00500000;
        disc_read_ee(src, lwu(TOC + 0x12c0), n);  // (0x12f208; FlushCache around the unpacking)
        wad_decompress(src, B);
        // its ten IOP modules (+0x70 .. +0xb4: offset, size; +0x98 twice) loaded by 0x201520
        // (sceSifLoadModuleBuffer): the host's IOP stand-ins
        TODO_PORT(0x00201520, "front: IOP modules from the boot data (the host's IOP)")();
    }
    debug_printf_stub();
    TODO_PORT(0x00217048, "front: the overlays' 0x266ed0")();
    TODO_PORT(0x00122e68, "libgraph: the vsync callback set")(0x0012f1c8u);
    sw(0x00160f0c, 0x00160000u);
    memory_layout();
    TODO_PORT(0x0020ac58, "front: the overlays' 0x24cf10")();
    screen_setup();
    projection_setup();
    sub_002a07f8();
    sound_system_init();
    {  // an 8 x 8 white texture at block 0x3ffb
        mem_fill32(0x001941c0, 0x80808080u, 0x100);
        const uint32_t lp = ee_stack_alloc(0x60);
        gs_set_def_load_image(lp, 0x3ffb, 1, 0, 0, 0, 8, 8);
        gs_exec_load_image(lp, 0x001941c0);  // (after FlushCache)
        gs_sync_path(0, 0);
        ee_stack_free(0x60);
    }
    TODO_PORT(0x001e9338, "front: the debug font (the overlays' 0x1eb000)")();
    ee_t1_reset();  // timer 1 (T1_MODE 0x82: BUSCLK / 256; T1_COUNT 0): runtime/ee/ee_timer.h
    TODO_PORT(0x00232ce0, "front: the overlays' 0x293af8")();
    boot_clut_order();
    // the console's language (0x12d240: 1 English -> 0, 2 French -> 2, 3 Spanish -> 4, 4 German
    // -> 3, 5 Italian -> 5, others 0) into 0x15ed88: the host has no console settings, its
    // stand-in (English, or the debug menu's saved choice) is set before the start
    TODO_PORT(0x0012d240, "kernel: the console's language")();
}

// The memory card checked (port / slot of the save block 0x13d290): 1 when there is none
// (sceMcGetInfo -10 or below, or -5) or it is not a PS2 card, 0 when the game's directory is on
// it or it is unformatted (-2: the game formats it later), else 2 when it has less than 350 KB
// free. Each sceMcSync waited for with vsyncs.
// orig: front:0x00209168
int32_t boot_card_check() {
    const uint32_t sp = ee_stack_alloc(0x20);
    sw(sp + 0x10, static_cast<uint32_t>(mc_get_info(lw(0x0013d290), lw(0x0013d294), sp, sp + 4, sp + 8)));
    while (mc_sync(1, 0x0013d348, 0x0013d34c) == 0) TODO_PORT(0x00122298, "kernel: vsync wait")(0);
    const int32_t r = lw(0x0013d34c);
    int32_t ret = 1;
    if (r == -5 || r < -9) {
        ret = 1;
    } else if (r == -2) {
        ret = 0;
    } else if (lw(sp) != 2) {
        ret = 1;
    } else if (lw(sp + 8) == 0) {
        ret = 0;
    } else {
        TODO_PORT(0x00123ef0, "libmc: sceMcGetDir (the game's directory)")();
        ret = 0;  // (the host has no card: never reached, sceMcGetInfo fails first)
    }
    ee_stack_free(0x20);
    return ret;
}

// A full-screen picture (512 x 416 PSMCT32, 448 PAL) at `src` sent into the frame buffer
// *(0x15ee84) by IMAGE transfers of 0x80 lines, in the frame's packets *(0x160f00), then a
// TEXFLUSH.
// orig: front:0x002012b8
void boot_picture(uint32_t src) {
    constexpr uint32_t CUR = 0x00160f00;  // the frame's packet cursor
    int32_t rows = lw(0x0015ed80) != 0 ? 0x1c0 : 0x1a0;
    int32_t dbp = static_cast<int32_t>(lwu(0x0015ee84)) >> 8;
    uint32_t data = src;
    uint32_t v1 = 0;
    do {
        const int32_t left = rows - 0x80;
        const int32_t n = -1 < left ? 0x80 : rows;
        const uint32_t qwc = static_cast<uint32_t>(n) << 7;
        uint32_t c = lwu(CUR);
        sw(c, 0x10000006u);
        sw(c + 4, 0);
        sw(c + 8, 0);
        sw(c + 0xc, 0x50000006u);
        const uint64_t bitbltbuf = static_cast<uint64_t>(static_cast<uint32_t>(dbp)) << 32 | 0x0008000000000000ull;
        dbp += n << 3;
        rows = left;
        sd(c + 0x10, 0x4000000000000001ull);
        sd(c + 0x18, 0x0eeeeeeeull);
        sd(c + 0x20, bitbltbuf);
        sd(c + 0x28, 0x50);
        sd(c + 0x30, 0);
        sd(c + 0x38, 0x51);
        sd(c + 0x40, static_cast<uint64_t>(static_cast<uint32_t>(n)) << 32 | 0x200);
        sd(c + 0x48, 0x52);
        sd(c + 0x50, 0);
        sd(c + 0x58, 0x53);
        sd(c + 0x60, qwc | 0x0800000000008000ull);
        sd(c + 0x68, 0);
        c += 0x70;
        sw(CUR, c);
        sw(c, qwc | 0x30000000u);
        sw(c + 4, data);
        data += static_cast<uint32_t>(n) << 11;
        sw(c + 8, 0);
        sw(c + 0xc, qwc | 0x50000000u);
        v1 = lwu(CUR);
        sw(CUR, v1 + 0x10);
    } while (rows > 0);
    sw(v1 + 0x10, 0x10000002u);
    sw(lwu(CUR) + 4, 0);
    sw(lwu(CUR) + 8, 0);
    sw(lwu(CUR) + 0xc, 0x50000002u);
    const uint32_t c = lwu(CUR);
    sd(c + 0x10, 0x1000000000008001ull);
    sd(c + 0x18, 0xe);
    sd(c + 0x20, 0);
    sd(c + 0x28, 0x3f);
    sw(CUR, c + 0x30);
}

// The title's sound bank (989snd: by disc location `lsn`, the bank's handle returned), the
// sound's busy flag 0x13e594 held meanwhile.
// orig: front:0x0022d708
int32_t boot_bank_load(uint32_t lsn) {
    constexpr uint32_t S = 0x0013e550;
    int32_t h = 0;
    if (lw(S + 0x44) == 0) {
        sw(S + 0x44, 1);
        TODO_PORT(0x00120c30, "libcdvd: sceCdSync")(0);
        h = snd_bank_load_by_loc(lsn, 0);
        snd_stream_reads_wait(1);
        snd_cd_status(0);
        snd_cmd_08();
        sw(S + 0x44, 0);
    }
    return h;
}

}  // namespace rac1
