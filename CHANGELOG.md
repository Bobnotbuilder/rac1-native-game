# Changelog

What changed in each release. Releases are source only: you build the game with `./setup.sh`
(`setup.cmd` on Windows) from your own disc. Issue numbers refer to this repository's issues.

## 0.2.0

### New
- **Batalia, Gaspar and Orxon** are ported and playable: Batalia's rain, its turret and the Trespasser
  puzzle; Gaspar's lava and the Pilot's Helmet (it could not be picked up before, on any level);
  Orxon's toxic goo, Clank's section and Ratchet's part with the O2 mask.
- Weapons and gadgets shared by every level: the Drone Device's drones, the Morph-o-Ray's chickens
  and its HUD element.
- **The European PAL disc** (`SCES-50916`) works on its own, without the North American disc (#12). It
  runs in the PAL game's own 60 Hz mode. Ten functions that the PAL release changed are still
  temporary versions, not yet proved against the original (see the Setup page).
- **Windows support, experimental** (#11): the same sources build on Windows; `setup.cmd` downloads
  everything it needs, and `play.cmd` starts the game.
- **An update check** (#14): at start the game checks GitHub for a new release and says so at the main
  menu. It downloads nothing, and can be turned off in Options > Advanced. A work in progress.
- **The oracle is in this repository** (`tests/oracle`, [Oracle](docs/oracle.md)): the tests that prove
  each port against the original code.

### Fixed
- Controllers missing from the controller database (for example the GameSir G7 SE) were ignored: on
  Linux, Xbox-style controllers now get the Xbox layout automatically (#1).
- The language chosen in the game is kept for the next start, as a PS2 keeps its system language (#15).
- Below 60 fps (the 30 FPS cap, V-Sync at 75 Hz, a heavy scene) the game lost frames: it ran slow and
  the movies' sound fluttered (#4, #22; better on #8 and #24).
- Effects turned black or invisible after a while on Aridia on NVIDIA cards (glows, smoke, bolts, the
  sand), which could then crash the game (#19; to be confirmed on NVIDIA).
- The air meter left a line at the screen's edge after diving (#5).
- The Vendor showed a chicken icon and 0/0 ammo for the centre weapon (#27).
- Kerwan's helicopters' bullets froze in mid-air (#26).
- Advanced options changed at the main menu were lost if you quit without playing.
- On Windows, the setup failed with a disc image whose path contains `&` (#35).

## 0.1.0

Milestone 1: the game playable from Veldin to Umbris, on Linux, from the North American disc.
