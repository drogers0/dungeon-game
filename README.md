# Dungeon Game

A small two‑player arcade style prototype built on **SFML 2**. Players control a rocket and a robot, earn points via collisions / attacks, and the game features animated sprites, sounds, music, and a simple start/end screen loop.

**NEW: Online Multiplayer Support!** Play with a friend over the network - one player hosts and the other joins via IP address.

This repository originated from a Windows Visual Studio project (`.vcxproj`). This README focuses on building and running the code on **macOS (Apple Silicon or Intel)** using Homebrew + `clang++`, and optionally CMake or Xcode.

Note about SFML versions:
- Homebrew may install SFML 3 by default as of late 2025. This code works with SFML 2.x

---
## 1. Project Structure
```
AnimatedGameObject.*    Sprite sheet animation helper
RegularGameObject.*     Simple textured sprite wrapper
Game.*                  Core game loop, input handling, scoring, collisions
GameObject.h            Base interface (implied by usage) for drawable/movable objects
NetworkManager.*        TCP-based networking for online multiplayer
main.cpp                Start menu + end screen loop; calls Game::run()
resource_path.h         Defines resource_path = "elements//" for asset loading
elements/               Asset root (images, audio, fonts, sprite sheets)
content/                (Present but unused in current code – legacy folder)
SkeletonCode.vcxproj*   Legacy Visual Studio project files (not used on macOS)
```
All asset loads are performed via `resource_path + filename`, meaning the **working directory must be the project root** (so that `elements/` resolves). If you run from another directory (e.g. inside `build/`), either copy the `elements/` folder next to the executable or adjust `resource_path.h`.

---
## 2. SFML Modules Used
The source includes and APIs from:
- Graphics (`#include <SFML/Graphics.hpp>`) – windows, sprites, fonts, text
- Audio (`sf::Music`, `sf::Sound`, `sf::SoundBuffer`)
- Window / Events (`sf::RenderWindow`, `sf::Event`)
- System (`sf::Clock`, `sf::Time`)
- Network (`sf::TcpListener`, `sf::TcpSocket`, `sf::Packet`) – for online multiplayer

You must link: `sfml-graphics sfml-window sfml-system sfml-audio sfml-network`.

---
## 3. Prerequisites (macOS)
1. Install Homebrew (if not installed):
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```
2. Install SFML:
```bash
brew install sfml@2
```
Homebrew places headers in `/opt/homebrew/include` or `/opt/homebrew/opt/sfml` (Apple Silicon) or `/usr/local/include` (Intel), and libs in the matching `lib` directory. Use `brew --prefix sfml@2` to confirm.

---
## 4. Quick Build & Run (One‑liner)
From the project root (so `elements/` is visible):
```bash
clang++ -std=c++17 \
  main.cpp Game.cpp AnimatedGameObject.cpp RegularGameObject.cpp NetworkManager.cpp \
  -I"$(brew --prefix sfml@2)/include" \
  -L"$(brew --prefix sfml@2)/lib" \
  -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lsfml-network \
  -Wl,-rpath,"$(brew --prefix sfml@2)/lib" \
  -O2 -o dungeon_game

./dungeon_game
```


<details>

<summary>Everything further is ai gerated and untested</summary>

---
## 5. Recommended: Separate Build Directory
```bash
mkdir -p build
cd build
clang++ -std=c++17 ../main.cpp ../Game.cpp ../AnimatedGameObject.cpp ../RegularGameObject.cpp \
  -I"$(brew --prefix)/include" -L"$(brew --prefix)/lib" \
  -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio \
  -Wl,-rpath,"$(brew --prefix)/lib" \
  -O2 -o dungeon_game

# Copy assets or adjust resource_path
cp -R ../elements ./
./dungeon_game
```
Alternative: edit `resource_path.h` to `"../elements//"` when running from `build/`.

---
## 6. CMake Support (Optional)
Create `CMakeLists.txt` in the repo root:
```cmake
cmake_minimum_required(VERSION 3.15)
project(DungeonGame LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(SOURCES
    main.cpp
    Game.cpp
    AnimatedGameObject.cpp
    RegularGameObject.cpp
    NetworkManager.cpp
)

# Find SFML (Homebrew install). Accept either 2.x or 3.x
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};$(brew --prefix sfml)")
find_package(SFML REQUIRED COMPONENTS graphics window system audio network)

