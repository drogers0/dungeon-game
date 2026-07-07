# Building & running

## Prerequisites

- A C++17 compiler (Clang, GCC, or MSVC).
- **CMake ≥ 3.16.**
- Git (for `FetchContent` to pull SFML) and a network connection on the first configure.
- Platform toolchains for SFML's dependencies:
  - **macOS:** Xcode command-line tools. Homebrew optional.
  - **Linux:** X11/OpenGL/udev/FLAC/OpenAL/etc. dev packages (see the SFML docs; on Debian/Ubuntu
    the `libsfml-dev` build-deps cover it). CI installs these explicitly.
  - **Windows:** Visual Studio 2019+ (MSVC) — see issue #1 for the release path.

## SFML

SFML **2.6.2** is pinned and built via CMake `FetchContent`, so a system SFML install is **not**
required and every machine/CI runner builds against the same version. If a compatible SFML 2 is
already discoverable, a `find_package(SFML 2 ...)` fast path is used instead to skip the source
build. (A full SFML 3 migration is intentionally out of scope — the code targets the SFML 2.x API.)

The first configure fetches and compiles SFML, which takes a few minutes; subsequent builds reuse
the cached build tree.

## Configure, build, run

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug   # or Release
cmake --build build -j
./build/dungeon_game
```

Assets from [../assets/](../assets/) are copied next to the executable at build time, so the
game finds them regardless of the working directory. (Historically the game only ran from the repo
root because of a hardcoded relative asset path — that dependency has been removed.)

## Tests

```bash
ctest --test-dir build --output-on-failure
```

The suite uses Catch2 (fetched via `FetchContent`). Pure-logic tests (packets, collision,
animation math, state capture/apply, AI decisions) run anywhere; windowed integration tests run
headless in CI (below).

## Headless / CI

- **Linux:** run windowed binaries under a virtual framebuffer:
  ```bash
  xvfb-run -s "-screen 0 1920x1080x24" ./build/dungeon_game --frames 120 --screenshot-every 30
  ```
- **macOS:** there is no true headless mode (native Cocoa/OpenGL); a real window must open. Use a
  local window plus the debug harness (issue #13) for playtesting.

## Warnings & tooling

Builds enable `-Wall -Wextra -Wpedantic` (and `-Werror` in CI). Format with `clang-format` and
statically analyze with `clang-tidy` using the repo-root configs; install both via `brew install
llvm` on macOS or your distro's LLVM packages on Linux. See issue #6 and
[contributing.md](contributing.md).
