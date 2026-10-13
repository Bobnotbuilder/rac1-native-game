# Status

*State as of October 2026.*

**Milestone 1 is reached: the game is complete up to Umbris** (Veldin, Novalis, Aridia, Kerwan,
Eudora, Rilgar, Blarg / Nebula G34 and Umbris; see [Roadmap](roadmap.md)). The port as a whole is
still in progress: the planets after Umbris are only partly ported or not yet.

Every planet of the milestone has its own classes ported and proved against the original code, and
runs on its own overlay. The gaps that remain on them are listed below as **known issues**; they are
bugs to fix, not missing parts of the milestone.

## Level names

The code and the game's data name the levels by their folder on the disc (`levels/<n>_<name>`),
and some of those names differ from the planet or location the player sees in the game. This table
is the one place that maps them; other pages link here.

| Level | Name in the code | Name in the game |
|---|---|---|
| boot module | the boot module (not a level) | The start of the game: intro movie, title screen, main menu, New Game, story cards |
| 0 | Veldin (tutorial) | Veldin |
| 1 | Novalis | Novalis |
| 2 | Aridia | Aridia |
| 3 | Kerwan | Kerwan (its city is called Metropolis) |
| 4 | Eudora | Eudora |
| 5 | Rilgar | Rilgar (its city is Blackwater City, where the hoverboard race is) |
| 6 | Nebula G34 | Blarg's station in the Nebula G34 galaxy |
| 7 | Umbris | Umbris |
| 8 | Batalia | Batalia |
| 9 | Gaspar | Gaspar |
| 10 | Orxon | Orxon |
| 11 | Pokitaru | Pokitaru |
| 12 | Hoven | Hoven |
| 13 | Oltanis Orbit | Gemlik Base |
| 14 | Oltanis | Oltanis |
| 15 | Quartu | Quartu |
| 16 | Kalebo III | Kalebo III |
| 17 | Veldin Orbit | Drek's fleet |
| 18 | Veldin, the finale | Veldin, the return at the end of the game (called "Veldin II" in some notes; unverified) |

The rest of this page uses the in-game name first and the code's name in brackets where they differ.

## The planets

Levels 0 to 7 are Milestone 1, and the whole milestone has been played through from Veldin to Umbris (included): apart from a few small bugs and glitches it plays without major problems. After Umbris the game takes the player to Batalia, which runs but is only partly ported, as are the planets after it: they are not playable end to end yet.

> **The planets after Umbris:** they all load and run, and some may already be largely playable. Batalia, for example, looks playable apart from its rain effect. But part of the code that covers them, or that may cover some of their sections, is not ported yet, so expect missing effects, objects that don't react, or sections that can't be finished. They have not been tested in depth yet. Bug reports are welcome.

