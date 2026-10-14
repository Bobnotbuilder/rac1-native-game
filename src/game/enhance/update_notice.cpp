// The update notice (see update_notice.h). Not a port: everything here is new, written on top of the
// original's menu data so that the panel looks and behaves like the game's own pages.
//
// The page is a copy of the pause menu's Options page (0x1b4950: its title, list, help label and
// picture), as the New Game mode popup (graphics.cpp): our title, a list of three entries (Update Now,
// Not Now, Skip Version), the help label showing the selected entry's text (the news, the release
// notes in short and how to update; the label scrolls a text longer than its box) and our picture ("NEW", advanced.cpp).
// All entries are pages (action 3) to the main menu's first page, the page's parent (triangle too);
// update_notice_choice, a hook in the list's action 3, triggers the update or keeps "Skip Version" in the preferences.
//
// It is opened as the game opens a page (the menu's next page, its +8) by update_notice_menu_frame,
// once, when the main menu stands idle on its first page: the game's state is not touched otherwise,
// and nothing at all happens without an offer (headless runs: never).
//
// EE RAM block UPD (0xf8000..0xf9000), in the PS2 kernel's first megabyte as the Advanced, Graphics and
// Controls blocks (0xf0000..0xf8000): nothing of the game or the runtime lives there.
#include <game/enhance/update_notice.h>

#include <game/enhance/advanced.h>
#include <game/ovl/ovl_common.h>
#include <game/ovl/ovl_reloc.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace rac1 {

