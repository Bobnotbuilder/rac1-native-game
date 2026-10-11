# gamecontrollerdb

`gamecontrollerdb.txt` is the community game controller database of
[SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB) (zlib licence, see `LICENSE`),
unmodified: the SDL 2.0.16 mapping format, which GLFW reads (`glfwUpdateGamepadMappings`).

`rac1-native` loads it at start (`src/host/orig_main.cpp` `load_controller_db`), so that Xbox,
DualShock, DualSense, Switch Pro and generic pads are seen as a standard gamepad and mapped to the PS2 pad
(`~/.config/rac1-native/controls.txt`, see `docs/input.md`). GLFW's own built-in copy stays under it;
the player's additions (`~/.config/rac1-native/gamecontrollerdb.txt`) and `SDL_GAMECONTROLLERCONFIG` (what Steam sets)
are read after it.

Copy of 2026-09-18. To refresh it, replace both files with the upstream `gamecontrollerdb.txt` and `LICENSE`.
