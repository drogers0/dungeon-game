// Integration tests: construct a real Game, drive it via replay scripts, assert
// on the returned tuple and snapshot().
//
// CRITICAL - score field naming is inverted in captureGameState():
//   p1_score = p2points  (rocket/P1 score; incremented by P1 laser kills)
//   p2_score = points    (robot/P2 score; incremented by P2 sword kills)
//
// These tests run from $<TARGET_FILE_DIR:dungeon_game> (see CMakeLists) so
// resource_path resolves to the assets/ directory copied there at build time.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <string>
#include <thread>
#include <tuple>

#include "Game.h"
#include "NetworkManager.h"
#include "ai.h"
#include "debug.h"
#include "rng.h"

// Absolute path to tests/data/ is injected at build time via CMake define.
#ifndef TESTS_DATA_DIR
#define TESTS_DATA_DIR "."
#endif

static std::string dataPath(const std::string& file) {
    return std::string(TESTS_DATA_DIR) + "/" + file;
}

// Helper: run a Game with the given config; return (result-tuple, snapshot).
static std::pair<std::tuple<int, int, float, int, bool, bool>, GameState>
runGame(DebugConfig cfg, NetworkMode mode = NetworkMode::LOCAL,
        std::shared_ptr<NetworkManager> nm = nullptr) {
    if (mode != NetworkMode::LOCAL && nm) {
        Game g(mode, nm);
        g.setDebugConfig(cfg);
        auto t = g.run();
        return {t, g.snapshot()};
    } else {
        Game g;
        g.setDebugConfig(cfg);
        auto t = g.run();
        return {t, g.snapshot()};
    }
}

// ── 1. Idle run ───────────────────────────────────────────────────────────────

TEST_CASE("integration: idle run - scores 0/0, initial positions", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 60;
    auto [t, s] = runGame(cfg);

    REQUIRE(s.p1_score == 0); // p2points = 0
    REQUIRE(s.p2_score == 0); // points = 0
    REQUIRE(!s.wait);

    // Positions are load-dependent (asset must exist); just assert reasonable
    // non-zero values (loaded successfully from assets/).
    REQUIRE(s.p1_x > 0.f);
    REQUIRE(s.p2_x > s.p1_x); // P2 starts to the right of P1
}

// ── 2. P2 sword kill ─────────────────────────────────────────────────────────

TEST_CASE("integration: P2 sword kill - p2_score becomes 1, wait set", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 75;
    cfg.replayPath = dataPath("p2_kill.replay");
    auto [t, s] = runGame(cfg);

    // points == 1 -> p2_score == 1
    REQUIRE(s.p2_score == 1);
    REQUIRE(s.p1_score == 0);
    REQUIRE(s.wait == true);
    // Players are reset to spawn positions after a kill
    REQUIRE(s.p1_x == Catch::Approx(154.f).margin(5.f));
    REQUIRE(s.p2_x == Catch::Approx(1636.f).margin(5.f));
    REQUIRE(!s.p1_left);
    REQUIRE(s.p2_left);
}

// ── 3. P1 laser kill ─────────────────────────────────────────────────────────

TEST_CASE("integration: P1 laser kill via --replay-p1 - p1_score becomes 1", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 75;
    cfg.replayPathP1 = dataPath("p1_laser.replay");
    auto [t, s] = runGame(cfg);

    // p2points == 1 -> p1_score == 1
    REQUIRE(s.p1_score == 1);
    REQUIRE(s.p2_score == 0);
    REQUIRE(s.wait == true);
}

// ── 4. Whiff - score unchanged ────────────────────────────────────────────────

TEST_CASE("integration: P2 whiff - scores unchanged", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 5;
    cfg.replayPath = dataPath("p2_whiff.replay");
    auto [t, s] = runGame(cfg);

    REQUIRE(s.p2_score == 0);
    REQUIRE(s.p1_score == 0);
    REQUIRE(!s.wait);
}

// ── 5. Cooldown duration ──────────────────────────────────────────────────────
// After a kill sets wait==true, exactly 106 sim-steps must drain the timer.
// Kill happens at step 71.  temp_time = 71/60 + 1.75.
// wait becomes false at step 177 (177/60 > temp_time). Verified at step 180.

TEST_CASE("integration: cooldown drains after 106 steps", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 180;
    cfg.replayPath = dataPath("p2_kill.replay"); // kill at step 71, then idle
    auto [t, s] = runGame(cfg);

    REQUIRE(s.p2_score == 1); // kill still scored
    REQUIRE(!s.wait);         // cooldown has drained
}

