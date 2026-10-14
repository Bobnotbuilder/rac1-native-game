// The installed version and the update check (GitHub issue #14; docs/settings.md, "Update check"). Host code:
// at start, in a thread of its own, tools/update_check.py asks GitHub once (anonymously) whether there is
// something newer than this build; what it found is handed to the game's update notice
// (src/game/enhance/update_notice.h), shown at the main menu. Never in a headless run; off with Options >
// Advanced > Update Check.
#ifndef RAC1_HOST_UPDATE_CHECK_H
#define RAC1_HOST_UPDATE_CHECK_H

#include <string>

namespace rac1::update_check {

// orig: (none) — host: this build's version as the log shows it: "0.1.0 (v0.1.0-10-g7dee3d1-dirty, main)"
std::string version_text();

// orig: (none) — host: the check started in the background when the player's settings ask for it (never
// headless); testing: RAC1_UPDATE_FAKE (headless, or --debug-tools) offers a made-up update instead, no network
void start(bool headless);

// orig: (none) — host: once a host frame, on the game's thread: the check's answer, when it has come,
// handed to the update notice (once)
void poll();

// orig: (none) — host: true when the player chose "Update Now" on the update notice (the main loop leaves)
bool update_requested();

// orig: (none) — host: after the window and audio are closed: starts the setup in update mode (--update --play)
void run_update();

}  // namespace rac1::update_check

#endif  // RAC1_HOST_UPDATE_CHECK_H
