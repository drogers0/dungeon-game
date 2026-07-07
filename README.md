# Dungeon Game

A small two-player arcade-style prototype built on **SFML 2**. Players control a rocket and a
robot, earn points via collisions and attacks, and the game features animated sprites, sounds,
music, and a start/end screen loop. Online multiplayer (host/join over TCP) is also supported.

See [`docs/building.md`](docs/building.md) for full build and run instructions.

---

## Quick start

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/dungeon_game
```

SFML 2.6.2 is fetched and built automatically on the first configure — no system SFML install
required. Assets are copied next to the binary at build time, so the game runs from any directory.

---

## Online multiplayer

Launch the game and use the main menu:

- **LOCAL** — two players share the keyboard (Player 1: numpad/arrows, Player 2: WASD/Space).
- **HOST** — start a TCP server; share your local IP with the other player.
- **JOIN** — enter the host's IP address and press Enter to connect.

---

## Controls

| Action         | Player 1 (Rocket)      | Player 2 (Robot)  |
|----------------|------------------------|-------------------|
| Move           | Numpad 8/5/4/6         | W/A/S/D           |
| Attack         | Right arrow            | Space             |

| Global         | Key                    |
|----------------|------------------------|
| Slow down      | O                      |
| Speed up       | P                      |
| Skip cooldown  | K                      |
| Pause / resume | Esc                    |
| Quit to menu   | Q (while paused)       |
| Fullscreen     | F11                    |

The window is resizable and scales with a letterbox (aspect preserved). Key bindings can be customised
via a `controls.cfg` file next to the binary — run `dungeon_game --help` for the format.

---

## Project layout

```
AnimatedGameObject.*    Sprite-sheet animation helper
RegularGameObject.*     Simple textured sprite wrapper
Game.*                  Core game loop, input, scoring, collisions, network integration
GameObject.h            Abstract entity interface
NetworkManager.*        TCP host/client transport; PlayerInput/GameState wire format
main.cpp                Start/end screen menus; mode selection (local/host/join)
resource_path.{h,cpp}   Executable-relative asset root (initResourcePath)
assets/                 Asset root — images, audio, fonts
CMakeLists.txt          CMake build (SFML 2.6.2 via FetchContent)
```

---

## Tests

```bash
ctest --test-dir build --output-on-failure
```

The test framework (Catch2 via FetchContent) and test sources are added in issue #7.

---

## Building on Linux

Install SFML's system dev packages before configuring:

```bash
sudo apt-get install -y \
  libx11-dev libxrandr-dev libxcursor-dev libxi-dev \
  libudev-dev libgl1-mesa-dev libopenal-dev
```

---

## Download & Play

Grab the latest zip for your OS from the [Releases](../../releases) page, unzip, and run the binary.
Assets sit alongside it in the `assets/` folder.

- **Windows:** the zip contains `dungeon_game.exe`, `openal32.dll`, and `assets/` — no install required.
- **macOS:** unzip and run `dungeon_game` directly.
- **Linux:** unzip and run `dungeon_game`. The binary is statically linked against SFML but still
  resolves system runtime libraries — install the following if they are not already present:
  ```bash
  sudo apt-get install libopenal1 libgl1 libx11-6 libxrandr2 libxcursor1 libxi6 \
    libudev1 libfreetype6 libflac8 libvorbis0a libogg0
  ```

Releases are cut by pushing a `vX.Y.Z` tag; the CI release workflow builds and packages all three platforms automatically.

---

## Build on Windows

CMake with MSVC (Visual Studio 2019 or later):

```bat
cmake -B build
cmake --build build --config Release
build\Release\dungeon_game.exe
```

For a fully redistributable executable (no Visual C++ runtime dependency):

```bat
cmake -B build -A x64 -DBUILD_SHARED_LIBS=OFF -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
cmake --build build --config Release
```

SFML 2.6.2 is fetched automatically on the first configure. `openal32.dll` is copied next to the
executable at build time. See [`docs/building.md`](docs/building.md) for full details.

---

## License

Fonts and other third-party assets inside `assets/` carry their own licenses (e.g. SIL Open
Font License). Review and preserve any included license files when distributing.
