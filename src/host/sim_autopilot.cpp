// (testing) RAC1_AUTOPILOT: the left stick steered toward a target (see sim_autopilot.h).
#include "sim_autopilot.h"

#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace rac1 {

namespace {

struct Pilot {
    bool on = false;
    uint64_t first = 0, last = 0;
    int cls = -1, nth = 0;
    struct Wp {
        float x, y;
        bool jump;
    };
    std::vector<Wp> wp;
    size_t next = 0;
    float px = 0, py = 0, heading = 0, tx = 0, ty = 0, mx = 0, my = 0;
    bool have_prev = false, have_target = false, coast = false;
    // go: the waypoints walked in order and the stick let go at the last, camera-relative (a walking hero: Clank): the
    // view's heading c from its position 0x166dc0 to the hero; motion angle = c - stick angle, stick angle = atan2(lx, -ly)
    bool go = false, cam_known = false, done = false;
    float cam_c = 0;
};

// orig: (host) the RAC1_AUTOPILOT setting parsed
Pilot parse(const char* s) {
    Pilot p;
    if (s == nullptr) return p;
    char* e = nullptr;
    p.first = std::strtoull(s, &e, 10);
    if (*e != '-') return p;
    p.last = std::strtoull(e + 1, &e, 10);
    if (*e != ':') return p;
    ++e;
    if (std::strncmp(e, "class:", 6) == 0) {
        p.cls = std::atoi(e + 6);
        if (const char* c = std::strchr(e + 6, ':')) p.nth = std::atoi(c + 1);
        p.on = true;
    } else if (std::strncmp(e, "wp:", 3) == 0 || std::strncmp(e, "go:", 3) == 0) {
        p.go = e[0] == 'g';
        for (const char* q = e + 3; *q != 0;) {
            char* z = nullptr;
            const float x = std::strtof(q, &z);
            if (*z != ',') break;
            const float y = std::strtof(z + 1, &z);
            bool j = false;
            if (*z == ',') j = std::strtol(z + 1, &z, 10) != 0;
            p.wp.push_back({x, y, j});
            q = *z == '/' ? z + 1 : z;
            if (*z == 0) break;
        }
        p.on = !p.wp.empty();
    }
    return p;
}

// orig: (host) the n-th live moby of class c (0 when none)
uint32_t nth_moby(int c, int n) {
    for (uint32_t m = static_cast<uint32_t>(lw(OV(0x0015ffd8))); m < static_cast<uint32_t>(lw(OV(0x0015ffe0))); m += 0x100) {
        if ((static_cast<uint32_t>(lw(m + 0x20)) & 0xff) >= 0xfd || lh(m + 0xa6) != c) continue;
        if (n-- == 0) return m;
    }
    return 0;
}

}  // namespace

