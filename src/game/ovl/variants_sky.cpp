// See variants_sky.h. The per-level table of the sky: both sides are literal addresses (the level's own dispatcher, Veldin's variant it calls), like
// variants_l1.cpp.
#include <game/ovl/variants_sky.h>

#include <game/ovl/sky_ee.h>

namespace rac1 {

namespace {
constexpr SkyLevel kSky[] = {
    {1, 0x00252570, ovl_289420_tr},   // 0x289420
    {2, 0x00240048, ovl_2895a0_tr},   // 0x2895a0
    {3, 0x0022c6a8, ovl_2899a8_tr},   // 0x2899a8
    {4, 0x00230f48, ovl_289a90_tr},   // 0x289a90
    {5, 0x00267520, ovl_289bd8_tr},   // 0x289bd8
    {6, 0x0024d218, ovl_289cb8_tr},   // 0x289cb8
    {7, 0x00265b58, ovl_28a208_tr},   // 0x28a208
    {8, 0x00248148, ovl_28a2f0_tr},   // 0x28a2f0
    {9, 0x0025d648, ovl_28a3a8_tr},   // 0x28a3a8
    {10, 0x00230fd0, ovl_28a480_tr},  // 0x28a480
    {11, 0x00263908, ovl_28ab08_tr},  // 0x28ab08
    {12, 0x00256748, ovl_28ab08_tr},  // 0x28ab08
    {13, 0x002497d8, ovl_28a920_tr},  // 0x28a920
    {14, 0x00245940, ovl_28a998_tr},  // 0x28a998
    {15, 0x0022dad0, ovl_28aa98_tr},  // 0x28aa98
    {16, 0x00239c30, ovl_28ab08_tr},  // 0x28ab08
    {17, 0x00238c50, ovl_28ac88_tr},  // 0x28ac88
    {18, 0x00240840, ovl_28b080_tr},  // 0x28b080
};
}  // namespace

// orig: (host) see variants_sky.h
const SkyLevel* sky_level(int level) {
    for (const SkyLevel& s : kSky)
        if (s.level == level) return &s;
    return nullptr;
}

// orig: (host) see variants_sky.h
bool sky_dispatcher_row(int level, VariantRow* row) {
    const SkyLevel* s = sky_level(level);
    if (s == nullptr) return false;
    *row = VariantRow{0x0023d968, s->dispatcher, 0x7c};
    return true;
}

}  // namespace rac1