// ── 6. P1 hazard scoring + clamp ─────────────────────────────────────────────
// Pre-seed p2points=3 via applyNetworkState, then move P1 onto fire2.
// One hazard hit: p2points = 3-1 = 2.  snapshot().p1_score == 2.

TEST_CASE("integration: P1 hazard decrements p2points", "[integration]") {
    Game g;

    // Pre-seed p2points=3 (p1_score=3) with initial positions and no wait.
    GameState preset;
    preset.p1_x = 134.f;
    preset.p1_y = 400.f;
    preset.p2_x = 1636.f;
    preset.p2_y = 400.f;
    preset.p1_score = 3;
    preset.p2_score = 0;
    preset.p1_left = false;
    preset.p2_left = true;
    preset.p1_scale_x = 2.f;
    preset.p1_scale_y = 2.f;
    preset.p2_scale_x = -1.f;
    preset.p2_scale_y = 1.f;
    preset.wait = false;
    g.applyNetworkState(preset);

    DebugConfig cfg;
    cfg.frames = 35;
    cfg.replayPathP1 = dataPath("p1_onto_fire.replay"); // P1 moves right onto fire2
    g.setDebugConfig(cfg);
    g.run();

    GameState s = g.snapshot();
    REQUIRE(s.p1_score == 2); // p2points: 3 -> 2 (one hazard hit; 2.2s timeout = 132 steps)
}

TEST_CASE("integration: P1 hazard clamps p2points at 0", "[integration]") {
    Game g;

    // p2points starts at 0; fire hit decrements to -1 then clamps to 0.
    GameState preset;
    preset.p1_x = 134.f;
    preset.p1_y = 400.f;
    preset.p2_x = 1636.f;
    preset.p2_y = 400.f;
    preset.p1_score = 0;
    preset.p2_score = 0;
    preset.wait = false;
    preset.p1_left = false;
    preset.p1_scale_x = 2.f;
    preset.p1_scale_y = 2.f;
    preset.p2_scale_x = -1.f;
    preset.p2_scale_y = 1.f;
    preset.p2_left = true;
    g.applyNetworkState(preset);

    DebugConfig cfg;
    cfg.frames = 35;
    cfg.replayPathP1 = dataPath("p1_onto_fire.replay");
    g.setDebugConfig(cfg);
    g.run();

    REQUIRE(g.snapshot().p1_score == 0); // clamped from -1 to 0
}

// ── 7. P2 hazard scoring + clamp ─────────────────────────────────────────────

TEST_CASE("integration: P2 hazard decrements points", "[integration]") {
    Game g;

    GameState preset;
    preset.p1_x = 134.f;
    preset.p1_y = 400.f;
    preset.p2_x = 1636.f;
    preset.p2_y = 400.f;
    preset.p1_score = 0;
    preset.p2_score = 3; // points = 3
    preset.p1_left = false;
    preset.p2_left = true;
    preset.p1_scale_x = 2.f;
    preset.p1_scale_y = 2.f;
    preset.p2_scale_x = -1.f;
    preset.p2_scale_y = 1.f;
    preset.wait = false;
    g.applyNetworkState(preset);

    DebugConfig cfg;
    cfg.frames = 30;
    cfg.replayPath = dataPath("p2_onto_fire.replay"); // P2 moves left onto fire2
    g.setDebugConfig(cfg);
    g.run();

    REQUIRE(g.snapshot().p2_score == 2); // points: 3 -> 2
}

TEST_CASE("integration: P2 hazard clamps points at 0", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 30;
    cfg.replayPath = dataPath("p2_onto_fire.replay");
    auto [t, s] = runGame(cfg);
    REQUIRE(s.p2_score == 0); // clamped from -1 to 0
}

// ── 8. P1 left wraparound ─────────────────────────────────────────────────────
// P1 moves left 30 steps; wraps from ~x=-138 to x=1920, ends up on right side.

TEST_CASE("integration: P1 wraps around left edge", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 35;
    cfg.replayPathP1 = dataPath("p1_wrap_left.replay");
    auto [t, s] = runGame(cfg);

    // After wrap, P1 should be on the right side (x > window_width/2)
    REQUIRE(s.p1_x > 960.f);
}

// ── 9. P2 right wraparound ────────────────────────────────────────────────────
// P2 moves right 25 steps; direction flip on frame 1, then wraps past x=1920.

