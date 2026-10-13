// Oracle batch 128 (task 0369): Novalis's scrolling surfaces and its water (novalis_water.cpp / novalis_water_asm.cpp), Novalis-only
// code: run with ORACLE_LEVEL=1 build/tests/ovl_oracle --batch 128 (needs the ISO); on other levels every check is skipped.
// On Novalis's own data after its first gameplay frame: the strip records (0x1e2fc0 x 8, 0x1e3380 x 2, 0x1fbc80 x 4, 0x202d80 x 3) and
// the 21 water tiles (0x1e34c0, 0x1190 bytes each) with their scroll / phase / height / texture fields varied, the camera moved about
// them (so that some are seen, some clipped, some culled), the scratchpad lists filled at random for the packet builders alone.

#include "ovl_oracle.h"

#include <game/ovl/novalis_water.h>
#include <game/ovl/ovl_reloc.h>
#include <game/ovl/variants_l1.h>

#include <runtime/vu/vu0.h>

#include <cmath>
#include <cstring>

namespace rac1::oracle {

namespace {
// A check whose original is Novalis's own function at `l1addr`.
void l1check(const char* name, uint32_t l1addr, int n, Ret ret, const std::function<Args()>& gen, const std::function<uint64_t(const Args&)>& port) {
    if (g_only_batch != 0 && g_only_batch != g_batch) return;
    if (const char* only = std::getenv("ORACLE_ONLY"); only != nullptr && std::strcmp(only, name) != 0) return;
    if (g_oracle_level != 1) {
        std::printf("%-26s L1:0x%08x  skipped (Novalis only: run with ORACLE_LEVEL=1)\n", name, l1addr);
        ++g_skipped;
        return;
    }
    std::printf("%-26s L1:0x%08x\n", name, l1addr);
    if (!check_fn(name, l1addr, n, ret, gen, port, Scope::FullRam, 0, 0)) ++g_fail;
}

// The VU0 the original started from (the interpreter's: its registers and data memory), given to the port before it runs: the two
// keep separate VU0s, and the routines read registers the micro programs 0x190 / 0x192 leave (as batch 89's moby draw)
VUState g_vu0_in;
void vu0_sync() {
    VUState& v = vu0::global().st;
    v.vf = g_vu0_in.vf;
    v.vi = g_vu0_in.vi;
    v.acc = g_vu0_in.acc;
    v.q = g_vu0_in.q;
    v.i = g_vu0_in.i;
    v.r = g_vu0_in.r;
    v.p = g_vu0_in.p;
    v.macflag = g_vu0_in.macflag;
    v.statusflag = g_vu0_in.statusflag;
    v.clipflag = g_vu0_in.clipflag;
    std::memcpy(v.mem.data(), g_vu0_in.mem.data(), 0x1000);
}

constexpr uint32_t SPR = 0x70000000u;
constexpr OvConst<0x00166dc0u> CAM{};  // the camera's position (Novalis's 0x167240; the view matrices 0x167140 / 0x1671c0)
constexpr uint32_t TILES = ORACLE_LADDR(1, 0x001e34c0u);
constexpr uint32_t TILE_SIZE = 0x1190;
struct Set {
    uint32_t recs;
    int32_t n;
};
constexpr Set SETS[] = {{ORACLE_LADDR(1, 0x001e2fc0u), 8}, {ORACLE_LADDR(1, 0x001e3380u), 2}, {ORACLE_LADDR(1, 0x001fbc80u), 4}, {ORACLE_LADDR(1, 0x00202d80u), 3}};

// the camera moved near a point (or left where it is), its view rows turned a little
void move_camera(const Vec4& at, float spread) {
    if (ri(0, 3) == 0) return;
    Vec4& c = VV(CAM);
    c.x = at.x + rf(-spread, spread);
    c.y = at.y + rf(-spread, spread);
    c.z = at.z + rf(-spread / 4, spread / 4);
    if (ri(0, 3) == 0) {  // its rotation turned (the camera's rows 0x166fd0..: a rotation about z)
        const float a = rf(-3.14f, 3.14f), cs = std::cos(a), sn = std::sin(a);
        for (uint32_t r = OV(0x00166fd0u); r < OV(0x00166fd0u) + 0x30; r += 0x10) {
            Vec4& v = VV(r);
            const float x = v.x, y = v.y;
            v.x = cs * x - sn * y;
            v.y = sn * x + cs * y;
        }
    }
    render_view_setup();  // (the frame's view matrices from it, as the render frame makes them)
}

// the point at `p` moved about the camera until the port's screen test (0x2002e0, the copy the originals call) sees it with radius r
void into_view(uint32_t p, float r, float spread) {
    const Vec4 c = VV(CAM);
    for (int k = 0; k < 200; ++k) {
        Vec4& v = VV(p);
        v.x = c.x + rf(-spread, spread);
        v.y = c.y + rf(-spread, spread);
        v.z = c.z + rf(-spread / 3, spread / 3);
        const int64_t pr = point_on_screen(p, r);
        if (std::getenv("DBG369")) std::printf("into_view %d: %lld cam %g %g %g\n", k, static_cast<long long>(pr), c.x, c.y, c.z);
        if (pr != -1) return;
    }
}

// a strip record's moving fields varied
void vary_strip(uint32_t r) {
    if (coin()) wfrand(r + 0x40, 4, -1.2f, 1.2f);  // the uv scroll
    if (coin()) wfrand(r + 0x10, 2, -2.0f, 2.0f);  // the scroll rates' scales
    if (coin()) wfrand(r + 0x20, 4, -0.2f, 0.2f);  // the scroll rates
    if (coin()) w32(r + 0x50, R());                 // the phase
    if (coin()) w32(r + 0x1c, R() & 0x3ffffu);      // its step
    if (coin()) wf(r + 0x5c, rf(0.0f, 1.5f));        // the wave height
    if (ri(0, 3) == 0) w32(r + 0x30, R());           // the colour
    if (ri(0, 3) == 0) w8(r + 0x3c, R());            // the alphas
    if (ri(0, 3) == 0) w8(r + 0x3d, R());
}

// a water tile's moving fields varied
void vary_tile(uint32_t t) {
    if (coin()) wfrand(t + 0x20, 2, -1.2f, 1.2f);   // the uv scroll
    if (coin()) wfrand(t + 0x28, 2, -0.2f, 0.2f);   // its rates
    if (coin()) wfrand(t + 0x30, 2, -7.0f, 7.0f);   // the swell's angles
    if (coin()) wfrand(t + 0x38, 2, -0.3f, 0.3f);   // their rates
    if (coin()) wf(t + 0x40, rf(0.0f, 0.2f));        // the swell's size
    if (ri(0, 3) == 0) w16(t + 0x1e, R());           // the cells drawn
    if (ri(0, 3) == 0) w32(t + 0x44, R() & 0xfu);    // the flat edges
    if (ri(0, 4) == 0) w8(t + 0x1c, R());            // the alphas
    if (ri(0, 4) == 0) w8(t + 0x1d, R());
    if (ri(0, 3) == 0) wfrand(t + 0x50, 3 * 0x5c0 / 4, -0.6f, 0.6f);  // the three frames' heights (and their borders)
}
}  // namespace

void batch_128() {
    if (g_only_batch != 128) return;
    g_batch = 128;
    const std::vector<uint32_t> saved_ignored = g_ignored_words;
    g_ignored_words = vcross_lanes();
    const bool null_ram = g_ee_null_is_ram;
    g_ee_null_is_ram = true;  // as in the runtime

    static std::vector<uint8_t> snap;
    static int snap_level = -1;
    auto level = [] {
        uint8_t* ram = ee_get_ram_base();
        if (snap.empty() || snap_level != g_oracle_level) {
            veldin_boot();
            level_setup_body(g_oracle_level == 0 ? 1 : g_oracle_level, 0);
            w32(OV(0x0016c060), 0xf);
            gameplay_frame();
            render_view_setup();  // (the view matrices of the render frame: zero after an update alone)
            snap.assign(ram, ram + 0x02000000);
            snap_level = g_oracle_level;
        } else {
            std::memcpy(ram, snap.data(), snap.size());
        }
    };
    if (g_oracle_level != 1) {  // (no snapshot: every check below is skipped)
        l1check("nov_strip_verts", 0x00262618, 0, Ret::None, [] { return Args{}; }, [](A) { return uint64_t{0}; });
        g_ee_null_is_ram = null_ram;
        g_ignored_words = saved_ignored;
        return;
    }
    level();  // (the snapshot made before the first check: as batch 126)
    // the port's VU0 micro memory as the game leaves it (the moby pass sends these chains every frame: the sin / cos programs 0x190 /
    // 0x192 the strips call are in 0x10e4c0's); the interpreter loads the same MPGs itself
    vu0::vif0_send_chain(0x00100080, true);
    vu0::vif0_send_chain(0x0010e4c0, true);
    g_pre_call = [](EeInterpreter& ee) {
        const VUState& o = ee.vu0();
        g_vu0_in.vf = o.vf;
        g_vu0_in.vi = o.vi;
        g_vu0_in.acc = o.acc;
        g_vu0_in.q = o.q;
        g_vu0_in.i = o.i;
        g_vu0_in.r = o.r;
        g_vu0_in.p = o.p;
        g_vu0_in.macflag = o.macflag;
        g_vu0_in.statusflag = o.statusflag;
        g_vu0_in.clipflag = o.clipflag;
        std::memcpy(g_vu0_in.mem.data(), o.mem.data(), 0x1000);
    };
    // a strip record of one of the sets (the camera near its sphere), varied
    auto strip = [level](const Set*& set) -> uint32_t {
        level();
        set = &SETS[ri(0, 3)];
        const uint32_t r = set->recs + u(ri(0, set->n - 1)) * 0x60;
        for (int32_t i = 0; i < set->n; ++i) vary_strip(set->recs + u(i) * 0x60);
        if (ri(0, 2) != 0) {  // some of the set's spheres brought into view (seen, or clipped near the edges)
            for (int32_t i = 0; i < set->n; ++i)
                if (coin()) into_view(ee_read<uint32_t>(set->recs + u(i) * 0x60 + 8), coin() ? 400.0f : 0.0f, coin() ? 150.0f : 600.0f);
        } else {
            move_camera(VV(ee_read<uint32_t>(r + 8)), 300.0f);
        }
        return r;
    };
    auto tile_at = [level](int32_t& k) -> uint32_t {
        level();
        k = ri(0, 20);
        const uint32_t t = TILES + u(k) * TILE_SIZE;
        for (int32_t i = 0; i < 21; ++i) vary_tile(TILES + u(i) * TILE_SIZE);
        if (ri(0, 2) != 0) {  // tiles brought into view (whole, or partly: clipped cells)
            for (int32_t i = 0; i < 21; ++i)
                if (i == k || ri(0, 3) == 0) into_view(TILES + u(i) * TILE_SIZE, coin() ? 0.0f : 30.0f, coin() ? 40.0f : 120.0f);
        } else {
            move_camera(VV(t), 120.0f);
        }
        if (ri(0, 3) == 0) w32(L1_ADDR(0x00161220u), u(ri(0, 2)));  // the grid frame
        return t;
    };

    // ---- the scrolling strips
    l1check("nov_strip_verts", 0x00262618, 200, Ret::None, [strip] {
        const Set* s = nullptr;
        return Args{{strip(s)}, {}};
    }, [](A a) { vu0_sync(); nov_strip_verts_l1(a.i[0]); return uint64_t{0}; });
    l1check("nov_prim_list2", 0x0021fa98, 200, Ret::None, [level] {
        level();
        const int32_t n = ri(1, 120);
        wrand(SPR, n * 3);
        wrand(SPR + 0x1000, n * 2);
        wrand(SPR + 0x2000, n * 2);
        wrand(SPR + 0x3000, n);
        return Args{{u(n), SPR, SPR + 0x3000, SPR + 0x1000, SPR + 0x2000, u(ri(0, 1))}, {}};
    }, [](A a) { vu0_sync(); nov_prim_list2_l1(static_cast<int32_t>(a.i[0]), a.i[1], a.i[2], a.i[3], a.i[4], a.i[5]); return uint64_t{0}; });
    l1check("nov_strips_vu7_setup", 0x002b96c0, 60, Ret::None, [level] {
        level();
        move_camera(VV(CAM), 200.0f);
        if (coin()) w32(L1_ADDR(0x0015f5e0u), u(ri(0, 8)));
        return Args{};
    }, [](A) { vu0_sync(); nov_strips_vu7_setup_l1(); return uint64_t{0}; });
    l1check("nov_strips_draw", 0x002b96e0, 200, Ret::None, [strip] {
        const Set* s = nullptr;
        strip(s);
        return Args{{u(s->n), s->recs}, {}};
    }, [](A a) { vu0_sync(); nov_strips_draw_l1(static_cast<int32_t>(a.i[0]), a.i[1]); return uint64_t{0}; });
    {
        struct Hook {
            const char* name;
            uint32_t addr;
            void (*fn)(uint32_t);
            int set;
        };
        const Hook hooks[] = {{"nov_strips_676_draw", 0x002f60f8, nov_strips_676_draw_l1, 0}, {"nov_strips_678_draw", 0x002f6150, nov_strips_678_draw_l1, 1},
                              {"nov_strips_761_draw", 0x002feb28, nov_strips_761_draw_l1, 2}, {"nov_strips_1225_draw", 0x00309bf8, nov_strips_1225_draw_l1, 3}};
        for (const Hook& h : hooks) {
            l1check(h.name, h.addr, 80, Ret::None, [level, h] {
                level();
                const Set& s = SETS[h.set];
                for (int32_t i = 0; i < s.n; ++i) vary_strip(s.recs + u(i) * 0x60);
                for (int32_t i = 0; i < s.n; ++i)
                    if (coin()) into_view(ee_read<uint32_t>(s.recs + u(i) * 0x60 + 8), coin() ? 400.0f : 0.0f, 300.0f);
                if (ri(0, 2) == 0) w32(L1_ADDR(0x0015f5ccu), R());  // the frame counter (class 1225's wave)
                return Args{{M}, {}};
            }, [h](A a) { vu0_sync(); h.fn(a.i[0]); return uint64_t{0}; });
        }
    }

    // ---- the water
    l1check("nov_water_vu7_setup", 0x002b8f98, 60, Ret::None, [level] {
        level();
        move_camera(VV(CAM), 200.0f);
        if (coin()) w32(L1_ADDR(0x0015f5e0u), u(ri(0, 8)));
        return Args{};
    }, [](A) { vu0_sync(); nov_water_vu7_setup_l1(); return uint64_t{0}; });
    l1check("nov_water_grid_in", 0x00261c10, 100, Ret::None, [tile_at] {
        int32_t k = 0;
        const uint32_t t = tile_at(k);
        return Args{{SPR, t + 0x50 + u(ri(0, 2)) * 0x5c0}, {}};
    }, [](A a) { vu0_sync(); nov_water_grid_in_l1(a.i[0], a.i[1]); return uint64_t{0}; });
    l1check("nov_water_grid_light", 0x00261df8, 150, Ret::None, [tile_at] {
        int32_t k = 0;
        const uint32_t t = tile_at(k);
        if (coin()) {
            nov_water_grid_in_l1(SPR, t + 0x50 + u(ri(0, 2)) * 0x5c0);  // (the port's grid, as the original's caller leaves it)
        } else {
            for (uint32_t a = SPR; a < SPR + 0x780; a += 4) wf(a, rf(-2.0f, 2.0f));
        }
        if (ri(0, 3) == 0) wfrand(L1_ADDR(0x00167100u), 8, -1.0f, 1.0f);  // the light
        return Args{{t}, {}};
    }, [](A a) { vu0_sync(); nov_water_grid_light_l1(a.i[0]); return uint64_t{0}; });
    l1check("nov_water_grid_xyz", 0x002b8c08, 100, Ret::None, [tile_at] {
        int32_t k = 0;
        const uint32_t t = tile_at(k);
        for (uint32_t a = SPR; a < SPR + 0x780; a += 4) wf(a, rf(-2.0f, 2.0f));
        return Args{{t}, {}};
    }, [](A a) { vu0_sync(); nov_water_grid_xyz_l1(a.i[0]); return uint64_t{0}; });
    l1check("nov_water_uv_scroll", 0x002b80d0, 150, Ret::None, [tile_at] {
        int32_t k = 0;
        return Args{{tile_at(k)}, {}};
    }, [](A a) { vu0_sync(); nov_water_uv_scroll_l1(a.i[0]); return uint64_t{0}; });
    l1check("nov_water_cell_packet", 0x00262388, 150, Ret::None, [level] {
        level();
        wrand(SPR, 0x3000 / 4);
        return Args{{coin() ? 0xcu : 0xeu, u(ri(0, 15))}, {}};
    }, [](A a) { vu0_sync(); nov_water_cell_packet_l1(a.i[0], static_cast<int32_t>(a.i[1])); return uint64_t{0}; });
    l1check("nov_water_cell", 0x002b8cb0, 200, Ret::None, [tile_at] {
        int32_t k = 0;
        const uint32_t t = tile_at(k);
        wrand(SPR, 0x3000 / 4);
        return Args{{t, u(ri(0, 15)), u(ri(0, 2))}, {}};
    }, [](A a) { vu0_sync(); nov_water_cell_l1(a.i[0], static_cast<int32_t>(a.i[1]), static_cast<int32_t>(a.i[2])); return uint64_t{0}; });
    l1check("nov_water_draw", 0x002b91c8, 60, Ret::None, [tile_at] {
        int32_t k = 0;
        tile_at(k);
        const int32_t first = ri(0, 3) == 0 ? ri(0, 18) : 0;
        return Args{{TILES + u(first) * TILE_SIZE, u(ri(0, 3) == 0 ? ri(1, 21 - first) : 21 - first)}, {}};
    }, [](A a) { vu0_sync(); nov_water_draw_l1(a.i[0], static_cast<int32_t>(a.i[1])); return uint64_t{0}; });
    l1check("nov_water_751_draw", 0x002fd0c0, 40, Ret::None, [tile_at] {
        int32_t k = 0;
        tile_at(k);
        return Args{{M}, {}};
    }, [](A a) { vu0_sync(); nov_water_751_draw_l1(a.i[0]); return uint64_t{0}; });

    g_pre_call = nullptr;
    g_ee_null_is_ram = null_ram;
    g_ignored_words = saved_ignored;
}

}  // namespace rac1::oracle
