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

Because SFML has no API to inject synthetic events, automated testing reuses the game's own input
and rendering channels behind a debug flag/build (issue #13):

- **Scriptable input** — feed a sequence of `PlayerInput`s (`--replay <file>`) or drive player 2
  over the network client path; deterministic and focus-independent.
- **Screenshots** — capture the framebuffer to a file on a debug key and/or `--screenshot-every N`.
- **Determinism** — fixed timestep + seedable RNG + `--frames N` (run N frames then exit) make
  playthroughs reproducible: drive → capture → assess in a loop.

A scripted playthrough doubles as an integration test (#7).

## CI

A GitHub Actions workflow builds on macOS + Linux (+ Windows for releases, #1), runs
`clang-format` in check mode, `clang-tidy`, and the `ctest` suite (Linux under `xvfb`). PRs must be
green before merge.