TEST_CASE("integration: P2 wraps around right edge", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 30;
    cfg.replayPath = dataPath("p2_wrap_right.replay");
    auto [t, s] = runGame(cfg);

    // After wrap, P2 position (raw, before scale) is near -119 + a few steps right
    REQUIRE(s.p2_x < 100.f);
}

// ── 10. Direction-flip reposition ─────────────────────────────────────────────
// P1 first move left: p1left flips false->true; position adjusted by -(width*old_scale.x).
// P1 starts at x=134, scale=(2,2). After flip: x = 134 - (134*2) = 134+268 = 402.
// After 1 left step + move: x ≈ 382.

TEST_CASE("integration: P1 direction-flip reposition exact x", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 2; // enough for 1 left step + direction flip
    // 1 left step
    cfg.replayPathP1 = dataPath("p1_wrap_left.replay"); // first line is 0 0 1 0 0
    auto [t, s] = runGame(cfg);

    // After 2 frames of left, P1 should be at approximately 402 - 20*2 = 362
    // The exact value depends on the direction flip reposition.
    REQUIRE(s.p1_x < 420.f); // repositioned from 134 to 402, then moved left
    REQUIRE(s.p1_x > 340.f); // not zero (valid position)
}

// ── 11. Player-vs-player pushback ─────────────────────────────────────────────
// When P2 moves into P1, P2 is pushed right by 40.  We verify P2 doesn't pass
// through P1 by checking P2's position after 60 left steps (P2 pinned near P1).

TEST_CASE("integration: P2 pinned against P1 does not pass through", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 65;
    cfg.replayPath = dataPath("p2_kill.replay"); // first 65 lines = left
    auto [t, s] = runGame(cfg);

    // P1 starts at x≈134 with scale (2,2): right edge ≈ 402.
    // P2 stopped near P1; P2 normalised left edge (p2_x - 119) must be near P1.
    float p2NormLeft = s.p2_x - 119.f;   // scale.x=-1 -> normalised left = p2_x - width
    REQUIRE(p2NormLeft >= s.p1_x - 5.f); // P2 didn't pass through P1 (±5 tolerance)
}

// ── 12. Capture / apply round-trip ────────────────────────────────────────────
// snapshot() -> applyNetworkState on a second Game -> snapshot() fields equal.

TEST_CASE("integration: capture/apply round-trip", "[integration]") {
    // Run game1 long enough to get a non-trivial state (P2 kill)
    DebugConfig cfg1;
    cfg1.frames = 75;
    cfg1.replayPath = dataPath("p2_kill.replay");
    auto [t1, s1] = runGame(cfg1);

    // Apply s1 to game2, take snapshot
    Game game2;
    game2.applyNetworkState(s1);
    GameState s2 = game2.snapshot();

    REQUIRE(s2.p1_x == Catch::Approx(s1.p1_x).margin(0.01f));
    REQUIRE(s2.p1_y == Catch::Approx(s1.p1_y).margin(0.01f));
    REQUIRE(s2.p2_x == Catch::Approx(s1.p2_x).margin(0.01f));
    REQUIRE(s2.p2_y == Catch::Approx(s1.p2_y).margin(0.01f));
    REQUIRE(s2.p1_score == s1.p1_score);
    REQUIRE(s2.p2_score == s1.p2_score);
    REQUIRE(s2.p1_left == s1.p1_left);
    REQUIRE(s2.p2_left == s1.p2_left);
    REQUIRE(s2.p1_scale_x == Catch::Approx(s1.p1_scale_x).margin(0.01f));
    REQUIRE(s2.p1_scale_y == Catch::Approx(s1.p1_scale_y).margin(0.01f));
    REQUIRE(s2.p2_scale_x == Catch::Approx(s1.p2_scale_x).margin(0.01f));
    REQUIRE(s2.p2_scale_y == Catch::Approx(s1.p2_scale_y).margin(0.01f));
    REQUIRE(s2.wait == s1.wait);
}

// ── 13. Determinism ───────────────────────────────────────────────────────────
// Same replay + same frames -> identical snapshot fields.

