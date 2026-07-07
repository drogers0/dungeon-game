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

- Framework: **Catch2** v3.5.4 via CMake `FetchContent`; targets are registered with `ctest`.
- Two test binaries:
  - `dungeon_tests` — **unit tests** (label `unit`): socket-free netcode, harness/replay
    parser, extracted free functions (`objectBounds`, `advanceFrameRect`, `applyInputToP1`),
    and loopback socket tests. No display required.
  - `dungeon_integration_tests` — **integration tests** (label `integration`): construct a real
    `Game`, run via replay scripts, assert on `snapshot()` and the `run()` return tuple. Requires
    a display (real window on macOS; `xvfb-run` on Linux CI).
- Run unit tests only (no display):

  ```bash
  ctest --test-dir build -L unit --output-on-failure
  ```

- Run the full suite (macOS opens real windows; Linux use `xvfb-run`):

  ```bash
  ctest --test-dir build --output-on-failure
  # or on Linux headless:
  xvfb-run -a ctest --test-dir build --output-on-failure
  ```

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
  --replay-p1 tests/data/p1_laser.replay \  # drive P1 from replay script
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
0 0 0 1 0     # P2 moves right  (or P1 moves right if used with --replay-p1)
0 0 0 0 1     # P2 attacks      (or P1 fires laser if used with --replay-p1)
0 0 0 0 0     # idle
```

The same file format drives both `--replay` (P2/robot) and `--replay-p1` (P1/rocket).
Field order is identical; the bindings differ:

| Field    | `--replay` (P2) | `--replay-p1` (P1) |
|----------|-----------------|--------------------|
| `up`     | `w` (P2 up)     | `m_up` (P1 up)     |
| `down`   | `s` (P2 down)   | `m_down` (P1 down) |
| `left`   | `a` (P2 left)   | `m_left` (P1 left) |
| `right`  | `d` (P2 right)  | `m_right` (P1 right) |
| `attack` | `space` (P2 sword) | `right` (P1 laser) |

A malformed line (wrong field count or a value other than `0`/`1`) is skipped with a single
`std::cerr` warning; parsing always completes. If the replay ends before `--frames N` steps are
done, the remaining steps receive an all-zero `PlayerInput` (both players idle).

### Determinism guarantee

Two runs with the same `--replay`, `--replay-p1`, `--frames`, and `--seed` produce byte-identical
screenshots. Verified via `md5` / `shasum` on the output PNGs. The invariants are:

- Fixed timestep (`kFixedDt = 1/60 s`) — no wall-clock randomness in the sim.
- Replay scripts drive players; keyboard is disabled when any harness flag is active.
- `rng::seed()` seeds the shared `std::mt19937`; gameplay currently has no RNG, so the seed
  matters only for future stochastic features (AI mistake-rate, etc.).

### F12 manual screenshot

Press **F12** at any time during a running game to save
`<screenshotDir>/manual_<step>.png` to the configured screenshot directory (default: `.`).

A scripted playthrough doubles as an integration test (#7).

## Coverage

Build with LLVM coverage instrumentation and report:

```bash
# Configure with coverage (clang required)
cmake -B covbuild \
  -DCMAKE_CXX_COMPILER=$(which clang++) \
  -DDUNGEON_BUILD_TESTS=ON \
  -DDUNGEON_COVERAGE=ON

cmake --build covbuild -j

# Run tests (Linux: prepend xvfb-run -a)
mkdir -p covbuild/cov
LLVM_PROFILE_FILE=$PWD/covbuild/cov/%p-%m.profraw \
  ctest --test-dir covbuild --output-on-failure

# Merge profiles and report
llvm-profdata merge covbuild/cov/*.profraw -o covbuild/cov/merged.profdata
llvm-cov report \
  covbuild/dungeon_tests covbuild/dungeon_integration_tests \
  -instr-profile=covbuild/cov/merged.profdata \
  -ignore-filename-regex='main\.cpp|tests/'
```

The `DUNGEON_COVERAGE=ON` flag adds `-fprofile-instr-generate -fcoverage-mapping` as PUBLIC
flags on `dungeon_lib`, so both test binaries automatically receive the instrumentation.
The `mkdir -p covbuild/cov` step is required — LLVM's profiling runtime does not create
missing directories and will silently drop `.profraw` files otherwise.

Coverage target for `dungeon_lib` is **≥80%** with honest exclusions (`main.cpp` menus,
`render()` draw calls, `processEvents`/`handlePlayerInput` keyboard glue). Reported in CI.

## CI

The GitHub Actions workflow has two jobs:

- **Linux** (`ubuntu-latest`): installs `clang llvm xvfb`, builds with
  `-DCMAKE_CXX_COMPILER=clang++ -DDUNGEON_BUILD_TESTS=ON -DDUNGEON_COVERAGE=ON`, runs the
  full test suite under `xvfb-run -a`, then reports LLVM coverage.
- **macOS** (`macos-latest`): standard build with `-DDUNGEON_BUILD_TESTS=ON`, then
  `ctest -L unit` only (SFML windows on GH macOS runners are flaky).

PRs must be green before merge.
