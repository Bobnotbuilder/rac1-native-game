// The console's language, kept by the host as the PS2's system settings keep it (issue #15). Host code.
//
// The game does not save its language: at the start its main asks the console for the console's
// language (0x12d240) and writes the game's code for it into 0x15ed88, which the texts, subtitles,
// movies and the HUD read; the Options page changes 0x15ed88 for the session only. On a PS2 the
// player sets the console's language once, in the console's own settings. The host has no console,
// so it stands in for those settings: the language the player last chose in the game becomes the
// console's language, kept in the settings folder (console.txt). The game's logic is unchanged.
#ifndef RAC1_HOST_LANGUAGE_H
#define RAC1_HOST_LANGUAGE_H

#include <cstdint>

namespace rac1::host_language {

// The game's code for the console's language (0 English, 2 French, 3 German, 4 Spanish, 5 Italian):
// the one kept in console.txt, else the system's language when it is one of those, else English.
// `keep`: false in a headless run (the tests): English, and nothing is read or written.
uint32_t console_language(bool keep);

// Once per host frame, with the game's language (0x15ed88): when the player changed it, kept as the
// console's language. Nothing is written in a headless run or while RAC1_LANGUAGE (testing) is set.
void note(uint32_t game_language);

}  // namespace rac1::host_language

#endif
