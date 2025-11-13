# Dungeon Game

A small two‑player arcade style prototype built on **SFML 2**. Players control a rocket and a robot, earn points via collisions / attacks, and the game features animated sprites, sounds, music, and a simple start/end screen loop.

This repository originated from a Windows Visual Studio project (`.vcxproj`). This README covers building and running on both **Windows** and **macOS**.

**Quick Navigation:**
- [Windows Build Instructions](#windows-build-instructions) (Visual Studio, MSVC, MinGW)
- [macOS Build Instructions](#macos-build-instructions) (Homebrew + clang++)
- [Building a Release](#building-a-release)

Note about SFML versions:
- This code works with SFML 2.x (SFML 2.5.1 or later recommended)
- Homebrew on macOS may install SFML 3 by default as of late 2025 - use `sfml@2` to get version 2.x

---
## 1. Project Structure
```
AnimatedGameObject.*    Sprite sheet animation helper
RegularGameObject.*     Simple textured sprite wrapper
Game.*                  Core game loop, input handling, scoring, collisions
GameObject.h            Base interface (implied by usage) for drawable/movable objects
main.cpp                Start menu + end screen loop; calls Game::run()
resource_path.h         Defines resource_path = "elements//" for asset loading
elements/               Asset root (images, audio, fonts, sprite sheets)
content/                (Present but unused in current code – legacy folder)
SkeletonCode.vcxproj*   Visual Studio project files for Windows builds
```
All asset loads are performed via `resource_path + filename`, meaning the **working directory must be the project root** (so that `elements/` resolves). If you run from another directory (e.g. inside `build/`), either copy the `elements/` folder next to the executable or adjust `resource_path.h`.

---
## 2. SFML Modules Used
The source includes and APIs from:
- Graphics (`#include <SFML/Graphics.hpp>`) – windows, sprites, fonts, text
- Audio (`sf::Music`, `sf::Sound`, `sf::SoundBuffer`)
- Window / Events (`sf::RenderWindow`, `sf::Event`)
- System (`sf::Clock`, `sf::Time`)

You must link: `sfml-graphics sfml-window sfml-system sfml-audio`.

---
# Windows Build Instructions

## 3. Prerequisites (Windows)

### Option A: Visual Studio (Recommended for Windows)
1. **Install Visual Studio** (2017 or later):
   - Download from https://visualstudio.microsoft.com/
   - Select "Desktop development with C++" workload
   - Visual Studio 2017 (v141 toolset) or later is supported

2. **Download and Install SFML 2.x**:
   - Download SFML 2.5.1 (or latest 2.x) from https://www.sfml-dev.org/download.php
   - Choose the version matching your Visual Studio version:
     - Visual Studio 2017: Use SFML Visual C++ 15 (2017) - 32-bit or 64-bit
     - Visual Studio 2019: Use SFML Visual C++ 15 (2017) - 32-bit or 64-bit (compatible)
     - Visual Studio 2022: Use SFML Visual C++ 17 (2022) - 32-bit or 64-bit
   - Extract to `C:\SFML\SFML-2.5.1\` (or update paths in the `.vcxproj` file)

3. **SFML Directory Structure** should look like:
   ```
   C:\SFML\SFML-2.5.1\
   ├── include\
   │   └── SFML\
   │       ├── Graphics.hpp
   │       ├── Audio.hpp
   │       └── ...
   └── lib\
       ├── sfml-graphics.lib
       ├── sfml-graphics-d.lib  (debug version)
       └── ...
   ```

### Option B: MinGW-w64 (Command Line Build)
1. **Install MinGW-w64**:
   - Download from https://www.mingw-w64.org/ or use MSYS2
   - Add MinGW `bin` directory to your PATH

2. **Download SFML for MinGW**:
   - Get SFML 2.5.1 MinGW version from https://www.sfml-dev.org/download.php
   - Extract to a known location (e.g., `C:\SFML\SFML-2.5.1-mingw\`)

---
## 4. Building with Visual Studio

### Using the Visual Studio IDE:
1. Open `SkeletonCode.vcxproj` in Visual Studio
2. **Update SFML paths if needed** (if not using `C:\SFML\SFML-2.5.1\`):
   - Right-click project → Properties
   - Under C/C++ → General → Additional Include Directories: Update SFML include path
   - Under Linker → General → Additional Library Directories: Update SFML lib path
3. Select configuration:
   - **Debug** (Win32 or x64) for development
   - **Release** (Win32 or x64) for final builds
4. Build → Build Solution (or press F7)
5. **Copy SFML DLLs** to the output directory:
   - From `C:\SFML\SFML-2.5.1\bin\`, copy these DLLs next to your `.exe`:
     - `sfml-graphics-2.dll`
     - `sfml-window-2.dll`
     - `sfml-system-2.dll`
     - `sfml-audio-2.dll`
     - `openal32.dll` (for audio)
   - For **Debug** builds, use debug DLLs (e.g., `sfml-graphics-d-2.dll`)
6. Ensure `elements/` folder is accessible from the `.exe` location
7. Run the game from Visual Studio (F5) or double-click the `.exe`

### Using MSBuild (Command Line):
```cmd
REM Open Developer Command Prompt for Visual Studio
cd path\to\dungeon-game

REM Build Release (Win32)
msbuild SkeletonCode.vcxproj /p:Configuration=Release /p:Platform=Win32

REM Build Release (x64)
msbuild SkeletonCode.vcxproj /p:Configuration=Release /p:Platform=x64

REM Copy SFML DLLs to output directory
copy C:\SFML\SFML-2.5.1\bin\*.dll Release\

REM Run
cd Release
SkeletonCode.exe
```

---
## 5. Building with MinGW (Command Line)

From the project root (Windows Command Prompt or PowerShell):
```bash
g++ -std=c++17 ^
  main.cpp Game.cpp AnimatedGameObject.cpp RegularGameObject.cpp ^
  -I"C:\SFML\SFML-2.5.1-mingw\include" ^
  -L"C:\SFML\SFML-2.5.1-mingw\lib" ^
  -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio ^
  -O2 -o dungeon_game.exe

REM Copy SFML DLLs to current directory
copy C:\SFML\SFML-2.5.1-mingw\bin\*.dll .

REM Run
dungeon_game.exe
```

**Note:** Make sure SFML DLLs are in the same directory as your executable or in your system PATH.

---
## 6. Windows Runtime Requirements

For distributing your Windows build, include these files:
```
dungeon_game.exe (or SkeletonCode.exe)
elements/              (all game assets - images, audio, fonts)
sfml-graphics-2.dll
sfml-window-2.dll
sfml-system-2.dll
sfml-audio-2.dll
openal32.dll
```

**Visual C++ Runtime:** Users need the appropriate Visual C++ Redistributable installed:
- For Visual Studio 2017/2019: [VC++ 2017 Redistributable](https://aka.ms/vs/16/release/vc_redist.x64.exe)
- For Visual Studio 2022: [VC++ 2022 Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

---
## 7. Windows Troubleshooting

| Issue | Cause | Fix |
|-------|-------|-----|
| `SFML headers not found` during compile | Include path not set correctly | Update Additional Include Directories in VS project or `-I` flag in command line |
| `Cannot open sfml-graphics.lib` during link | Library path not set correctly | Update Additional Library Directories in VS project or `-L` flag in command line |
| `Missing sfml-graphics-2.dll` at runtime | DLLs not in PATH or exe directory | Copy all required SFML DLLs next to your `.exe` file |
| `MSVCP140.dll missing` | Visual C++ Runtime not installed | Install Visual C++ Redistributable for your VS version |
| Window opens then closes immediately | Working directory doesn't contain `elements/` folder | Run from project root or copy `elements/` folder next to `.exe` |
| Black/empty window or missing assets | Asset loading failures due to incorrect path | Ensure `elements/` folder is in the working directory |
| Build errors about platform toolset | VS version mismatch | Update project properties Platform Toolset to match your installed VS version |

**Check DLL dependencies:**
```cmd
REM Use Dependency Walker or dumpbin to check what DLLs are required
dumpbin /dependents SkeletonCode.exe
```

---
# macOS Build Instructions

## 8. Prerequisites (macOS)
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
## 9. Quick Build & Run (One‑liner)
From the project root (so `elements/` is visible):
```bash
clang++ -std=c++17 \
  main.cpp Game.cpp AnimatedGameObject.cpp RegularGameObject.cpp \
  -I"$(brew --prefix sfml@2)/include" \
  -L"$(brew --prefix sfml@2)/lib" \
  -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio \
  -Wl,-rpath,"$(brew --prefix sfml@2)/lib" \
  -O2 -o dungeon_game

./dungeon_game
```


<details>

<summary>Everything further is ai gerated and untested</summary>

---
## 10. Recommended: Separate Build Directory
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
## 11. CMake Support (Optional)
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
)

# Find SFML (Homebrew install). Accept either 2.x or 3.x
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};$(brew --prefix sfml)")
find_package(SFML REQUIRED COMPONENTS graphics window system audio)

add_executable(dungeon_game ${SOURCES})

target_link_libraries(dungeon_game PRIVATE sfml-graphics sfml-window sfml-system sfml-audio)

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
## 12. Xcode IDE Setup (Manual Linking)
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
## 13. Runtime Asset Path
`resource_path.h` hardcodes:
```cpp
const std::string resource_path = "elements//";
```
So the executable must see `./elements/…` at runtime. Options:
- Run from repo root.
- Copy `elements/` next to produced binary.
- Change to an absolute path or use macOS bundle resource logic (future improvement).

---
## 14. Controls (Observed From Source)
Player 1 (Robot): Arrow keys (Right key triggers attack), Numpad 4/6/8/5 also mapped for movement.
Player 2 (Rocket): WASD movement, Space triggers attack.
Other keys:
- `O` slow down movement speed.
- `P` speed up movement speed.
- `K` skip cooldown.
- `Escape` closes the main game window.
- Menu interactions: Mouse over Start/Quit images and click.

---
## 15. Troubleshooting (macOS)
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
## 16. Suggested Improvements (Future Work)
- Introduce a unified game state loop (menu, play, end) rather than recursive `startgame()` calls.
- Replace hardcoded asset dimensions (sprite sheet frame calculations) with metadata.
- Add frame limiting or vertical sync (`m_window.setFramerateLimit(60)` or `m_window.setVerticalSyncEnabled(true)`).
- Convert to CMake + package config entirely and remove legacy `.vcxproj` files.
- Introduce error handling / asserts for failed loads beyond `std::cout` messages.
- Bundle resources using an app bundle (`.app`) and relative `Resources/` path for macOS distribution.

---
## 17. License Notes
Fonts and other third‑party assets inside `elements/` may carry their own licenses (e.g. SIL Open Font License). Review and preserve any included license files when distributing.

---
## 18. Quick Reference Commands (macOS)
```bash
# Install dependencies
brew install sfml

# Build (simple)
clang++ -std=c++17 main.cpp Game.cpp AnimatedGameObject.cpp RegularGameObject.cpp \
  -I"$(brew --prefix)/include" -L"$(brew --prefix)/lib" \
  -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio \
  -Wl,-rpath,"$(brew --prefix)/lib" \
  -o dungeon_game

# Run from project root
./dungeon_game

# If dylib not found
export DYLD_LIBRARY_PATH="$(brew --prefix)/lib"
./dungeon_game
```
</details>

---
# Building a Release

## 19. Building a Release for Windows

### Using Visual Studio:
1. **Set Configuration to Release**:
   - Select "Release" and target platform (Win32 or x64) from the toolbar
   
2. **Build the Release**:
   ```cmd
   REM In Developer Command Prompt
   msbuild SkeletonCode.vcxproj /p:Configuration=Release /p:Platform=x64
   ```
   Or press Ctrl+Shift+B in Visual Studio IDE

3. **Create Distribution Folder**:
   ```cmd
   mkdir dist
   cd dist
   
   REM Copy executable
   copy ..\x64\Release\SkeletonCode.exe dungeon_game.exe
   
   REM Copy game assets
   xcopy /E /I ..\elements elements
   
   REM Copy SFML DLLs (Release versions - no "-d" suffix)
   copy C:\SFML\SFML-2.5.1\bin\sfml-graphics-2.dll .
   copy C:\SFML\SFML-2.5.1\bin\sfml-window-2.dll .
   copy C:\SFML\SFML-2.5.1\bin\sfml-system-2.dll .
   copy C:\SFML\SFML-2.5.1\bin\sfml-audio-2.dll .
   copy C:\SFML\SFML-2.5.1\bin\openal32.dll .
   ```

4. **Optional: Check Dependencies**:
   ```cmd
   dumpbin /dependents dungeon_game.exe
   ```

5. **Create Distribution Archive**:
   ```cmd
   cd ..
   powershell Compress-Archive -Path dist\* -DestinationPath dungeon_game_windows.zip
   ```
   Or use 7-Zip, WinRAR, etc.

### Distribution Package Contents:
```
dungeon_game_windows.zip/
├── dungeon_game.exe
├── elements/
│   ├── (all PNG, WAV, TTF files)
├── sfml-graphics-2.dll
├── sfml-window-2.dll
├── sfml-system-2.dll
├── sfml-audio-2.dll
└── openal32.dll
```

### Notes for Windows Distribution:
- Users will need Visual C++ Redistributable installed
- Consider including a README.txt with instructions
- Test on a clean Windows machine without Visual Studio installed
- For x64 builds, use x64 versions of SFML DLLs
- For x86 (Win32) builds, use x86 versions of SFML DLLs

---
## 20. Building a Release for macOS

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