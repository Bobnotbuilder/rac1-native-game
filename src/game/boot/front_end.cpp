// The boot module's main (on its own image since): the boot module's own
// code, ported at its own addresses (`// orig: front:0x...`, see front_image.h); its loops (the
// memory card check's pictures, the movies, the title loop's wait and passes) split into the
// host's frames. The addresses below are the boot module's.
#include <game/boot/front_end.h>

#include <game/boot/boot_start.h>
#include <game/boot/title.h>
#include <game/lib/movie_player.h>
#include <game/lib/snd989.h>
#include <game/math/vecmath.h>
#include <game/ovl/level_change.h>
#include <game/ovl/memory_card.h>
#include <game/ovl/movies.h>
#include <game/ovl/music_streams.h>
#include <game/ovl/ovl.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/screen_fade.h>
#include <game/port_stubs.h>

#include <cstdio>
#include <cstdlib>

#include <game/ovl/ovl_reloc.h>
namespace rac1 {

namespace {
constexpr uint32_t TOC = 0x00137b80;
constexpr uint32_t BOOT_DATA = 0x00500000;  // the boot data, unpacked by the start (0x201650)
constexpr FrConst<0x0015f604u> MODE{};       // the game mode (the overlays' 0x15f5c4)

enum class Phase { Off, Card, Intro, Wait, Title, Attract, Left };
struct Boot {
    Phase phase = Phase::Off;
    int32_t card = 0;     // the main's s2: the memory card check's last result
    int32_t shown = 0;    // its s0: the picture shown
    int32_t passes = 0;   // its s3
    int32_t wait = 0;     // the title loop's frames to wait
    bool picture = false; // a boot picture in the frame buffer (until the title draws)
};
Boot g_boot;

// A boot picture (the boot data's table `table` + language * 8: offset, size) unpacked into the
// work area (the boot data's +0x70) and sent into the frame buffer, with the frame's packets.
// orig: (helper) repeated in front:0x001e9658 (its card check loop and after the intro)
void picture_frame(uint32_t table) {
    const uint32_t s1 = BOOT_DATA;
    // (FlushCache around the unpacking: no memory effect)
    wad_decompress(s1 + lwu(table), s1 + lwu(s1 + 0x70));
    sub_002a07f8();
    bg_color_set(0, 0, 0);
    packet_call_display();
    sub_00200fd8();
    boot_picture(s1 + lwu(s1 + 0x70));
    sub_00201040();
    packet_call_151b60();
    TODO_PORT(0x002336a0, "front: DMA: packet kick (the overlays' 0x2a08c8)")();
    sub_002a0858();
    dma_wait(1);
}

// The memory card check's pass (the main's loop at 0x1e97f4): the check; none (0) or, after 11
// passes, a button pressed ends it; else its picture (1: "No Memory Card (PS2) inserted ...",
// 2: "Insufficient space ..." of the language) sent when it changed, a vsync, the pad. False
// once it ends (a fade when a picture was shown).
// orig: front:0x001e9658 (its memory card check loop, one pass)
bool card_pass() {
    Boot& b = g_boot;
    b.card = boot_card_check();
    if (b.card == 0 || (!(b.passes < 0xb) && lwu(0x0013c940 + 0x1a4) != 0)) {
        if (b.shown != 0) screen_fade(0xa);
        return false;
    }
    if (b.card != b.shown) {
        const uint32_t tab = (b.card ^ 1) == 0 ? BOOT_DATA + 0x10 : BOOT_DATA + 0x40;
        picture_frame(tab + lwu(0x0015ed88) * 8);
        b.picture = true;
    }
    TODO_PORT(0x00122298, "kernel: vsync wait")(0);
    b.shown = b.card;
    pad_update();
    ++b.passes;
    return true;
}

// The intro movie (0.pss NTSC, 1.pss PAL: table of contents 0x139378 / 0x139380), not
// skippable (0x15eed8 = -1), played in the buffers above the boot data's work area, language 0.
// orig: front:0x001e9658 (its intro movie)
void intro_start() {
    const uint32_t a2 = lwu(BOOT_DATA + 0x70) + BOOT_DATA;
    sw(0x0015eed8, 0xffffffffu);
    sw(FR(0x0015ef5c), a2);
    const bool pal = lw(0x0015ed80) != 0;
    const uint32_t row = pal ? 0x00139380u : 0x00139378u;
    movie_stream_start(lwu(row), lwu(row + 4), (a2 + 0x3f) & 0xffffffc0u, (a2 + 0x2c003f) & 0xffffffc0u, 0);
    std::printf("[boot] the intro movie\n");
}

// After the intro: the fade, the copyright picture (the boot data's +0 NTSC / +8 PAL), the frame
// counter, the title's sound bank (table of contents +0x14e0) and its sound tables (0x186100: 7
// sounds, the title's; 0x1861e0: class 0x472's), then the title loop's start.
// orig: front:0x001e9658 (after its intro movie)
void after_intro() {
    sw(0x0015eed8, 0);
    screen_fade(frame_scale_i(0x12));
    picture_frame(BOOT_DATA + (lw(0x0015ed80) != 0 ? 8u : 0u));
    g_boot.picture = true;
    TODO_PORT(0x00122298, "kernel: vsync wait")(0);
    sw(FR(0x0015f438), lwu(FR(0x0015f438)) + 1);
    debug_printf_stub();
    const uint32_t bank = static_cast<uint32_t>(boot_bank_load(lwu(TOC + 0x14e0)));
    snd_cmd_08();
    sw(FR(0x001861e0) + 0x9c, bank);
    sw(FR(0x00186100) + 0xdc, bank);
    sw(FR(0x0015f634), FR(0x00186100));
    sw(FR(0x0015f630), 7);
    for (uint32_t o : {0x1cu, 0x3cu, 0x5cu, 0x7cu, 0x9cu, 0xbcu}) sw(FR(0x00186100) + o, bank);
    for (uint32_t o : {0x1cu, 0x3cu, 0x5cu, 0x7cu}) sw(FR(0x001861e0) + o, bank);
    g_boot.wait = title_loop_start();
    if (g_boot_title_loaded_hook != nullptr) g_boot_title_loaded_hook();  // (host: the renderers' copy of the title's level)
    g_boot.phase = Phase::Wait;
    std::printf("[boot] the title loaded (bank %08x)\n", bank);
}

// An attract movie (table of contents +0x1a78 + k * 8, PAL +0x1a98: 80..83.pss NTSC): any button
// skips it (0x15eed8 = 2), the HUD flag 8, the sounds and streams stopped, a fade, game mode 1,
// the memory card's machine run until idle, then the player in the buffers above the title's
// level (*(0x1940dc) + 0x100000 / + 0x400000), language 0.
// orig: front:0x001e9488 (up to its player)
void attract_start(int32_t k) {
    if (k < 0) return;
    const bool pal = lw(0x0015ed80) != 0;
    const uint32_t row = TOC + static_cast<uint32_t>(k) * 8 + (pal ? 0x1a98u : 0x1a78u);
    const uint32_t size = lwu(row + 4), lsn = lwu(row);
    sw(0x0015eed8, 2);
    sb(0x0013e550 + 0x6b, lbu(0x0013e550 + 0x6b) | 8);
    // (FlushCache)
    sound_stop_all();
    music_stop_all();
    screen_fade(frame_scale_i(0xc));
    sw(MODE, 1);
    // (FlushCache)
    sound_stop_all();
    music_stop_all();
    snd_cd_status(0);
    while (!(lw(0x0013d290 + 0xd4) < 3) || lw(0x0013d290 + 0xdc) >= 0) card_machine();
    const uint32_t a = lwu(FR(0x001940dc));
    movie_stream_start(lsn, size, a + 0x100000, a + 0x400000, 0);
}

// The attract movie's end: the vsync callback, the GS, the HUD's texture page again, 0x15eed8 0,
// a fade, game mode 0, the HUD flag 0x10.
// orig: front:0x001e9488 (after its player)
void attract_end() {
    TODO_PORT(0x00120c30, "libcdvd: sceCdSync")(0);
    TODO_PORT(0x00122298, "kernel: vsync wait")(0);
    TODO_PORT(0x00120558, "libgraph: sceGsSyncPath")(0, 0);
    TODO_PORT(0x00122e68, "libgraph: the vsync callback set")(0x0012f1c8u);
    hud_tex_upload(0x01000000u, lw(0x0015ee88), 0x1b, 6, 6, 1);
    sw(0x0015eed8, 0);
    screen_fade(4);
    sw(MODE, 0);
    sb(0x0013e550 + 0x6b, lbu(0x0013e550 + 0x6b) | 0x10);
}

// The main after its title loop (0x1e99c0): the video mode switched if the options asked it,
// 0x13e05a 1, the next level the current one (0 for a new game, the saved level for a loaded
// one), the current level -1: then its level exit (0x231ff0, the overlays' 0x292e08: the host's).
// orig: front:0x001e9658 (after its title loop)
void title_left() {
    title_loop_end();
    const uint32_t want = lbu(FR(0x0016034c));
    if (lwu(0x0015ed80) != want) {
        sw(0x0015ed80, want);
        TODO_PORT(0x001204b8, "libgraph: sceGsResetPath")();
        TODO_PORT(0x00121878, "libgraph: sceGsResetGraph")(0, 1, lw(0x0015ed80) == 0 ? 2 : 3, 0);
        TODO_PORT(0x001f34e8, "front: the GS start (the overlays' 0x1f80d0)")();
        TODO_PORT(0x001fb2a8, "front: the display packet sent (the overlays' 0x200f18)")();
    }
    sh(0x0013e030 + 0x2a, 1);
    sw(FR(0x0015f600), lwu(0x0015ed84));
    sw(0x0015ed84, 0xffffffffu);
    g_boot.phase = Phase::Left;
    std::printf("[boot] the title left for level %d\n", lw(FR(0x0015f600)));
}
}  // namespace

// The boot module's main up to its memory card check: the front end flag, its start (0x201650),
// the texture base, its BSS cleared, the frame's state, the render enables, the display, black.
// orig: front:0x001e9658 (its start)
void boot_start() {
    sw(FR(0x0015f5e8), 1);
    boot_module_start();
    boot_tex_base();
    for (uint32_t a = FR(0x00161280); a < FR(0x00165430); ++a) sb(a, 0);
    sub_002a07f8();
    sub_002a1128();
    render_enables_init();
    sw(FR(0x0015f438), 0);
    TODO_PORT(0x00122298, "kernel: vsync wait")(0);
    sw(MODE, 0);
    TODO_PORT(0x001fb2a8, "front: the display packet sent (the overlays' 0x200f18)")();
    bg_color_set(0, 0, 0);
    g_boot = Boot{};
    g_boot.phase = Phase::Card;
    std::printf("[boot] the boot module started: the memory card check\n");
}

// One frame of the boot: the memory card check's pass, the intro movie's frame, a frame of the
// title loop's wait, one pass of the title loop, an attract movie's frame. False once the title
// loop is left (0x15f5b0, by New Game / Load Game / the debug menu): the next level then in
// 0x15f600, the current level -1 (the host runs the level exit, the boot module's 0x231ff0).
// orig: front:0x001e9658 (its loops' frames)
bool boot_frame() {
    Boot& b = g_boot;
    switch (b.phase) {
        case Phase::Card:
            if (lw(FR(0x0015f5b0)) != 0 || !card_pass()) {
                if (lw(FR(0x0015f5b0)) != 0) {  // (the debug menu's skip)
                    title_left();
                    return false;
                }
                b.phase = Phase::Intro;
                intro_start();
            }
            return true;
        case Phase::Intro:
            if (!movie_stream_frame()) return true;
            if (lw(FR(0x0015f5b0)) != 0) {
                title_left();
                return false;
            }
            after_intro();
            return true;
        case Phase::Wait:
            if (--b.wait > 0) {
                TODO_PORT(0x00122298, "kernel: vsync wait")(0);
                return true;
            }
            title_loop_ready();
            b.phase = Phase::Title;
            b.picture = false;
            std::printf("[boot] the title\n");
            return true;
        case Phase::Title: {
            if (std::getenv("RAC1_TITLE_TRACE") != nullptr && lw(FR(0x0018cb20) + 0x34) % 100 == 1) {  // testing: the title's scene
                const uint32_t S = FR(0x0018cb20u);
                std::printf("[title] scene frame %d block %d (frame %d of %d) mobys %d list %08x..%08x\n", lw(S + 0x34), lw(S + 0x3c), lw(S + 0x38), lh(S + 0x40), lh(S + 0x44), lwu(FR(0x0015ff18)), lwu(FR(0x0015ff1c)));
                for (int32_t i = 0; i < lh(S + 0x44) && i < 16; ++i) {
                    const uint32_t m = lwu(S + 0x178 + static_cast<uint32_t>(i) * 4);
                    std::printf("[title]  moby %08x class %d state %d fn %08x pos %.2f %.2f %.2f flags %04x scale %.3f\n", m, lh(m + 0xa6), lbu(m + 0x20), lwu(m + 0x74), lwc1(m + 0x10), lwc1(m + 0x14), lwc1(m + 0x18), lhu(m + 0x34), lwc1(m + 0x2c));
                }
                std::printf("[title] camera %.2f %.2f %.2f\n", lwc1(FR(0x00187080)), lwc1(FR(0x00187084)), lwc1(FR(0x00187088)));
            }
            const int32_t r = title_loop_frame();
            if (r == 1) {
                attract_start(g_title_attract);
                b.phase = Phase::Attract;
                return true;
            }
            if (r == 2) {
                title_left();
                return false;
            }
            return true;
        }
        case Phase::Attract:
            if (!movie_stream_frame()) return true;
            attract_end();
            title_attract_after();
            b.phase = Phase::Title;
            return true;
        default:
            return false;
    }
}

void (*g_boot_title_loaded_hook)() = nullptr;

bool boot_active() { return g_boot.phase != Phase::Off && g_boot.phase != Phase::Left; }

bool boot_in_title() { return g_boot.phase == Phase::Title; }

bool boot_in_card_check() { return g_boot.phase == Phase::Card; }

bool boot_picture_shown() { return g_boot.picture && (g_boot.phase == Phase::Card || g_boot.phase == Phase::Intro || g_boot.phase == Phase::Wait); }

// orig: (host) the debug menu's skip: the title left as a loaded game leaves it (0x15ed84 the
// level, the level change flag 0x15f5b0)
void boot_skip_to(int32_t level) {
    if (!boot_active()) return;
    if (g_boot.phase == Phase::Intro || g_boot.phase == Phase::Attract) {  // (a movie playing ends at its next frame)
        movie_player_stop();
        while (!movie_stream_frame()) {
        }
        if (g_boot.phase == Phase::Attract) attract_end();
        sw(0x0015eed8, 0);
        g_boot.phase = Phase::Title;
    }
    if (g_boot.phase == Phase::Wait) g_boot.phase = Phase::Title;
    sw(0x0015ed84, static_cast<uint32_t>(level));
    sw(FR(0x0015f600), static_cast<uint32_t>(level));
    sw(FR(0x0015f5b0), 1);
    sw(FR(0x0015f618), 1);
}

}  // namespace rac1