Every one of the game's 19 levels (0 to 18) loads and runs on its own overlay (the level's own build of the
engine, see [Architecture](architecture.md#per-level-overlays-and-the-ov-macros)), without crashing,
in automated runs that idle, walk and attack. What differs from planet to planet is how much of its
own code is ported. Code that is not ported yet is a `TODO_PORT` stub: it logs once and does
nothing, so an object whose class is not ported stands still or does not appear.

| # | Planet | State | Known gaps |
|---|---|---|---|
| 0 | Veldin | Complete: boot, intro movie, title screen, the opening, the tutorial and the ending movie to Novalis. | |
| 1 | Novalis | Complete. | |
| 2 | Aridia | Complete. | |
| 3 | Kerwan | Complete. | |
| 4 | Eudora | Complete. | |
| 5 | Rilgar | Complete, including the hoverboard race. | |
| 6 | Nebula G34 (Blarg's station; see [names](#level-names)) | Complete, including the boss fight. | |
| 7 | Umbris | Complete, including the Snagglebeast. | |
| 8 | Batalia | First part ported: 58 of its own classes. | Not playable end to end yet: some moby callbacks and draw callbacks not ported; some class states differ from the original. |
| 9 | Gaspar | Its own classes ported, with their draw callbacks (the lava's animated texture, the background ships' glows, the Visibomb's static) and the Pilot's Helmet pickup. Ready for playtest; not played through by hand yet. | |
| 10 | Orxon | Its own classes ported, with their draw callbacks: Clank's section, and Ratchet's part once he has the O2 mask. Ready for playtest; not played through by hand yet. | |
| 11 | Pokitaru | First part ported. | Weapons and gadgets bought there not ported. |
| 12 | Hoven | Loads and runs. | Nothing of its own ported. |
| 13 | Gemlik Base (code: Oltanis Orbit) | Loads and runs. | Nothing of its own ported. |
| 14 | Oltanis | Loads and runs. | Nothing of its own ported. |
| 15 | Quartu | Loads and runs. | Nothing of its own ported. |
| 16 | Kalebo III | Loads and runs. | Nothing of its own ported. |
| 17 | Drek's fleet (code: Veldin Orbit) | Loads and runs. | Nothing of its own ported. |
| 18 | Veldin, the finale | Loads and runs. | Nothing of its own ported. |

### Fixed since the first playtest

- RAC 3 movement (Advanced): the wrench can no longer be thrown again before it is back in Ratchet's hand.

- The flight between planets: the loading screen shows the ship flying through space to the next
  planet, then the planet and its name (it was a dark screen with only the sound).
- The pause menu's 3D models: Ratchet holding the selected weapon or gadget, and the item models on
  the menus' pages, are drawn.
- The Trespasser puzzle (Aridia, Eudora) is drawn, takes its input and can be solved.
- The Suck Cannon pulls enemies in and stores them as ammo, on every level.
- Objects whose drawing the port had skipped are drawn: the Visibomb's out-of-range static, the
  Sandmouse's glows and the Thruster-Pack's jets.
- In widescreen, effects such as the ship's canopy glass and the glows no longer drift sideways as
  the camera turns.
- The Advanced page's movement options (RAC 3 movement, the strafe, modern aiming) now work fully on
  every level, including those with their own build of the hero's code.
- Cutscene subtitles are drawn when Options > Subtitles is on, in every language the disc carries.
- Clank's Gadgebot command menu (the ring opened with triangle while playing as Clank) is drawn and
  its commands reach the Gadgebots, on Nebula G34 and Orxon.

## Engine and PC features

| Area | State |
|---|---|
| Boot, intro movie, title screen, main menu, New Game and the story cards and movies to Veldin | Ported. |
| Memory card, save and load, the save on planet change | Works, but may still change: the save files' format, location and size have not been checked by hand yet. See [Saves](saves.md). |
| Pause menu (with its 3D models), HUD, Vendor, the ship's planet menu, the loading screen's flight | Ported. Known issue: the planet menu's information panels draw empty. |
| Sound and music (989snd), dialogue streams, movies | Working; see [Audio](audio.md). The sound mirror that keeps music playing through loads is still a proposal (see [Decisions](decisions.md#13-the-iops-sound-runs-twice)). |
| 3D rendering (the MRP) | The default renderer; see [Rendering](rendering.md). |
| Modern lighting (optional) | Working, off by default; see [Lighting](lighting.md). |
| Options > Graphics (including Texture Detail and Overscan, the 2D cut to a TV's area), Controls, Advanced; the Original / Modern / Experimental choice at New Game | Implemented, known issues may remain; see [Settings](settings.md). |
| Keyboard, mouse and controller mapping, remapping in Options > Controls | Implemented, known issues may remain; see [Input](input.md). |
| Debug tools behind `--debug-tools` | Implemented; see [Settings](settings.md#debug-tools). |

## Not done yet

- **The second half of the game.** Levels 8 to 11 have their first part ported; levels 12 to 18
  have nothing of their own ported yet.
- **Shared code still stubbed.** Every level still reaches a few `TODO_PORT` stubs that are not its
  own code: some kernel and DMA calls the host does not need. Several weapon and gadget classes are
  not ported on any level yet.
- **Not everything is native in the renderer.** The HUD, menus and some effects are still drawn
  through the GS replica, and the mobys' flattened shadow meshes are not drawn yet (see
  [Rendering](rendering.md#limits)).
- **Platforms.** Linux x86-64; Windows x86-64 is experimental (see [Platforms](platforms.md)). Vulkan / Direct3D renderers and macOS may come later.
- **Regions.** Only the North American disc (`SCUS-97199`) is supported.
