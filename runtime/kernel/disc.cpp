#include "disc.h"

#include "ee_memory.h"

#include <platform/platform.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace rac1 {

namespace {
std::FILE* g_iso = nullptr;
}

std::string disc_path() {
    const char* env = std::getenv("RAC1_ISO");
    if (env != nullptr) return env;
    // the setup's link to the player's image, one name per region (the PAL disc's when it is the one
    // there); where links cannot be made (Windows without the privilege), the image's path written in a
    // text file beside it instead
    const std::string link = "assets/iso/rac1-ntsc-u.iso", pal_link = "assets/iso/rac1-pal.iso";
    std::error_code ec;
    if (std::filesystem::exists(link, ec)) return link;
    if (std::filesystem::exists(pal_link, ec)) return pal_link;
    std::ifstream f(disc_path_file());
    std::string p;
    if (f && std::getline(f, p)) {
        while (!p.empty() && (p.back() == '\r' || p.back() == '\n')) p.pop_back();
        if (!p.empty()) return p;
    }
    return link;
}

const char* disc_path_file() { return "assets/iso/disc-path.txt"; }

namespace {
// SYSTEM.CNF's BOOT2 file name, read through the ISO9660 root directory ("" when it cannot be read)
std::string read_boot2() {
    std::FILE* f = std::fopen(disc_path().c_str(), "rb");
    if (f == nullptr) return "";
    auto rd = [f](uint32_t off, uint32_t n) {
        std::vector<uint8_t> b(n);
        if (std::fseek(f, static_cast<long>(off), SEEK_SET) != 0 || std::fread(b.data(), 1, n, f) != n) b.clear();
        return b;
    };
    std::string out;
    const std::vector<uint8_t> pvd = rd(16 * 2048, 2048);
    if (pvd.size() == 2048 && std::memcmp(&pvd[1], "CD001", 5) == 0) {
        uint32_t lba = 0, size = 0;
        std::memcpy(&lba, &pvd[156 + 2], 4);
        std::memcpy(&size, &pvd[156 + 10], 4);
        const std::vector<uint8_t> dir = size < 0x10000 ? rd(lba * 2048, size) : std::vector<uint8_t>{};
        for (size_t o = 0; o < dir.size();) {
            const uint8_t len = dir[o];
            if (len == 0) {
                o = (o / 2048 + 1) * 2048;
                continue;
            }
            if (o + len > dir.size() || len < 34) break;
            const std::string name(reinterpret_cast<const char*>(&dir[o + 33]), dir[o + 32]);
            if (name.rfind("SYSTEM.CNF", 0) == 0) {
                uint32_t flba = 0, fsize = 0;
                std::memcpy(&flba, &dir[o + 2], 4);
                std::memcpy(&fsize, &dir[o + 10], 4);
                const std::vector<uint8_t> cnf = fsize < 2048 ? rd(flba * 2048, fsize) : std::vector<uint8_t>{};
                const std::string txt(cnf.begin(), cnf.end());
                const size_t b = txt.find("cdrom0:\\");
                if (b != std::string::npos) {
                    const size_t e = txt.find_first_of(";\r\n", b + 8);
                    out = txt.substr(b + 8, e == std::string::npos ? std::string::npos : e - (b + 8));
                }
                break;
            }
            o += len;
        }
    }
    std::fclose(f);
    return out;
}
}  // namespace

const std::string& disc_boot_elf() {
    static const std::string name = [] {
        const std::string b = read_boot2();
        if (b.empty()) return std::string("SCUS_971.99");
        if (b != "SCUS_971.99") std::fprintf(stderr, "[disc] %s boots %s: another region's build\n", disc_path().c_str(), b.c_str());
        return b;
    }();
    return name;
}

std::string disc_boot_elf_path() { return "assets/dumps/" + disc_boot_elf(); }

bool disc_other_region() { return disc_boot_elf() != "SCUS_971.99"; }

bool disc_open(const std::string& path) {
    if (g_iso != nullptr) return true;
    std::string p = path;
    if (p.empty()) p = disc_path();
    g_iso = std::fopen(p.c_str(), "rb");
    return g_iso != nullptr;
}

bool disc_is_open() { return g_iso != nullptr; }

// a positional read (pread; ReadFile at an offset on Windows), no shared position: the game's thread and the host's
// audio thread (the IOP mirror's stream reader, ADR 0013) read at the same time
bool disc_read(uint32_t lba, uint32_t sectors, void* dst) {
    if (g_iso == nullptr && !disc_open()) return false;
    const size_t n = static_cast<size_t>(sectors) * 2048;
    size_t got = 0;
    while (got < n) {
        const int64_t r = platform::read_at(g_iso, static_cast<uint64_t>(lba) * 2048 + got, static_cast<char*>(dst) + got, n - got);
        if (r <= 0) return false;
        got += static_cast<size_t>(r);
    }
    return true;
}

void disc_will_need(uint32_t lba, uint32_t sectors) {
    if (g_iso == nullptr && !disc_open()) return;
    platform::will_need(g_iso, static_cast<uint64_t>(lba) * 2048, static_cast<uint64_t>(sectors) * 2048);
}

bool disc_read_ee(uint32_t ee_dst, uint32_t lba, uint32_t sectors) {
    std::vector<uint8_t> buf(static_cast<size_t>(sectors) * 2048);
    if (!disc_read(lba, sectors, buf.data())) return false;
    for (size_t i = 0; i < buf.size(); ++i) ee_write<uint8_t>(ee_dst + static_cast<uint32_t>(i), buf[i]);
    return true;
}

}  // namespace rac1
