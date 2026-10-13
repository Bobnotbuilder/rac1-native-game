// Oracle batch 149 (task 0416): the cutscene subtitle (0x1f9a20, cutscene_subtitle_draw), on RAM dumps of headless runs
// taken in game mode 2 with the Options' Subtitles switch (0x15ee40) on (assets/dumps/oracle/subtitles_0416/, made by
// tools/oracle/subtitles_dumps.sh; skipped without them):
//   veldin_scene2_L0.bin   Veldin's scene 2 (Clank meets Ratchet, frame 3000: tools/oracle/subtitles_dumps.sh), ORACLE_LEVEL=0;
//   umbris_arrival_L7.bin  Umbris's arrival scene (scene 0, Qwark's welcome: frame 1500 of a RAC1_LEVEL=7 run), ORACLE_LEVEL=7.
// Varied: the language 0x15ed88 (0..6: English, the unused 1, French, German, Spanish, Italian, past the list), the scene's
// frame +0x34 (inside / at the edges of a record's frames, or anywhere), the screen's height 0x13e504, the frame packet's
// cursor, a record's text made long (several lines: the box moved up), no table (+0x4c = 0), an empty table.
#include "ovl_oracle.h"

#include <game/ovl/cutscene.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>
#include <runtime/ee/ee_interpreter.h>

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace rac1::oracle {

namespace {
const std::vector<uint8_t>& st_dump(const std::string& path) {
    static std::map<std::string, std::vector<uint8_t>> cache;
    std::vector<uint8_t>& d = cache[path];
    if (d.empty()) {
        if (FILE* f = std::fopen(path.c_str(), "rb")) {
            d.resize(0x02000000 + 0x4000);
            if (std::fread(d.data(), 1, d.size(), f) != d.size()) d.clear();
            std::fclose(f);
        }
    }
    return d;
}
void st_load(const std::vector<uint8_t>& d) {
    std::memcpy(ee_get_ram_base(), d.data(), 0x02000000);
    std::memcpy(ee_ram_ptr(0x70000000u), d.data() + 0x02000000, 0x4000);
}
}  // namespace

void batch_149() {
    if (g_only_batch != 149) return;
    g_batch = 149;
    const int lv = g_oracle_level;
    if (lv != 0 && lv != 7) {
        skip_check("subtitle_draw", 0x001f9a20, "the subtitle dumps of task 0416: ORACLE_LEVEL=0 / 7");
        return;
    }
    const std::string path = lv == 0 ? dump_path("subtitles_0416/veldin_scene2_L0.bin") : dump_path("subtitles_0416/umbris_arrival_L7.bin");
    const std::vector<uint8_t>& d = st_dump(path);
    if (d.empty()) {
        skip_check("subtitle_draw", 0x001f9a20, "no subtitle RAM dump (tools/oracle/subtitles_dumps.sh)");
        return;
    }
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // (as in the runtime)
    const uint32_t SC = OV(0x0016c860u);
    check("subtitle_draw", 0x001f9a20, 200, Ret::None, [&d, SC] {
        st_load(d);
        w32(0x0015ed88, u(ri(0, 6)));
        const uint32_t tbl = ee_read<uint32_t>(SC + 0x4c);
        std::vector<uint32_t> recs;
        if (tbl != 0)
            for (uint32_t r = tbl; ee_read<int16_t>(r) >= 0 && recs.size() < 256; r += 0x10) recs.push_back(r);
        const int k = ri(0, 9);
        if (!recs.empty() && k < 7) {
            const uint32_t r = recs[static_cast<size_t>(ri(0, static_cast<int>(recs.size()) - 1))];
            const int32_t a = ee_read<int16_t>(r), b = ee_read<int16_t>(r + 2);
            const int e = ri(0, 4);  // inside, the first / last frame, one before / after
            const int32_t f = e == 0 ? ri(a, b > a ? b : a) : e == 1 ? a : e == 2 ? b : e == 3 ? a - 1 : b + 1;
            w32(SC + 0x34, u(f));
            if (ri(0, 3) == 0) {  // its text made long in every language (several lines: the box moved up)
                static const char* longs[] = {
                    "A much longer line of text, to see the box wrap it over several lines and move up from the bottom of the screen "
                    "so that it never passes the screen's height less twenty pixels, as the original does.",
                    "Short one."};
                const char* s = longs[ri(0, 1)];
                // (the offsets are s16: the string put 0x7000 past the table, in the block's buffer)
                const uint32_t near = tbl + 0x7000;
                for (size_t i = 0; i <= std::strlen(s); ++i) w8(near + static_cast<uint32_t>(i), static_cast<uint8_t>(s[i]));
                for (uint32_t l = 0; l < 5; ++l) w16(r + 4 + l * 2, 0x7000);
            }
        } else if (k == 7) {
            w32(SC + 0x34, u(ri(-10, 3000)));
        } else if (k == 8) {
            w32(SC + 0x4c, 0);
        } else if (tbl != 0) {
            w16(tbl, 0xffff);  // an empty table
        }
        if (ri(0, 4) == 0) w32(0x0013e504, u(ri(0x100, 0x200)));
        w32(OV(0x001611c0), ee_read<uint32_t>(OV(0x001611c0)) + u(ri(0, 0x40)) * 0x10);
        return Args{{}, {}};
    }, [](A) { cutscene_subtitle_draw(); return uint64_t{0}; });
    g_ee_null_is_ram = null_ram;
}

}  // namespace rac1::oracle
