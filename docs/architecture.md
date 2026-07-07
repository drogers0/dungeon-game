# Architecture

A conceptual map of the game. File paths are given where stable, but the emphasis is on
concepts that survive refactors (the physical layout is being reorganized under issue #8).

## The game in one paragraph

Two fighters share one arena: **player 1 ("rocket")** and **player 2 ("robot")**. Each can move
in four directions, face left/right, and throw a melee/ranged attack that sweeps a rectangle in
their facing direction. Landing an attack on the opponent scores a point and triggers a short
**cooldown** ("intermission") that recenters both fighters. Standing in a **fire hazard** costs a
point (with brief post-hit invincibility). A running clock is displayed; when the window closes
the final scores + elapsed time are returned to the menu, which tracks a high score.

## Control sources → one input path

Every mode ultimately drives the same per-player movement/attack booleans consumed in
`Game::update`. This uniformity is the central design invariant:

- **Local** — a shared keyboard: player 1 on the numpad (+ arrow to attack), player 2 on WASD
  (+ space to attack). Handled in `Game::handlePlayerInput`.
- **Online** — one peer hosts, the other joins over TCP. The client samples its keyboard into a
  `PlayerInput`, sends it to the host; the host simulates authoritatively and streams `GameState`
  back. See `NetworkManager`.
- **AI (#12)** — the pure `decideAiInput(AiView, AiParams, rng) → PlayerInput` decider drives
  player 2 from a snapshot of game state, with Easy/Medium/Hard difficulty presets. See `ai.h`/`ai.cpp`.
- **Replay/debug (#13)** — the debug harness feeds a scripted sequence of `PlayerInput`s (loaded via
  `--replay`/`--replay-p1`) through the same path. See `replay.h`/`replay.cpp` and `debug.h`.

`PlayerInput` (`NetworkManager.h`) is the shared abstraction. New control sources should produce
a `PlayerInput` per frame rather than reaching into `Game` internals.

## Entity model

`GameObject` (`GameObject.h`) is an abstract interface: `load`, `draw`, `update`, position/scale,
validity, origin. Two implementations:

- **`RegularGameObject`** — a single static sprite (walls, backgrounds, menu art).
- **`AnimatedGameObject`** — a sprite sheet advanced frame-by-frame in `update()`; constructed with
  frame dimensions + count. Players, fire, and menu buttons are animated objects.

Objects are currently created with raw `new` and stored as `GameObject*` (including the `stuff`
vector of arena props). Ownership is being moved to smart pointers / RAII (issue #9); note the base
class currently has **no virtual destructor**.

## The game loop

`Game::run()` drives a loop while the window is open:

1. **`processEvents()`** — pump SFML events; key up/down toggles per-player input booleans.
2. **`update(deltaT, time)`** — apply movement (`speed * deltaT`, frame-rate independent),
   resolve player↔player push-out, evaluate attacks (a facing-direction hit rectangle vs. the
   opponent), and set the cooldown flag on a scoring hit.
3. **Hazard damage** is resolved back in `run()` itself (not `update()`): after `update()` and
   before `render()`, `run()` tests each player against the hazards and decrements score with a
   per-player invincibility window.
4. **`render()`** — clear, draw hazards/props, HUD (scores + timer), both players, then display.

> Note the split: `update()` handles movement/attacks/cooldown-flag, while `run()` owns the
> per-frame timing, hazard-damage loop, and cooldown countdown. When extending hazard logic, edit
> `run()`, not `update()`.

**Timing caveats (issue #11):** movement scales with `deltaT` (good), but sprite animation uses a
`time > .1f` reset accumulator (a frame-rate-sensitive hack), and the window is a fixed
non-resizable `1920x1080` while menus are `1024x576`. Fixed-timestep + a scalable view are the
intended fixes, and are also what makes deterministic replay (#13) and AI testing (#12) possible.

## Scoring, hazards, cooldown

- **Attacks:** each attack builds a `sf::Rect` extending from the attacker in its facing direction
  and tests overlap with the opponent (`Game::collision(sf::Rect, GameObject*)`). A hit scores and
  sets `reset`, which recenters both fighters and enters `wait` (cooldown).
- **Hazards:** the `stuff` vector holds a wall + fire objects; touching one decrements score,
  gated by a per-player timeout so damage can't repeat every frame.
- **Cooldown (`wait`):** after a score, the loop pauses gameplay ~1.75s (showing an intermission
  banner) before resuming with a gong.

> Internal score naming is currently inverted/confusing (`points` tracks the robot/p2 score,
> `p2points` tracks the rocket/p1 score); this is a known smell (#9) — verify against the HUD
> strings and the `run()` return tuple before relying on either name.

## Network model

`NetworkManager` wraps an `sf::TcpListener` + `sf::TcpSocket`. `PlayerInput` and `GameState`
serialize via `sf::Packet`. The host is authoritative: it receives the client's `PlayerInput`,
simulates, and returns the full `GameState` each frame. Known hardening work (issue #2): packets
are demultiplexed by a leading type byte but mismatches are silently dropped, disconnection isn't
detected, full state is sent every frame with no interpolation, and join input isn't validated.

## Testability seams

SFML opens a real window and audio device, so **logic that must be unit-tested should not depend
on the window**: packet round-trips (`PlayerInput`/`GameState`), collision/overlap math, animation
frame indexing, `GameState` capture/apply, and AI decisions are all pure functions of
data and should be reachable without constructing a `Game`/window. CI runs the windowed paths
headless under `xvfb` on Linux (see [contributing.md](contributing.md)).
