// Oracle batch 108 (task 0332): the MRP world assets (src/formats/world_mrp.h, generated from the ISO by
// the setup (src/setup/mrp_export.cpp) into assets/generated/mrp/world) against the world's ported VU1 programs (task 0328, proved
// against the uploaded original by batch 104). For every VU1 start state captured in the running game (rac1-native with
// RAC1_VU1_CAPTURE=<dir> RAC1_VU1_CAPTURE_ALL=<n>: assets/dumps/oracle/vu1_world/*, the levels batch 104 uses) the packet the state
// holds is found in the assets by its content, the CPU reference evaluates the asset's kick program (shrub / tie: the traced
// GS stream; tfrag: the program re-implemented) with the instance data / constants the state holds, and the GIF packets the
// ported program kicks are compared qword by qword (the clipping programs B: every reference triangle the clip test leaves
// alone must be kicked whole). Each state also runs perturbed (ORACLE_MRPW_VARIANTS, default 3): the view's / the instances'
// w lanes scaled, the tie morph factors drawn: other LOD morphs, substitutions, fat-vertex blends.
// ORACLE_MRPW_DIR: the assets (default assets/generated/mrp/world); ORACLE_VU1_DIR=a:b the state directories.
#include "ovl_oracle.h"

#include <game/ovl/world_vu1.h>
#include <game/render/mrp_world_assets.h>
#include <game/render/mrp_world_check.h>
#include <runtime/vu/vu1.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace rac1::oracle {

namespace {

bool read_state(const std::string& path, VUState& st, std::vector<uint64_t>& micro, uint32_t& start) {
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (fp == nullptr) return false;
    uint32_t hdr[4];
    float fr[4];
    uint32_t fl[4];
    micro.assign(2048, 0);
    const bool rd = std::fread(hdr, 4, 4, fp) == 4 && std::fread(st.vf.data(), sizeof(VUVector), 32, fp) == 32 && std::fread(st.vi.data(), 2, 16, fp) == 16 &&
                    std::fread(&st.acc, sizeof(VUVector), 1, fp) == 1 && std::fread(fr, 4, 4, fp) == 4 && std::fread(fl, 4, 4, fp) == 4 &&
                    std::fread(st.mem.data(), 1, st.mem.size(), fp) == st.mem.size() && std::fread(micro.data(), 8, 2048, fp) == 2048;
    std::fclose(fp);
    if (!rd || hdr[0] != 0x31555676u) return false;
    st.q = fr[0], st.i = fr[1], st.r = fr[2], st.p = fr[3];
    st.macflag = fl[0], st.statusflag = fl[1], st.clipflag = fl[2];
    st.xtop = hdr[2];
    start = hdr[1];
    return true;
}

float getf(const VUState& st, uint32_t q, int l) {
    float f;
    std::memcpy(&f, &st.mem[((q & 0x3ff) << 4) + 4 * static_cast<uint32_t>(l)], 4);
    return f;
}
void setf(VUState& st, uint32_t q, int l, float v) { std::memcpy(&st.mem[((q & 0x3ff) << 4) + 4 * static_cast<uint32_t>(l)], &v, 4); }

// a perturbed copy of the state: the w lanes of the matrix rows the program reads scaled by `s`, tie morph factors drawn
void perturb(VUState& st, mrpw::Program prog, std::mt19937& rng) {
    std::uniform_real_distribution<float> u(0.0f, 1.0f);
    const float s = std::exp2(u(rng) * 2.0f - 1.0f);  // 0.5 .. 2
    auto scale_w = [&](uint32_t row0) {
        for (uint32_t r = 0; r < 4; ++r) setf(st, row0 + r, 3, getf(st, row0 + r, 3) * s);
    };
    switch (prog) {
    case mrpw::Program::TfragA:
    case mrpw::Program::TfragB:
        scale_w(st.xtop + 5);
        break;
    case mrpw::Program::TieA:
    case mrpw::Program::TieB: {
        const uint32_t block = static_cast<uint32_t>(st.vi[3]) & 0x3ff;
        scale_w(block);
        const float a = u(rng);
        setf(st, block + 4, 0, a);
        setf(st, block + 4, 2, 256.0f * a);
        setf(st, block + 4, 3, 256.0f * (1.0f - a));
        break;
    }
    case mrpw::Program::ShrubA:
    case mrpw::Program::ShrubB: {
        uint32_t v;
        std::memcpy(&v, &st.mem[16 * 1 + 4], 4);
        const uint32_t block = (0x269u - (v & 0xffff)) & 0x3ff;
        std::memcpy(&v, &st.mem[16 * block], 4);
        for (uint32_t i = 0; i < (v & 0xffff) && i < 8; ++i) scale_w(block + 1 + 0x1c * i);
        break;
    }
    default:
        break;
    }
}

}  // namespace

