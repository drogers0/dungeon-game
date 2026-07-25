# CLAUDE.md

Guidance for Claude Code (and, via the `AGENTS.md` symlink, Codex / Copilot / other agents)
working in this repository. Keep this file concise; link out to [docs/](docs/) for depth.

## What this repo is

A **local two-player dungeon fighting game** built in C++ on **SFML 3**. Two sprite-animated
fighters trade sword/laser attacks in an arena while dodging fire hazards; first to out-score
the other before the clock runs out wins. It runs as a native desktop window (macOS / Linux /
Windows) — there is no server component beyond optional peer-to-peer online play.

The game supports several **control sources** that all feed the same movement/attack logic:
local shared-keyboard (player 1 numpad, player 2 WASD), online host/client over TCP, a
single-player AI opponent (Easy/Medium/Hard), and a scripted replay/debug harness. See
[docs/architecture.md](docs/architecture.md).

## Build, run, test

The build system is **CMake** (≥3.16). SFML 3.0.2 is fetched and pinned via `FetchContent`, so
no system SFML install is required (a `find_package` fallback is used if a compatible SFML 3 is
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

- **C++17**, warnings-as-signal (`-Wall -Wextra -Wpedantic`; `-Werror` in CI).
- Format with `clang-format` and lint with `clang-tidy` (configs at repo root) before committing.
  Match CI's **clang-format 18** — Apple's bundled v17 formats differently; use a pinned v18 binary
  (see [docs/building.md](docs/building.md#warnings--tooling)).
- Prefer RAII and value/smart-pointer ownership over raw `new`/`delete`.
- Keep gameplay/AI/network logic **separable from rendering** so it is unit-testable without a
  window (SFML opens a real window + audio device). See [docs/architecture.md](docs/architecture.md).
- Follow existing patterns; if a pattern looks wrong or fragile, flag it rather than copying it.

## Design decisions (don't "fix" these without reason)

- **SFML pinned at 3.0.2 via FetchContent**, not the system package. Reproducible across
  machines/CI and matches the SFML 3.x API the code uses. Do not install a system SFML 3
  locally — it would make `find_package` skip FetchContent and diverge from CI.
- **`PlayerInput` is the single input abstraction.** Local input, network packets, the AI
  opponent, and the replay/debug harness all produce a `PlayerInput` per frame that flows through
  one code path — do not special-case per-mode behavior in the update loop.
- **Online play is peer-to-peer** (one player hosts, the other joins); the host is authoritative
  for shared state. There is no dedicated server.

## Load-bearing quirks (don't "fix" without understanding)

These look wrong but are intentional; changing them breaks gameplay in non-obvious ways.

- **`geometry.h` negative-width rects.** `objectBounds()` yields a negative `size.x` for
  left-facing (scale.x < 0) objects; collision relies on SFML 3's `findIntersection()`
  normalising internally (verified in `Rect.inl`). Do not "fix" `objectBounds()`.
  `normalizedBounds()` is the positive-extent variant for edge math only — not collision.
- **`sprite_anim.h` int-ceil.** `advanceFrameRect` pins `(int)ceil(curr/nx)` (integer division
  *before* ceil) — the frame-advance math depends on it.
- **Score field inversion.** `captureGameState` sets `p1_score = p2points` (rocket/P1) and
  `p2_score = points` (robot/P2). Counterintuitive — verify direction before touching scoring.
- **Scale-aware wrap/bounds math.** `getWidth()/getHeight()` return the *unscaled* frame size,
  but the **rocket renders at scale ±2, the robot at ±1**. Any screen-wrap / edge / hazard math
  must multiply by scale (`scale.x` flips sign with facing → use `std::abs`; `scale.y` stays +2).
  This bug class bit both the Y-wrap (#28d) and X-wrap (#33).
- **`std::optional<sf::Sprite/Sound/Text>` members.** SFML 3 removed these types' default ctors,
  so they are held as optionals and `emplace()`d after the asset loads. The invariant
  `m_valid ⟺ m_sprite.has_value()` is load-bearing: `changeValid(true)` must `emplace(m_texture)`
  or the `m_valid`-guarded mutators dereference an empty optional (UB).
- **The `apieceofcrap` variable name** (the minutes accumulator in `Game.cpp`). Do not rename it
  to something sensible — it was deliberately restored (#30) and is considered load-bearing for
  reasons no one has been able to establish. Leave it exactly as is.

## Working in this repo

- **Merge convention:** feature branches off `master`, **squash-merged to `master`** via PR. One
  PR per coherent effort (may span multiple issues). See [docs/contributing.md](docs/contributing.md).
- Run the four gates before declaring a change done: **build**, **clang-format --dry-run**,
  **clang-tidy**, **ctest** — all must pass. CI enforces the same on every PR across 3 platforms.
- `dungeon_lib` is **one `-Werror` static library** built from all of `src/*.cpp`, and the test
  targets link it — so a broad edit leaves the whole tree red until *everything* compiles (no
  partial `ctest` mid-refactor).
- **Local test etiquette:** `ctest -L unit` is headless and safe. The full/integration suite
  constructs `Game`, which opens a **real macOS window** (no xvfb locally) and steals focus —
  validate windowed behaviour on CI, not locally. Build + clang-format + unit are the safe local
  gates. `TEST_CASE` names must be **ASCII-only** (Windows CTest mangles non-ASCII → "no tests
  matched").
- This game opens a real window; for automated/headless testing use the debug harness
  (screenshots + scriptable input + `--frames N`) — see
  [docs/contributing.md](docs/contributing.md#automated-playtesting).
