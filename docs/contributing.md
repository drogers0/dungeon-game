# Contributing

## Workflow

- Branch off `master`; open a PR; **squash-merge to `master`**. Keep one PR per coherent effort
  (a PR may resolve more than one issue).
- Read the relevant GitHub issue (#1–#13 track the current overhaul) before working in its area.
- Before declaring a change done, run the **four gates** locally — CI enforces the same:

```bash
cmake --build build -j                                   # 1. builds clean (warnings are errors in CI)
clang-format --dry-run --Werror $(git ls-files '*.cpp' '*.h')   # 2. formatting
clang-tidy -p build $(git ls-files '*.cpp')              # 3. static analysis
ctest --test-dir build --output-on-failure               # 4. tests
```

## Code style

- **C++17.** Format is defined by [`.clang-format`](../.clang-format); do not hand-format against
  it. Editor defaults come from [`.editorconfig`](../.editorconfig).
- Prefer RAII and smart-pointer/value ownership over raw `new`/`delete`. Give polymorphic base
  classes a virtual destructor.
- Prefer clear names over cleverness; avoid single-letter state and "joke" identifiers.
- Keep gameplay/network/AI logic separable from rendering so it stays unit-testable without a
  window. New control sources produce a `PlayerInput` (see [architecture.md](architecture.md)).
- Follow existing patterns; if one looks wrong or fragile, flag it rather than reproducing it.

## Testing

- Framework: **Catch2** via CMake `FetchContent`; targets are registered with `ctest`.
- Favor real dependencies over mocks; only mock what genuinely cannot run in a test.
- Target meaningful coverage of logic (packets, collision, animation indexing, state capture/apply,
  AI). Windowed paths are exercised headless in CI.

## Automated playtesting

The game supports a headless/scriptable harness activated by CLI flags (implemented in #13).
Pass any flag listed below to bypass menus entirely; the game constructs directly in LOCAL mode,
runs the specified number of steps, prints scores and step count, and exits.

```bash
./build/dungeon_game \
  --frames 300              \  # run exactly 300 sim steps then exit
  --replay tests/data/demo.replay \  # drive P2 from replay script
  --screenshot-every 60     \  # save PNG every 60 steps
  --screenshot-dir /tmp/out \  # directory for screenshots (created if absent)
  --seed 1                     # seed the RNG for reproducibility
```

### Replay format

Text file, one line per sim step:

```
# Lines starting with '#' are comments (ignored).
# Blank lines are ignored.
# Each data line has exactly 5 space-separated 0/1 fields:
#   up  down  left  right  attack
0 0 0 1 0     # P2 moves right
0 0 0 0 1     # P2 attacks
0 0 0 0 0     # P2 idle
```

A malformed line (wrong field count or a value other than `0`/`1`) is skipped with a single
`std::cerr` warning; parsing always completes. If the replay ends before `--frames N` steps are
done, the remaining steps receive an all-zero `PlayerInput` (both players idle).

### Determinism guarantee

Two runs with the same `--replay`, `--frames`, and `--seed` produce byte-identical screenshots.
Verified via `md5` / `shasum` on the output PNGs. The invariants are:

- Fixed timestep (`kFixedDt = 1/60 s`) — no wall-clock randomness in the sim.
- Replay script drives P2; keyboard is disabled when `--replay` / `--frames` is active.
- `rng::seed()` seeds the shared `std::mt19937`; gameplay currently has no RNG, so the seed
  matters only for future stochastic features (AI mistake-rate, etc.).

### F12 manual screenshot

Press **F12** at any time during a running game to save
`<screenshotDir>/manual_<step>.png` to the configured screenshot directory (default: `.`).

A scripted playthrough doubles as an integration test (#7).

## CI

A GitHub Actions workflow builds on macOS + Linux (+ Windows for releases, #1), runs
`clang-format` in check mode, `clang-tidy`, and the `ctest` suite (Linux under `xvfb`). PRs must be
green before merge.
