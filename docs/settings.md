# Settings

The port's own settings: three pages added to the game's Options (Graphics, Controls, Advanced), the
Original, Modern or Experimental choice at New Game, where settings are kept, and the debug tools.

## Removed compromises and enhancements

The port keeps two kinds of settings apart (see
[Overview](overview.md#faithful-to-the-games-code-not-to-the-ps2s-limits)):

- **PS2 compromises removed**, such as the 4:3 frame, short draw distances, small textures and
  fades that hid disc loading. Removing them changes the picture, not the game's state, and the
  original's way stays available. Most of these are on the Graphics page; Fades is on the Advanced
  page.
- **Enhancements**, which change the game's behaviour: the quick-select pause, modern aiming,
  Ratchet & Clank 3 movement, and, on the picture side, the modern lighting. They are optional, off by
  default, and labelled as changes.

## The pages

The three pages are added to Options in both the pause menu and the main menu, before Quit Game.
They are built from the game's own menu widgets and data (a copy of the Camera page), so they look
and behave like the game's own pages. Left and right change a row's value; Cross steps it forward.

### Graphics

`src/game/enhance/graphics.cpp`. Every row changes only what the host draws, never the game's state.

| Row | Values | Default |
|---|---|---|
| Display | Window, Fullscreen | Window |
| Window Size | 960x720 to 3840x2160 (11 sizes) | 960x720 |
| V-Sync | Off, On | On |
| Frame Rate Cap | 30, 60, 120, 144, 240, Unlimited | 60 |
| Render Resolution | x0.50 to x2.00 of the window's, in steps of 0.25 | x1.00 |
| Aspect Ratio | 4:3 Original, 16:9, Fill Screen | 4:3 Original |
| Fit to Screen | Black Bars, Stretch | Black Bars |
| Overscan | TV, Full Frame | TV |
| Field of View | x0.75 to x1.50 of the original's | x1.00 |
| Level of Detail | Original, Low, Medium, High, Maximum | Original |
| Lighting | Original, Low, Medium, High | Original |
| Shadow Distance | Low, Medium, High, Maximum | Maximum |
| Dynamic Shadows | Off, On | Off |
| Secondary Lights | Off, Game's, All | All |
| Ambient Occlusion | Off, On | Off |
| Texture Detail | Full, Original (PS2) | Full |

- **The frame-rate cap** only paces the picture. The game itself always runs at 60 frames a second.
- **Render Resolution** scales the 3D; the HUD stays at the window's resolution.
- **Aspect Ratio** Fill Screen widens the 3D to the window's shape without changing what the game
  sees; **Fit to Screen** applies when the chosen shape is not the window's. See
  [Rendering](rendering.md#widescreen-and-render-scale).
- **Overscan** TV hides a thin strip (4 of the game's 512 x 416 frame pixels) at the left, right and
  top edges, the part of the picture a TV of the time did not show; the game parks some 2D there that
  was never meant to be seen. The crop applies to the 2D layer only (HUD, menus, texts), never to the
  3D. Full Frame shows everything the PS2 drew. Headless runs always keep the full frame.
- **Level of Detail** draws more detail further away, without changing the game's own decisions. See
  [Rendering](rendering.md#level-of-detail).
- **Lighting and the four rows after it** are the optional modern lighting; the last four only act
  when Lighting is not Original. See [Lighting](lighting.md).

### Controls

The controller and keyboard bindings, mouse look, look inversion and mouse sensitivity. See
[Input](input.md#the-controls-page).

### Advanced

| Row | Values (default first) |
|---|---|
| Quick Select | Normal, Pause |
| Aiming | Original, Modern |
| Movement | Original, RAC 3 |
| Fades | Fast, Original |

See [Advanced options](advanced.md) for what each one does and how it is built.

## Original, Modern or Experimental mode

New Game opens a choice between three modes before the game starts
(`graphics_new_game_choice`). Each says that every setting can be changed at any time in Options.

| | Original | Modern | Experimental |
|---|---|---|---|
| Aspect Ratio | 4:3 Original | Fill Screen | Fill Screen |
| Level of Detail | Original | High | High |
| Texture Detail | Original (PS2) | Full | Full |
| Quick Select, Aiming, Movement | Normal, Original, Original | Pause, Modern, RAC 3 | Pause, Modern, RAC 3 |
| Modern lighting (Lighting, Dynamic Shadows, Secondary Lights, Shadow Distance, Ambient Occlusion) | off (the defaults) | off (the defaults) | everything at its highest |

Experimental is Modern plus the [modern lighting](lighting.md), which is experimental. No mode changes
the window settings or Fades. The mode chosen is kept with the save.

## Where settings are kept

| What | Where |
|---|---|
| Graphics page | `~/.config/rac1-native/graphics_options.txt` |
| Advanced page | `~/.config/rac1-native/advanced_options.txt` |
| Controls | `~/.config/rac1-native/controls.txt` (see [Input](input.md#the-bindings-file-controlstxt)) |
| The picture settings, the Advanced settings and the mode, per save | `save<N>.pc` next to each save (see [Saves](saves.md#the-pc-settings-next-to-each-save)) |
| The game's own options | the game's own settings file on the memory card, as on the PS2 |

`~/.config` stands for `$XDG_CONFIG_HOME` when it is set. On Windows the folder is
`%APPDATA%\rac1-native\`. `RAC1_GRAPHICS_FILE`, `RAC1_ADVANCED_FILE` and `RAC1_CONTROLS_FILE` point
to other files.

The window's own settings (display mode, window size, v-sync, frame-rate cap) belong to the machine
and are kept only in the preferences. The picture settings (render resolution, aspect ratio, fit,
field of view, level of detail, the lighting rows, texture detail, overscan) and the Advanced settings are also written into
each save's `save<N>.pc`, so loading a save brings back the settings it was played with. A save
without that file (an older one) keeps the current settings.

## Debug tools

The debug tools are for development and testing. They are off unless asked for with
`--debug-tools` on the command line or `RAC1_DEBUG_TOOLS=1` in the environment (the gate is
`debug_tools_gate` in `src/host/orig_main.cpp`, which runs before anything else):

```bash
./build/rac1-native --debug-tools
./build/rac1-native --debug-tools --level 3    # start directly on Kerwan
```

| | Without the debug tools | With `--debug-tools` |
|---|---|---|
| The debug menu (F1, or R3 on the pad) | not available; R3 is the game's | open/close |
| Moving Ratchet by hand (F2, or L3) | not available; L3 is the game's | on/off |
| Renderer choice (MRP or the faithful VU path), layer toggles, textures' mip choice | the MRP, everything drawn | in the debug menu's Rendering tab |
| The light editor | not available | the debug menu's Lights tab |
| `--level N` | ignored, with a message | starts on level N |
| `RAC1_*` testing variables | removed from the environment at start, with a message | honoured |

The debug menu's code is in `src/_devtools/` and can be left out of the build entirely with
`-DRAC1_DEVTOOLS=OFF`.

**Environment variables a player may use** (always honoured): `RAC1_ISO` (the disc image's path),
`RAC1_MEMCARD_DIR` (the memory card's directory), `RAC1_GRAPHICS_FILE`, `RAC1_ADVANCED_FILE`,
`RAC1_CONTROLS_FILE`, `RAC1_WINDOW`, `RAC1_FULLSCREEN`, `RAC1_VSYNC`, `RAC1_FPS_CAP`,
`RAC1_NO_AUDIO`, `RAC1_AUDIO_DEVICE`, `RAC1_AUDIO_PERIOD_MS`, `RAC1_AUDIO_PERIODS`,
`RAC1_MOVIE_VOLUME`, `RAC1_PRIORITY` and `RAC1_IOP_MIRROR`. `RAC1_HEADLESS` runs without a visible
window for automated tests, and also allows the testing variables.