void batch_108() {
    if (g_only_batch != 108) return;
    g_batch = 108;
    const char* adir = std::getenv("ORACLE_MRPW_DIR");
    const std::string assets = adir != nullptr ? adir : "assets/generated/mrp/world";
    std::vector<std::unique_ptr<mrpw::World>> worlds;
    mrpw::WorldIndex idx;
    for (int l = 0; l < 19; ++l) {
        auto w = std::make_unique<mrpw::World>();
        if (!mrpw::load_world(assets, l, *w)) continue;
        idx.add(w.get());
        worlds.push_back(std::move(w));
    }
    if (worlds.empty()) {
        std::printf("batch 108: no assets in %s (./setup.sh: src/setup/mrp_export.cpp)\n", assets.c_str());
        ++g_fail;
        return;
    }
    const char* dir_env = std::getenv("ORACLE_VU1_DIR");
    const std::string dirs = dir_env != nullptr ? std::string(dir_env) : dump_path("vu1_world");
    std::vector<std::string> files;
    for (size_t at = 0; at <= dirs.size();) {
        size_t end = dirs.find(':', at);
        if (end == std::string::npos) end = dirs.size();
        const std::string d = dirs.substr(at, end - at);
        at = end + 1;
        if (std::filesystem::is_directory(d))
            for (const auto& de : std::filesystem::recursive_directory_iterator(d))
                if (de.path().extension() == ".bin" && de.path().filename().string().rfind("vu1w_", 0) == 0) files.push_back(de.path().string());
    }
    std::sort(files.begin(), files.end());
    if (files.empty()) {  // no captures recorded on this machine: skipped, not failed
        std::printf("batch 108: skipped (no captured VU1 states in %s: batch 104's header)\n", dirs.c_str());
        ++g_skipped;
        return;
    }
    const int variants = std::getenv("ORACLE_MRPW_VARIANTS") != nullptr ? std::atoi(std::getenv("ORACLE_MRPW_VARIANTS")) : 3;
    std::mt19937 rng(std::getenv("ORACLE_SEED") != nullptr ? static_cast<uint32_t>(std::atoi(std::getenv("ORACLE_SEED"))) : 108u);
    mrpw::CheckStats captured, perturbed;
    size_t states = 0;
    std::vector<uint64_t> micro;
    auto st = std::make_unique<VUState>();
    auto pst = std::make_unique<VUState>();
    const mrpw::TfragCounts before = mrpw::g_tfrag_counts;
    for (const std::string& path : files) {
        uint32_t start = 0;
        if (!read_state(path, *st, micro, start)) continue;
        const mrpw::Program prog = mrpw::program_of(micro.data());
        if (prog == mrpw::Program::None) continue;
        ++states;
        mrpw::check_start(idx, *st, micro.data(), start, captured);
        for (int v = 0; v < variants; ++v) {
            *pst = *st;
            perturb(*pst, prog, rng);
            mrpw::check_start(idx, *pst, micro.data(), start, perturbed);
        }
    }
    const mrpw::TfragCounts& c = mrpw::g_tfrag_counts;
    std::printf("batch 108: %zu states of the world programs (%zu files), %zu levels' assets\n", states, files.size(), worlds.size());
    std::printf("-- as captured\n%s", captured.summary().c_str());
    std::printf("-- perturbed (%d per state)\n%s", variants, perturbed.summary().c_str());
    std::printf("tfrag reference: %llu vertices sent, %llu morphed, %llu took their parent's vertex info, %llu list entries fixed, %llu with ADC\n",
                static_cast<unsigned long long>(c.vertices - before.vertices), static_cast<unsigned long long>(c.morphed - before.morphed),
                static_cast<unsigned long long>(c.substituted - before.substituted), static_cast<unsigned long long>(c.fixed - before.fixed),
                static_cast<unsigned long long>(c.adc - before.adc));
    int bad = 0;
    for (const mrpw::CheckStats* s : {&captured, &perturbed}) {
        for (const auto& n : s->notes) std::printf("  %s\n", n.c_str());
        for (const auto& k : s->kind) bad += static_cast<int>(k.unknown + k.kick_count_differ * 0 + k.differ + (k.tri_ref - k.tri_found));
    }
    // (a kick count that differs is a difference only for the no-clip programs: the clipping programs kick their pieces apart)
    for (int p : {0, 2, 4})
        bad += static_cast<int>(captured.kind[p].kick_count_differ + perturbed.kind[p].kick_count_differ);
    if (states == 0) {
        std::printf("batch 108: no captured states in %s\n", dirs.c_str());
        ++bad;
    }
    std::printf("batch 108: %s\n", bad == 0 ? "ok" : "DIFFERENCES");
    if (bad != 0) ++g_fail;
}

}  // namespace rac1::oracle