// orig: (host) the autopilot's frame (see sim_autopilot.h)
void sim_autopilot(uint64_t frame, uint16_t& held, float& lx, float& ly) {
    // several pilots, '|' between them (each its own frames, e.g. a moby followed, then another)
    static std::vector<Pilot> pilots = [] {
        std::vector<Pilot> v;
        for (const char* s = std::getenv("RAC1_AUTOPILOT"); s != nullptr;) {
            v.push_back(parse(s));
            s = std::strchr(s, '|');
            if (s != nullptr) ++s;
        }
        return v;
    }();
    static const int trace = std::getenv("RAC1_AUTOPILOT_TRACE") != nullptr ? std::atoi(std::getenv("RAC1_AUTOPILOT_TRACE")) : 0;
    static const int log_cls = std::getenv("RAC1_AUTOPILOT_LOG") != nullptr ? std::atoi(std::getenv("RAC1_AUTOPILOT_LOG")) : -1;
    const uint32_t hm = static_cast<uint32_t>(lw(0x001413d0));
    if (log_cls >= 0 && frame % 10 == 0)
        for (int k = 0; k < 8; ++k)
            if (const uint32_t m = nth_moby(log_cls, k))
                std::printf("[autopilot-log] frame %llu class %d #%d %.1f %.1f %.1f\n", static_cast<unsigned long long>(frame), log_cls, k, lwc1(m + 0x10), lwc1(m + 0x14), lwc1(m + 0x18));
    Pilot* pp = nullptr;
    for (Pilot& q : pilots)
        if (q.on && frame >= q.first && frame < q.last) pp = &q;
    if (pp == nullptr || hm == 0) return;
    Pilot& p = *pp;
    const float hx = lwc1(hm + 0x10), hy = lwc1(hm + 0x14);
    bool respawned = false;
    if (p.have_prev) {
        const float dx = hx - p.px, dy = hy - p.py;
        if (dx * dx + dy * dy > 0.0004f) p.heading = std::atan2(dy, dx);
        respawned = dx * dx + dy * dy > 100.0f;  // (a jump of 10 units in a frame: put back at a checkpoint)
    }
    p.px = hx;
    p.py = hy;
    p.have_prev = true;
    float ax = 0, ay = 0;
    if (p.go) {
        const float cx = lwc1(OV(0x00166dc0)), cy = lwc1(OV(0x00166dc4));  // (the view's position: the camera looks from there at the hero)
        p.cam_c = std::atan2(hy - cy, hx - cx);
        p.cam_known = (hx - cx) * (hx - cx) + (hy - cy) * (hy - cy) > 0.01f;
        while (!p.done) {
            const Pilot::Wp& w = p.wp[p.next];
            if ((w.x - hx) * (w.x - hx) + (w.y - hy) * (w.y - hy) > 0.36f) break;
            if (++p.next == p.wp.size()) p.done = true;
        }
        if (p.done) {
            lx = ly = 0.0f;
            return;
        }
        float a = 0.0f;  // (ahead until the camera is known)
        if (p.cam_known) a = p.cam_c - std::atan2(p.wp[p.next].y - hy, p.wp[p.next].x - hx);
        lx = std::sin(a);
        ly = -std::cos(a);
        if (trace > 0 && frame % static_cast<uint64_t>(trace) == 0)
            std::printf("[autopilot] frame %llu hero %.1f %.1f go wp %zu (%.1f %.1f) cam %.2f lx %.2f ly %.2f\n", static_cast<unsigned long long>(frame), hx, hy, p.next,
                        p.wp[p.next].x, p.wp[p.next].y, p.cam_c, lx, ly);
        return;
    }
    if (p.cls >= 0) {
        const uint32_t m = nth_moby(p.cls, p.nth);
        if (m == 0) return;
        const float x = lwc1(m + 0x10), y = lwc1(m + 0x14);
        if (p.have_target) {
            p.mx = x - p.tx;
            p.my = y - p.ty;
        }
        p.tx = x;
        p.ty = y;
        p.have_target = true;
        ax = x + p.mx * 15.0f;
        ay = y + p.my * 15.0f;
    } else {
        // pure pursuit along the waypoints' line: the nearest point of the segments near the current one, then 6 units further on
        const size_t n = p.wp.size();
        float best = 1e30f, bt = 0;
        size_t bk = p.next;
        for (size_t i = 0; i < (respawned ? n : 12); ++i) {
            const size_t k = (p.next + n - (respawned ? 0 : 2) + i) % n;
            const Pilot::Wp& a = p.wp[k];
            const Pilot::Wp& b = p.wp[(k + 1) % n];
            const float sx = b.x - a.x, sy = b.y - a.y, l2 = sx * sx + sy * sy;
            const float t = l2 > 0 ? std::fmax(0.0f, std::fmin(1.0f, ((hx - a.x) * sx + (hy - a.y) * sy) / l2)) : 0.0f;
            const float qx = a.x + sx * t - hx, qy = a.y + sy * t - hy;
            if (qx * qx + qy * qy < best) {
                best = qx * qx + qy * qy;
                bk = k;
                bt = t;
            }
        }
        for (size_t k = p.next; k != bk; k = (k + 1) % n)  // (cross: the jumps marked at the waypoints passed)
            if (p.wp[k].jump) held |= 0x0040;
        p.next = bk;
        if (respawned) p.coast = true;  // (after a respawn: no steering until the hero meets the line, it may ride another route)
        if (best < 9.0f) p.coast = false;
        if (p.coast) {
            lx = 0.0f;
            ly = -1.0f;
            return;
        }
        float left = 6.0f;
        size_t k = bk;
        float x = p.wp[k].x + (p.wp[(k + 1) % n].x - p.wp[k].x) * bt, y = p.wp[k].y + (p.wp[(k + 1) % n].y - p.wp[k].y) * bt;
        for (int g = 0; g < 8 && left > 0; ++g) {
            const Pilot::Wp& b = p.wp[(k + 1) % n];
            const float dx = b.x - x, dy = b.y - y, d = std::sqrt(dx * dx + dy * dy);
            if (d >= left) {
                x += dx * left / d;
                y += dy * left / d;
                left = 0;
            } else {
                x = b.x;
                y = b.y;
                left -= d;
                k = (k + 1) % n;
            }
        }
        ax = x;
        ay = y;
    }
    float err = std::atan2(ay - hy, ax - hx) - p.heading;
    while (err > 3.14159265f) err -= 6.2831853f;
    while (err < -3.14159265f) err += 6.2831853f;
    lx = std::fmax(-1.0f, std::fmin(1.0f, -err * 3.0f));
    ly = -1.0f;
    if (trace > 0 && frame % static_cast<uint64_t>(trace) == 0)
        std::printf("[autopilot] frame %llu hero %.1f %.1f heading %.2f aim %.1f %.1f (wp %zu) err %.2f lx %.2f\n", static_cast<unsigned long long>(frame), hx, hy, p.heading, ax, ay,
                    p.next, err, lx);
}

}  // namespace rac1