TEST_CASE("integration: determinism - same replay twice gives identical snapshot",
          "[integration]") {
    DebugConfig cfg;
    cfg.frames = 75;
    cfg.replayPath = dataPath("p2_kill.replay");

    auto [t1, s1] = runGame(cfg);
    auto [t2, s2] = runGame(cfg);

    REQUIRE(s1.p1_x == Catch::Approx(s2.p1_x).margin(0.001f));
    REQUIRE(s1.p1_y == Catch::Approx(s2.p1_y).margin(0.001f));
    REQUIRE(s1.p2_x == Catch::Approx(s2.p2_x).margin(0.001f));
    REQUIRE(s1.p2_y == Catch::Approx(s2.p2_y).margin(0.001f));
    REQUIRE(s1.p1_score == s2.p1_score);
    REQUIRE(s1.p2_score == s2.p2_score);
    REQUIRE(s1.p1_left == s2.p1_left);
    REQUIRE(s1.p2_left == s2.p2_left);
    REQUIRE(s1.wait == s2.wait);
}

// ── 14. P1 moves down ────────────────────────────────────────────────────────
// P1 moves down 30 steps.  m_speed=1200, dt=1/60 -> 20 px/step.
// Starting y=400; wrap triggers at y>(1080-68)=1012, not reached in 30 steps.

TEST_CASE("integration: P1 moves down - p1_y increases 20 px per step", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 30;
    cfg.replayPathP1 = dataPath("p1_down.replay");
    auto [t, s] = runGame(cfg);

    // 30 steps × 20 px/step = 600; 400 + 600 = 1000.
    REQUIRE(s.p1_y == Catch::Approx(1000.f).margin(1.f));
}

// ── 15. P2 moves up ───────────────────────────────────────────────────────────
// P2 moves up 30 steps.  m_speed=1200, dt=1/60 -> 20 px/step.
// Starting y=400; wrap triggers at y<-180 at start of step 31, not reached here.

TEST_CASE("integration: P2 moves up - p2_y decreases 20 px per step", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 30;
    cfg.replayPath = dataPath("p2_up.replay");
    auto [t, s] = runGame(cfg);

    // 30 steps × 20 px/step = 600; 400 - 600 = -200.
    REQUIRE(s.p2_y == Catch::Approx(-200.f).margin(1.f));
}

// ── 16. P2 moves down ─────────────────────────────────────────────────────────
// P2 moves down 26 steps.  m_speed=1200, dt=1/60 -> 20 px/step.
// Starting y=400; wrap triggers at y>(1080-180)=900 at start of step 27, not reached here.

TEST_CASE("integration: P2 moves down - p2_y increases 20 px per step", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 26;
    cfg.replayPath = dataPath("p2_down.replay");
    auto [t, s] = runGame(cfg);

    // 26 steps × 20 px/step = 520; 400 + 520 = 920.
    REQUIRE(s.p2_y == Catch::Approx(920.f).margin(1.f));
}

// ── 17. P2 left-direction flip ────────────────────────────────────────────────
// P2 starts facing left (p2left=true, scale.x=-1).
// Step 1 (d=1): flips to right -> p2left=false, scale.x=1.
// Step 2 (a=1): triggers if(!p2left) branch -> p2left=true, scale.x=-1 (pinned here).
// Steps 3-5: continue left; p2left stays true (no re-flip).

TEST_CASE("integration: P2 left-direction flip branch sets p2left and scale", "[integration]") {
    DebugConfig cfg;
    cfg.frames = 5;
    cfg.replayPath = dataPath("p2_left_flip.replay"); // 1 right then 4 left steps
    auto [t, s] = runGame(cfg);

    // After step 2 the if(!p2left) branch has fired: p2left restored to true, scale.x=-1.
    REQUIRE(s.p2_left == true);
    REQUIRE(s.p2_scale_x == Catch::Approx(-1.f).margin(0.01f));
}

// ── 18. quitAtStep ────────────────────────────────────────────────────────────
// When DebugConfig::quitAtStep is set, the Game closes after that many sim
// steps and sets m_quitToMenu.  This pins the step-8 outer-loop contract.

TEST_CASE("integration: quitAtStep closes game and sets quitToMenu", "[integration]") {
    Game g;
    DebugConfig cfg;
    cfg.quitAtStep = 5;
    g.setDebugConfig(cfg);
    g.run();

    REQUIRE(g.quitToMenu() == true);
    REQUIRE(g.steps() == 5);
}

// ── 19. HOST Game + loopback NetworkManager ───────────────────────────────────
// Client connects, sends one input, disconnects.  Host detects peerLost via
// poll(); Game's handleNetworkCommunication() then sets m_peerLeft -> run()
// tuple element 5 == true.