namespace {
constexpr uint32_t UPD = 0x000f8000u;
constexpr uint32_t PAGE = UPD;              // the page (a copy of Options')
constexpr uint32_t TITLE = UPD + 0x90;      // its title (a copy of Options' title)
constexpr uint32_t LIST = UPD + 0xe0;       // its list (a copy of Options' list)
constexpr uint32_t LABEL = UPD + 0x130;     // its help label (a copy of Options')
constexpr uint32_t PICTURE = UPD + 0x180;   // its picture (a copy of Options')
constexpr uint32_t ENTRIES = UPD + 0x200;   // the list's entries (0xc each, ended by text 0)
constexpr uint32_t HELP = UPD + 0x240;      // the help label's texts, one per entry
constexpr uint32_t PICTURES = UPD + 0x260;  // the picture's images (sector, count), one per entry
constexpr uint32_t STRINGS = UPD + 0x300;   // the texts, made at each lookup
constexpr uint32_t STRING_SIZE = 0x1c0;

// The game's data the page is made from (Veldin's addresses, relocated: also in the boot module)
constexpr OvConst<0x001b4950u> OPTIONS_PAGE{};
constexpr OvConst<0x001b8798u> ROOT_NEW_GAME{};  // the main menu's New Game button (a one-entry list): an entry to copy

// Our menu picture (advanced.cpp makes it: a sector past the end of the disc)
constexpr uint32_t PICTURE_SECTOR = 0xadf00000u;
constexpr uint32_t PIC_UPDATE = 6;  // (advanced.cpp's PIC_UPDATE: "NEW")

// The texts. Ids from TEXT_FIRST (before the Advanced page's 0x7f00..; the game's are 0x4e00..0x5300)
constexpr int32_t TEXT_FIRST = 0x7e00;
enum : int32_t { I_TITLE, I_UPDATE, I_OK, I_SKIP, I_HELP_UPDATE, I_HELP_OK, I_HELP_SKIP, I_COUNT };  // the ids the widgets are given
static_assert(0x300 + I_COUNT * STRING_SIZE <= 0x1000, "the notice's texts end where its block does");
// per language (0x15ed88: 0 English, 2 French (in English for now), 3 German, 4 Spanish, 5 Italian); {v} the
// new version, {i} the installed one, {n} the commits, {s} the setup's command. The release notes are GitHub's (English).
enum : int32_t { T_TITLE, T_UPDATE, T_OK, T_SKIP, T_RELEASE, T_COMMITS, T_COMMIT, T_DO_UPDATE, T_HOW_LATER, T_SKIP_RELEASE, T_SKIP_COMMITS, T_COUNT };
constexpr const char* TEXTS[4][T_COUNT] = {
    {"Update Available", "Update Now", "Not Now", "Skip Version",
     "Version {v} is out (you have {i}).", "{n} new commits on main (you have {i}).", "1 new commit on main (you have {i}).",
     "Downloads and builds the update, then restarts the game.", "To update later: {s} --update",
     "Not shown again for version {v}.", "Not shown again until main has newer commits."},
    {"Update verfugbar", "Jetzt aktualisieren", "Spater", "Uberspringen",
     "Version {v} ist da (installiert: {i}).", "{n} neue Commits auf main (installiert: {i}).", "1 neuer Commit auf main (installiert: {i}).",
     "Ladt das Update herunter, baut es und startet das Spiel neu.", "Spater aktualisieren: {s} --update",
     "Fur Version {v} nicht mehr angezeigt.", "Erst bei neueren Commits auf main wieder angezeigt."},
    {"Actualizacion disponible", "Actualizar ahora", "Ahora no", "Omitir version",
     "Ya esta la version {v} (tienes la {i}).", "{n} commits nuevos en main (tienes {i}).", "1 commit nuevo en main (tienes {i}).",
     "Descarga y compila la actualizacion y reinicia el juego.", "Para actualizar despues: {s} --update",
     "No se mostrara mas para la version {v}.", "No se mostrara hasta que haya commits nuevos en main."},
    {"Aggiornamento disponibile", "Aggiorna ora", "Non ora", "Salta versione",
     "E uscita la versione {v} (hai la {i}).", "{n} nuovi commit su main (hai {i}).", "1 nuovo commit su main (hai {i}).",
     "Scarica e compila l'aggiornamento, poi riavvia il gioco.", "Per aggiornare dopo: {s} --update",
     "Non sara piu mostrato per la versione {v}.", "Non sara piu mostrato finche non ci sono nuovi commit su main."},
};

struct State {
    bool offered = false, shown = false, update_now = false;
    UpdateOffer offer;
} st_;

// orig: (none) — enhancement: n bytes (a multiple of 4) copied in EE RAM
void ee_copy(uint32_t dst, uint32_t src, uint32_t n) {
    for (uint32_t i = 0; i < n; i += 4) sw(dst + i, lwu(src + i));
}
// orig: (none) — enhancement: the page `page`'s slots holding `from` pointed at `to`
void slot_replace(uint32_t page, uint32_t from, uint32_t to) {
    for (uint32_t i = 0; i < 14; ++i)
        if (lwu(page + 0x44 + i * 4) == from) sw(page + 0x44 + i * 4, to);
}

// orig: (none) — enhancement: the language's texts
const char* const* texts() {
    switch (lwu(0x0015ed88)) {
        case 3: return TEXTS[1];
        case 4: return TEXTS[2];
        case 5: return TEXTS[3];
        default: return TEXTS[0];
    }
}

// orig: (none) — enhancement: a text with its {v} {i} {n} {s} filled in
std::string fill(const char* t) {
    const UpdateOffer& o = st_.offer;
    std::string out;
    for (const char* p = t; *p != 0; ++p) {
        if (p[0] == '{' && p[1] != 0 && p[2] == '}') {
            switch (p[1]) {
                case 'v': out += o.version; break;
                case 'i': out += o.installed; break;
                case 'n': out += std::to_string(o.commits); break;
                case 's': out += o.windows ? "setup.cmd" : "./setup.sh"; break;
                default: break;
            }
            p += 2;
            continue;
        }
        out += *p;
    }
    return out;
}

// orig: (none) — enhancement: the page built (its parent `root`, the main menu's first page)
void page_build(uint32_t root) {
    const uint32_t opt = OPTIONS_PAGE;
    ee_copy(PAGE, opt, 0x84);
    const uint32_t title = lwu(opt + 0x44), label = lwu(opt + 0x48), picture = lwu(opt + 0x4c), list = lwu(opt + 0x50);
    ee_copy(TITLE, title, 0x50);
    ee_copy(LABEL, label, 0x50);
    ee_copy(PICTURE, picture, 0x50);
    ee_copy(LIST, list, 0x50);
    sw(TITLE + 0x34, TEXT_FIRST + I_TITLE);
    // the entries: Update Now, Not Now and Skip Version, each a page (action 3) back to the main menu's first page
    const uint32_t ng = lwu(ROOT_NEW_GAME + 0x34);
    constexpr uint32_t N = 3;
    static constexpr int32_t NAMES[N] = {I_UPDATE, I_OK, I_SKIP}, HELPS[N] = {I_HELP_UPDATE, I_HELP_OK, I_HELP_SKIP};
    for (uint32_t i = 0; i < N; ++i) {
        const uint32_t e = ENTRIES + i * 0xc;
        ee_copy(e, ng, 0xc);
        sh(e, static_cast<uint32_t>(TEXT_FIRST + NAMES[i]));
        sh(e + 2, 3);
        sw(e + 4, root);
        sh(e + 0xa, 0);
        sw(HELP + i * 4, static_cast<uint32_t>(TEXT_FIRST + HELPS[i]));
        sw(PICTURES + i * 8, PICTURE_SECTOR + PIC_UPDATE);
        sw(PICTURES + i * 8 + 4, 1);
    }
    for (uint32_t i = 0; i < 0xc; i += 4) sw(ENTRIES + N * 0xc + i, 0);
    sw(HELP + N * 4, 0);
    sw(LIST + 0x34, ENTRIES);
    sw(LIST + 0x40, 0);  // (Update Now selected first)
    sw(LIST + 0x38, 0);
    sw(LIST + 0x3c, 0);
    sw(LABEL + 0x34, HELP);
    sw(PICTURE + 0x30, PICTURES);
    sw(PAGE + 0x38, root);
    sw(PAGE + 0x40, LIST);
    slot_replace(PAGE, title, TITLE);
    slot_replace(PAGE, label, LABEL);
    slot_replace(PAGE, picture, PICTURE);
    slot_replace(PAGE, list, LIST);
    sw(PAGE + 0x80, 0);
}
}  // namespace