add_executable(dungeon_game ${SOURCES})

target_link_libraries(dungeon_game PRIVATE sfml-graphics sfml-window sfml-system sfml-audio sfml-network)

# Ensure runtime finds dylibs (embed rpath)
set_target_properties(dungeon_game PROPERTIES
    INSTALL_RPATH "$(brew --prefix)/lib"
    BUILD_RPATH   "$(brew --prefix)/lib"
)
```
Build:
```bash
mkdir -p build
cd build
cmake ..
cmake --build . --config Release
cp -R ../elements ./
./dungeon_game
```
If `find_package` fails, supply hints:
```bash
cmake -DSFML_DIR="$(brew --prefix sfml)/lib/cmake/SFML" ..
```
If you specifically need SFML 2.x, point `SFML_DIR` at the 2.x config dir you built/installed.

---
## 7. Xcode IDE Setup (Manual Linking)
1. Open Xcode, create a new macOS Command Line Tool project (C++). 
2. Add existing source files (`main.cpp`, etc.) to the project.
3. Drag in the `elements/` folder ("Create folder references" so PNG/TTF/WAV ship with build or copy manually later).
4. In Build Settings:
   - Header Search Paths: `$(brew --prefix)/include`
   - Library Search Paths: `$(brew --prefix)/lib`
5. In Build Phases > Link Binary With Libraries: Add
   - `libsfml-graphics.dylib`
   - `libsfml-window.dylib`
   - `libsfml-system.dylib`
   - `libsfml-audio.dylib`
6. Set Runpath Search Paths (`LD_RUNPATH_SEARCH_PATHS`): `$(inherited) $(brew --prefix)/lib`
7. Ensure working directory set to project root (Scheme > Run > Options) or resource paths updated accordingly.

---
## 8. Runtime Asset Path
`resource_path.h` hardcodes:
```cpp
const std::string resource_path = "elements//";
```
So the executable must see `./elements/…` at runtime. Options:
- Run from repo root.
- Copy `elements/` next to produced binary.
- Change to an absolute path or use macOS bundle resource logic (future improvement).

---
## 9. Online Multiplayer Mode
The game now supports online multiplayer! When you start the game, you'll see a mode selection screen:

### Game Modes
1. **Local Multiplayer** - Original mode with two players on the same keyboard
2. **Host Game** - Start a server and wait for a friend to connect
3. **Join Game** - Connect to a friend's hosted game

### How to Play Online
**Player 1 (Host):**
1. Launch the game and select "2. Host Game"
2. The game will listen on port 53000
3. Wait for Player 2 to connect
4. Once connected, the game will start automatically

**Player 2 (Client):**
1. Launch the game and select "3. Join Game"
2. Enter the host's IP address (use `127.0.0.1` for local testing)
3. Press Enter to connect
4. Once connected, you'll join the game

### Network Controls
- **Host (Player 1)**: Controls the Rocket using Arrow keys (Right arrow to attack)
- **Client (Player 2)**: Controls the Robot using WASD (Space to attack)

### Finding Your IP Address
To play over a network (not on the same computer):
- **macOS/Linux**: Run `ifconfig` or `ip addr` in terminal
- **Windows**: Run `ipconfig` in command prompt
- Look for your local network IP (usually starts with 192.168.x.x or 10.x.x.x)

### Network Requirements
- Both players must be on the same network OR
- Host must forward port 53000 if playing over the internet
- Firewall may need to allow connections on port 53000

---
## 10. Controls (Observed From Source)
**Local Mode:**
- Player 1 (Rocket): Arrow keys (Right key triggers attack), Numpad 4/6/8/5 also mapped for movement
- Player 2 (Robot): WASD movement, Space triggers attack

**Online Mode:**
- Host: Controls Player 1 (Rocket) with Arrow keys
- Client: Controls Player 2 (Robot) with WASD

**Common Controls:**
- `O` slow down movement speed
- `P` speed up movement speed
- `K` skip cooldown
- `Escape` closes the main game window
- Menu interactions: Mouse over Start/Quit images and click, or press 1/2/3 for mode selection

---
## 11. Troubleshooting (macOS)
| Issue | Cause | Fix |
|-------|-------|-----|
| `dyld: Library not loaded: libsfml-graphics.*.dylib` | Runtime loader can't locate SFML dylibs | Add runpath: compile with `-Wl,-rpath,"$(brew --prefix)/lib"` OR export `DYLD_LIBRARY_PATH=$(brew --prefix)/lib`. In CMake set `INSTALL_RPATH`. |
| Black / empty window | Asset load failure or wrong working dir | Run from repo root or confirm `elements/` exists. Check console messages. |
| Fonts/music not found | Wrong resource path | Verify filenames & case (macOS is usually case‑insensitive but Git may differ). |
| High CPU usage | No frame limiting (game runs as fast as possible) | Add `sf::sleep` or `setFramerateLimit(60)` on the window. |
| Window scaled oddly / blurry on Retina | Default coordinate scaling | Optionally call `window.setView(sf::View(sf::FloatRect(0,0,width,height)));` or let macOS scale; adjust assets if blurry. |
| Input lag with key repeat | Using event + boolean toggles without handling key repeat intervals | Acceptable for prototype; to refine, handle `KeyPressed` vs `KeyReleased` distinctly and skip toggling on repeat. |
| Start screen recursion (calling `startgame()` repeatedly) | Design uses recursive call after closing end screen | Acceptable for small scope; for long sessions consider refactoring to a single main loop to avoid deep recursion. |

Additional diagnostics:
```bash
otool -L dungeon_game        # List linked dylibs
brew info sfml               # Confirm install paths
```

---
## 12. Suggested Improvements (Future Work)
- Introduce a unified game state loop (menu, play, end) rather than recursive `startgame()` calls.
- Replace hardcoded asset dimensions (sprite sheet frame calculations) with metadata.
- Add frame limiting or vertical sync (`m_window.setFramerateLimit(60)` or `m_window.setVerticalSyncEnabled(true)`).
- Convert to CMake + package config entirely and remove legacy `.vcxproj` files.
- Introduce error handling / asserts for failed loads beyond `std::cout` messages.
- Bundle resources using an app bundle (`.app`) and relative `Resources/` path for macOS distribution.

---
## 13. License Notes
Fonts and other third‑party assets inside `elements/` may carry their own licenses (e.g. SIL Open Font License). Review and preserve any included license files when distributing.

---
## 14. Quick Reference Commands
```bash
# Install dependencies
brew install sfml

# Build (simple)
clang++ -std=c++17 main.cpp Game.cpp AnimatedGameObject.cpp RegularGameObject.cpp NetworkManager.cpp \
  -I"$(brew --prefix)/include" -L"$(brew --prefix)/lib" \
  -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lsfml-network \
  -Wl,-rpath,"$(brew --prefix)/lib" \
  -o dungeon_game

# Run from project root
./dungeon_game

# If dylib not found
export DYLD_LIBRARY_PATH="$(brew --prefix)/lib"
./dungeon_game
```
</details>

### Building Release
```bash
# 0. start clean
mkdir -p dist
cp dungeon_game dist/
cp -R elements dist/
cd dist

# 1. put bundled dylibs in a subfolder (e.g., libs)
mkdir -p libs

# 2. bundle & rewrite load paths to @executable_path/libs
#    (note: no -oD on "." — we're using ./libs)
dylibbundler -b -x ./dungeon_game -d ./libs -p @executable_path/libs

# 3. sanity check: all non-system deps should point at @executable_path/libs
otool -L ./dungeon_game
otool -L ./libs/libsfml-graphics*.dylib
If the checks look good:


cd ..
zip -r dungeon_game_mac.zip dist
```