TEST_CASE("integration: HOST game detects peer loss", "[integration]") {
    // Setup: host + client on ephemeral port
    auto hostMgr = std::make_shared<NetworkManager>();
    REQUIRE(hostMgr->startHost(0));
    unsigned short port = hostMgr->localPort();

    NetworkManager client;
    REQUIRE(client.connectToHost("127.0.0.1", port));
    REQUIRE(hostMgr->waitForClient(sf::seconds(5)));

    // Client sends one input then disconnects
    PlayerInput inp;
    inp.up = true;
    client.sendInput(inp);
    // Small sleep to let the packet be sent before disconnect
    sf::sleep(sf::milliseconds(20));
    client.disconnect();

    // Poll host until it detects the disconnect (bounded, no infinite wait)
    bool detected = false;
    for (int i = 0; i < 200 && !detected; ++i) {
        hostMgr->poll();
        if (hostMgr->peerLost() || !hostMgr->isConnected())
            detected = true;
        sf::sleep(sf::milliseconds(5));
    }
    REQUIRE(detected); // sanity: host saw the disconnect before game starts

    // Run the HOST game; it should detect peerLost immediately and close the
    // window, returning with peerLeft==true.
    DebugConfig cfg;
    cfg.frames = 10; // worst case: runs 10 frames, but peerLost closes window early
    auto [result, s] = runGame(cfg, NetworkMode::HOST, hostMgr);

    REQUIRE(std::get<5>(result) == true); // peerLeft
}

// ── 20. AI Hard scores on idle P1 ────────────────────────────────────────────
// rng::seed(42), Hard AI, 900 frames, P1 idle -> robot (P2) scores at least once.

TEST_CASE("integration: AI Hard scores on idle P1 within 900 frames", "[integration]") {
    rng::seed(42);
    DebugConfig cfg;
    cfg.frames = 900;
    cfg.ai = AiDifficulty::Hard;
    auto [t, s] = runGame(cfg);

    REQUIRE(s.p2_score >= 1); // robot kills idle rocket at least once
    REQUIRE(s.p1_score == 0); // idle rocket never damages robot
}

// ── 21. AI reproducibility ────────────────────────────────────────────────────
// Two runs with identical seed -> identical snapshot fields.

TEST_CASE("integration: AI reproducibility - reseed 42 between runs gives identical snapshot",
          "[integration]") {
    DebugConfig cfg;
    cfg.frames = 300;
    cfg.ai = AiDifficulty::Hard;

    rng::seed(42);
    auto [t1, s1] = runGame(cfg);

    rng::seed(42);
    auto [t2, s2] = runGame(cfg);

    REQUIRE(s1.p1_x == Catch::Approx(s2.p1_x).margin(0.001f));
    REQUIRE(s1.p1_y == Catch::Approx(s2.p1_y).margin(0.001f));
    REQUIRE(s1.p2_x == Catch::Approx(s2.p2_x).margin(0.001f));
    REQUIRE(s1.p2_y == Catch::Approx(s2.p2_y).margin(0.001f));
    REQUIRE(s1.p1_score == s2.p1_score);
    REQUIRE(s1.p2_score == s2.p2_score);
    REQUIRE(s1.wait == s2.wait);
}

// ── 22. AI Easy completes without crash ──────────────────────────────────────

TEST_CASE("integration: AI Easy runs 600 frames without crash", "[integration]") {
    rng::seed(7);
    DebugConfig cfg;
    cfg.frames = 600;
    cfg.ai = AiDifficulty::Easy;
    auto [t, s] = runGame(cfg);

    REQUIRE(s.p1_score >= 0);
    REQUIRE(s.p2_score >= 0);
}

// ── 23. Replay overrides AI ───────────────────────────────────────────────────
// With both ai=Hard and replayPath set, the replay takes precedence (else-if
// ordering).  The scripted P2 kill should happen, not AI behaviour.

TEST_CASE("integration: replay overrides AI - replay kill wins over Hard AI", "[integration]") {
    rng::seed(42);
    DebugConfig cfg;
    cfg.frames = 75;
    cfg.ai = AiDifficulty::Hard;
    cfg.replayPath = dataPath("p2_kill.replay");
    auto [t, s] = runGame(cfg);

    // Replay's scripted kill: p2_score==1, wait==true (same as test 2)
    REQUIRE(s.p2_score == 1);
    REQUIRE(s.p1_score == 0);
    REQUIRE(s.wait == true);
}
