// See ovl_reloc.h.
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/variants_sky.h>
#include <game/ovl/variants.h>
#include <runtime/kernel/disc.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace rac1 {

uint32_t g_ovl_gen = 0;

namespace {

int g_level = 0;
bool g_identity = true;  // the ports' addresses as they are (NTSC-U Veldin's image, or no overlay)
std::map<uint32_t, uint32_t> g_data;               // Veldin -> level (the maps' data lines)
std::map<uint32_t, uint32_t> g_code;               // Veldin -> level (fn lines)
std::map<uint32_t, uint32_t> g_code_rev;           // level -> Veldin
std::unordered_map<uint32_t, uint32_t> g_code_size;  // Veldin function -> its size (bytes)
std::set<uint32_t> g_code_kept;                     // Veldin addresses handed out untranslated
// Veldin -> level code pointers the matched code builds (lui + addiu: callbacks, table entries)
// whose function is not the same code in the level (not an `fn` line: a variant); and back
std::map<uint32_t, uint32_t> g_code_ptr, g_code_ptr_rev;

// orig: (host) another region's disc: the region table's code runs of the level's own overlay
// (region_levelN.txt: NTSC-U level N -> the disc's level N, tools/overlay_match/region_maps.py), for the level
// addresses the ports carry (the variants' rows); 0 when the address is not in a run (code the region changed)
struct RegionRuns {
    std::vector<std::array<uint32_t, 3>> code;  // lo, hi (exclusive), displacement
    uint32_t fwd(uint32_t a) const {
        for (const auto& r : code)
            if (a >= r[0] && a < r[1]) return a + r[2];
        return 0;
    }
};
// orig: (host) see RegionRuns
RegionRuns region_runs(int level) {
    RegionRuns rr;
    std::ifstream in("assets/generated/overlay_map/region_level" + std::to_string(level) + ".txt");
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string kind, d;
        uint32_t lo = 0, hi = 0;
        s >> kind >> std::hex >> lo >> hi >> d;
        if (kind == "code") rr.code.push_back({lo, hi, static_cast<uint32_t>(std::stoll(d, nullptr, 16))});
    }
    return rr;
}

// orig: (host) an untranslated address, reported once
void log_once(const char* what, uint32_t a, const char* how = "has no translation: kept") {
    static std::set<std::pair<std::string, uint32_t>> seen;
    if (seen.insert({what, a}).second) std::fprintf(stderr, "[ovl_reloc] level %d: %s %08x %s\n", g_level, what, a, how);
}

}  // namespace

// orig: (host) see ovl_reloc.h
void ovl_reloc_set_level(int level) {
    g_data.clear();
    g_code.clear();
    g_code_rev.clear();
    g_code_kept.clear();
    g_code_size.clear();
    g_code_ptr.clear();
    g_code_ptr_rev.clear();
    g_level = level == OVL_NONE ? 0 : level;
    g_identity = true;
    ++g_ovl_gen;
    // Veldin is the identity on the NTSC-U disc only: on another region's disc its overlay is the
    // region's own build, translated like any level's (level0.txt)
    if (level == OVL_NONE || (level == 0 && !disc_other_region())) return;
    g_identity = false;
    const std::string path = level == OVL_FRONT ? std::string("assets/generated/overlay_map/front_reloc.txt")
                                                : "assets/generated/overlay_map/level" + std::to_string(level) + ".txt";
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "[ovl_reloc] %s missing (setup.sh): level %d kept on Veldin's addresses\n", path.c_str(), level);
        g_level = 0;
        g_identity = true;
        return;
    }
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string kind;
        uint32_t v = 0, l = 0, n = 0;
        s >> kind >> std::hex >> v >> l >> n;
        if (kind == "data") g_data[v] = l;
        else if (kind == "fn") {
            g_code[v] = l;
            g_code_rev[l] = v;
            g_code_size[v] = n;
        }
    }
    // the rows' level addresses are NTSC-U level N's: on another region's disc, the region table's
    const bool other = disc_other_region() && level != OVL_FRONT;
    const RegionRuns rr = other ? region_runs(level) : RegionRuns{};
    auto add_row = [&](const VariantRow& r) {
        uint32_t l = r.level;
        if (other && (l = rr.fwd(r.level)) == 0) {
            log_once("variant", r.level, "is code the region changed: its row dropped");
            return;
        }
        g_code[r.veldin] = l;
        g_code_rev[l] = r.veldin;
        g_code_size[r.veldin] = r.size;
    };
    {  // the level's variants (hand-checked, variants_l1.cpp): the level's own function stands in for Veldin's
        size_t n = 0;
        const VariantRow* rows = ovl_variants(level, &n);
        for (size_t i = 0; i < n; ++i) add_row(rows[i]);
    }
    {  // the level's own sky dispatcher (sky_levels.cpp): Veldin's 0x23d968 is another function on every level
        VariantRow row;
        if (sky_dispatcher_row(level, &row)) add_row(row);
    }
    for (const auto& [v, l] : g_data)  // (the data lines in Veldin's code: code pointers)
        if (v >= 0x001eaa00u && v < 0x002f0cd0u && g_code.count(v) == 0) {
            g_code_ptr[v] = l;
            g_code_ptr_rev[l] = v;
        }
}

