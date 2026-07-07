# CLAUDE.md

Guidance for Claude Code (and, via the `AGENTS.md` symlink, Codex / Copilot / other agents)
working in this repository. Keep this file concise; link out to [docs/](docs/) for depth.

## What this repo is

A **local two-player dungeon fighting game** built in C++ on **SFML 2.x**. Two sprite-animated
fighters trade sword/laser attacks in an arena while dodging fire hazards; first to out-score
the other before the clock runs out wins. It runs as a native desktop window (macOS / Linux /
Windows) — there is no server component beyond optional peer-to-peer online play.

The game supports several **control sources** that all feed the same movement/attack logic:
local shared-keyboard (player 1 numpad, player 2 WASD), online host/client over TCP, a
single-player AI opponent (Easy/Medium/Hard), and a scripted replay/debug harness. See
[docs/architecture.md](docs/architecture.md).

## Build, run, test

The build system is **CMake** (≥3.16). SFML 2.6.2 is fetched and pinned via `FetchContent`, so
no system SFML install is required (a `find_package` fallback is used if a compatible SFML 2 is
already present). Full details in [docs/building.md](docs/building.md).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug     # configure (first run fetches + builds SFML)
cmake --build build                          # build the dungeon_game executable
./build/dungeon_game                         # run
ctest --test-dir build --output-on-failure   # run the test suite
```

Assets in [assets/](assets/) are copied next to the binary at build time and resolved relative
to the executable, so the game runs from any directory.

## Repo layout

Sources live in `src/` (`.cpp`) and `include/` (`.h`); assets in `assets/`; tests in `tests/`.
The load-bearing pieces:

- `src/main.cpp` — entry point + menus (start/end screens, mode selection).
- `src/Game.cpp` / `include/Game.h` — the game: window, fixed game loop (`processEvents` → `update` → `render`),
  collision, scoring, audio, and network integration.
- `include/GameObject.h` — abstract entity interface, implemented by `RegularGameObject`
  (`RegularGameObject.{h,cpp}`, static sprite) and `AnimatedGameObject`
  (`AnimatedGameObject.{h,cpp}`, sprite-sheet animation) — each a separate file.
- `src/NetworkManager.cpp` / `include/NetworkManager.h` — TCP host/client transport; `PlayerInput` / `GameState` packet
  structs are the wire format and the uniform input channel across local/network/AI/replay.
- `include/resource_path.h` — asset path resolution.

## Conventions

- **C++17**, warnings-as-signal (`-Wall -Wextra -Wpedantic`; `-Werror` in CI). See issue #6.
- Format with `clang-format` and lint with `clang-tidy` (configs at repo root) before committing.
- Prefer RAII and value/smart-pointer ownership over raw `new`/`delete` (see issue #9).
- Keep gameplay/AI/network logic **separable from rendering** so it is unit-testable without a
  window (SFML opens a real window + audio device). See [docs/architecture.md](docs/architecture.md).
- Follow existing patterns; if a pattern looks wrong or fragile, flag it rather than copying it.

## Design decisions (don't "fix" these without reason)

- **SFML pinned at 2.6.2 via FetchContent**, not the system package. Reproducible across
  machines/CI and matches the SFML 2.x API the code uses. A full SFML 3 migration is a separate,
  out-of-scope effort.
- **`PlayerInput` is the single input abstraction.** Local input, network packets, the AI
  opponent, and the replay/debug harness all produce a `PlayerInput` per frame that flows through
  one code path — do not special-case per-mode behavior in the update loop.
- **Online play is peer-to-peer** (one player hosts, the other joins); the host is authoritative
  for shared state. There is no dedicated server.

## Working in this repo

- **Merge convention:** feature branches off `master`, **squash-merged to `master`** via PR. One
  PR per coherent effort (may span multiple issues). See [docs/contributing.md](docs/contributing.md).
- Run the four gates before declaring a change done: **build**, **clang-format --dry-run**,
  **clang-tidy**, **ctest** — all must pass. CI enforces the same on every PR.
- This game opens a real window; for automated/headless testing use the debug harness
  (screenshots + scriptable input + `--frames N`) — see issue #13 and
  [docs/contributing.md](docs/contributing.md#automated-playtesting).
- The overhaul in progress is tracked in GitHub issues #1–#13; read the relevant issue before
  starting work in its area.