// orig: (none) — enhancement (update_notice.h)
void update_notice_offer(const UpdateOffer& offer) {
    if (st_.offered && st_.offer.development == offer.development && st_.offer.id == offer.id) return;
    st_.offered = true;
    st_.shown = false;
    st_.offer = offer;
}

// orig: (none) — enhancement (update_notice.h)
void update_notice_menu_frame(uint32_t ms, uint32_t root) {
    if (!st_.offered || st_.shown || g_advanced_disabled) return;
    // idle on the first page: no page change or closing under way, no memory card screen asked
    if (lwu(ms + 4) != root || lwu(ms + 8) != 0 || lw(ms) == 1 || lw(ms + 0xc) != 0 || (lwu(0x0015eeb4) & 1) != 0) return;
    st_.shown = true;
    page_build(root);
    sw(ms + 8, PAGE);
    if (st_.offer.development)
        std::printf("[update] the notice shown: %d new commit(s) on main\n", st_.offer.commits);
    else
        std::printf("[update] the notice shown: version %s\n", st_.offer.version.c_str());
}

// orig: (none) — enhancement (update_notice.h)
void update_notice_choice(uint32_t w) {
    if (w != LIST || !st_.shown) return;
    const int32_t sel = lw(w + 0x40);
    if (sel == 0) {
        st_.update_now = true;
        std::printf("[update] Update Now chosen (%s)\n", st_.offer.id.c_str());
    } else if (sel == 2) {
        advanced_update_skip(st_.offer.id);
        std::printf("[update] %s skipped: not shown again\n", st_.offer.id.c_str());
    }
}

// orig: (none) — enhancement: true when the player chose Update Now on the notice
bool update_notice_update_now(bool* development) {
    if (!st_.update_now) return false;
    if (development != nullptr) *development = st_.offer.development;
    return true;
}

// orig: (none) — enhancement (update_notice.h)
uint32_t update_notice_page() { return st_.shown ? PAGE : 0; }

// orig: (none) — enhancement (update_notice.h)
uint32_t update_notice_text(int32_t id) {
    if (id < TEXT_FIRST || id >= TEXT_FIRST + I_COUNT || !st_.offered) return 0;
    const int32_t k = id - TEXT_FIRST;
    const char* const* t = texts();
    const UpdateOffer& o = st_.offer;
    const std::string news = fill(t[o.development ? (o.commits == 1 ? T_COMMIT : T_COMMITS) : T_RELEASE]);
    const std::string how = fill(t[T_HOW_LATER]);
    std::string s;
    switch (k) {
        case I_TITLE: s = t[T_TITLE]; break;
        case I_UPDATE: s = t[T_UPDATE]; break;
        case I_OK: s = t[T_OK]; break;
        case I_SKIP: s = t[T_SKIP]; break;
        case I_HELP_UPDATE:
            s = news;
            if (!o.development && !o.notes.empty()) s += " " + o.notes;
            s += std::string(" ") + t[T_DO_UPDATE];
            break;
        case I_HELP_OK:
            s = news + " " + how + ".";
            if (!o.development && !o.notes.empty()) s += " " + o.notes;
            break;
        case I_HELP_SKIP: s = fill(t[o.development ? T_SKIP_COMMITS : T_SKIP_RELEASE]) + " " + how + "."; break;
        default: break;
    }
    char* p = static_cast<char*>(ee_ram_ptr(STRINGS + static_cast<uint32_t>(k) * STRING_SIZE));
    std::strncpy(p, s.c_str(), STRING_SIZE - 1);
    p[STRING_SIZE - 1] = 0;
    return STRINGS + static_cast<uint32_t>(k) * STRING_SIZE;
}

}  // namespace rac1