// orig: (host) see ovl_reloc.h
int ovl_reloc_level() { return g_level; }

// orig: (host) see ovl_reloc.h
bool ovl_reloc_identity() { return g_identity; }

// orig: (host) see ovl_reloc.h
uint32_t ovl_xlate_data(uint32_t a) {
    if (g_identity) return a;
    const auto it = g_data.find(a);
    if (it != g_data.end()) return it->second;
    // an offset into a mapped object (a port's folded field offset): the nearest mapped address
    // below, close enough, with its displacement -- certain when the mapped address above moved
    // by the same amount (the block between them moved as one), else a guess (reported)
    auto hi = g_data.upper_bound(a);
    if (hi != g_data.begin()) {
        const auto lo = std::prev(hi);
        const uint32_t d = lo->second - lo->first;
        if (hi != g_data.end() && hi->second - hi->first == d) return a + d;
        if (a - lo->first < 0x100) {
            log_once("data", a, "guessed from the object below");
            return a + d;
        }
    }
    log_once("data", a);
    return a;
}

// orig: (host) see ovl_reloc.h
uint32_t ovl_code_lookup(uint32_t a) {
    if (g_identity) return a;
    const auto it = g_code.find(a);
    if (it != g_code.end()) return it->second;
    // a code pointer the level's matched code builds (lui + addiu; the maps' data lines): the
    // level's own function there, even when it is not the same code as Veldin's (a variant:
    // its dispatch finds no port and logs it)
    const auto ptr = g_data.find(a);
    if (ptr != g_data.end()) return ptr->second;
    // inside a mapped function (a callback address in the middle of one): its displacement
    auto lo = g_code.upper_bound(a);
    if (lo != g_code.begin()) {
        auto start = std::prev(lo);
        if (a - start->first < g_code_size[start->first]) return a + (start->second - start->first);
    }
    return 0;
}

// orig: (host) see ovl_reloc.h
uint32_t ovl_xlate_code(uint32_t a) {
    if (g_identity) return a;
    if (const uint32_t l = ovl_code_lookup(a); l != 0) return l;
    // a code pointer the matched code builds, to a function that is other code in the level
    // (a variant): the level's own function
    if (const auto p = g_code_ptr.find(a); p != g_code_ptr.end()) return p->second;
    log_once("code", a);
    g_code_kept.insert(a);
    return a;
}

// orig: (host) see ovl_reloc.h
uint32_t ovl_code_to_veldin(uint32_t a) {
    if (g_identity) return a;
    const auto it = g_code_rev.find(a);
    if (it != g_code_rev.end()) return it->second;
    if (g_code_kept.count(a) != 0) return a;  // (a Veldin address the translation kept: its port)
    auto lo = g_code_rev.upper_bound(a);  // (inside a mapped function, as above)
    if (lo != g_code_rev.begin()) {
        auto start = std::prev(lo);
        if (a - start->first < g_code_size[start->second]) return a - start->first + start->second;
    }
    // the level's variant of a Veldin function (a code pointer pair): Veldin's port stands in
    if (const auto p = g_code_ptr_rev.find(a); p != g_code_ptr_rev.end()) return p->second;
    return g_code_kept.count(a) != 0 ? a : 0;  // (a Veldin address the translation kept)
}

}  // namespace rac1
