// Unit tests for the AI decider + controller.
// Pure functions only - no window, no display, no audio.

#include <algorithm>
#include <catch2/catch_test_macros.hpp>

#include "ai.h"
#include "rng.h"

// ── helpers ──────────────────────────────────────────────────────────────────

static AiView makeView(float selfX, float selfY, float oppX, float oppY, bool facingLeft = true,
                       bool inCooldown = false) {
    AiView v;
    v.selfPos = {selfX, selfY};
    v.selfBounds = {selfX - 60.f, selfY - 90.f, 120.f, 180.f}; // normalised positive rect
    v.selfFacingLeft = facingLeft;
    v.oppPos = {oppX, oppY};
    v.oppBounds = {oppX - 60.f, oppY - 90.f, 120.f, 180.f};
    // Hazards far off-screen
    v.hazards = {sf::FloatRect{-2000.f, -2000.f, 1.f, 1.f},
                 sf::FloatRect{-2000.f, -2000.f, 1.f, 1.f},
                 sf::FloatRect{-2000.f, -2000.f, 1.f, 1.f}};
    v.inCooldown = inCooldown;
    v.selfScore = 0;
    v.oppScore = 0;
    v.step = 0;
    return v;
}

// ── 1. In range + aligned + facing + missTimeProb=0 -> attack ─────────────────

TEST_CASE("ai: in range, aligned, facing, missTimeProb=0 -> attack==true", "[ai]") {
    AiParams p = paramsFor(AiDifficulty::Hard);
    p.missTimeProb = 0.f;
    p.mistakeProb = 0.f;

    std::mt19937 rng(1);
    // Self at 960, opponent to the LEFT at 700 (dx = -260 < attackRange=320)
    // Facing left (selfFacingLeft=true), opponent is to the left -> correct facing
    AiView v = makeView(960.f, 400.f, 700.f, 400.f, /*facingLeft=*/true);

    PlayerInput out = decideAiInput(v, p, rng);
    REQUIRE(out.attack == true);
}

// ── 1b. Near-edge (body-to-body) attack: wide opponent to the right ───────────
// Mirrors the real posture makeAiView normalises: the opponent (rocket) sits to
// the RIGHT of the robot. The attack check must use the NEAR (left) edge of a
// normalised oppBounds, not the anchor/centre - here the anchor is out of range
// but the near edge is in range, so a body-aware AI still attacks. This is the
// oppToRight branch of nearOppEdge; a non-normalised (negative-width) oppBounds
// would pick the far edge and wrongly decline the attack.

TEST_CASE("ai: near-edge attack - wide opponent to the right, anchor out but edge in", "[ai]") {
    AiParams p = paramsFor(AiDifficulty::Hard); // attackRange=320, attackAlignY=80
    p.missTimeProb = 0.f;
    p.mistakeProb = 0.f;

    std::mt19937 rng(1);
    // Robot at x=960; opponent anchor at x=1410 (dx=+450 > attackRange -> anchor out of range).
    AiView v = makeView(960.f, 400.f, 1410.f, 400.f, /*facingLeft=*/false);
    // Wide opponent: near (left) edge at 1260 -> edge distance 300 < attackRange=320.
    v.oppBounds = sf::FloatRect{1260.f, 310.f, 300.f, 180.f};

    PlayerInput out = decideAiInput(v, p, rng);
    REQUIRE(out.attack == true); // fires on near-edge distance, not the far anchor
    REQUIRE(out.right == true);  // faces toward opponent (to the right)
    REQUIRE(out.left == false);
}

// ── 2. Beyond attackRange -> no attack, moves toward opponent ─────────────────

TEST_CASE("ai: beyond attackRange -> no attack, moves toward opponent", "[ai]") {
    AiParams p = paramsFor(AiDifficulty::Hard);
    p.mistakeProb = 0.f;
    p.aggression = 1.f;     // always close in
    p.preferredGapX = 50.f; // preferred gap much less than actual distance

    std::mt19937 rng(1);
    // Opponent far to the right: dx=800 > attackRange=320
    AiView v = makeView(500.f, 400.f, 1300.f, 400.f, /*facingLeft=*/false);

    PlayerInput out = decideAiInput(v, p, rng);
    REQUIRE(out.attack == false);
    REQUIRE(out.right == true); // close in toward opponent (right)
    REQUIRE(out.left == false);
}

// ── 3. Opponent behind (wrong facing) -> turns toward opponent ─────────────────

TEST_CASE("ai: opponent behind -> turns toward opponent", "[ai]") {
    AiParams p = paramsFor(AiDifficulty::Hard);
    p.missTimeProb = 0.f;
    p.mistakeProb = 0.f;

    std::mt19937 rng(1);
    // Self facing LEFT but opponent is to the RIGHT (dx=+200 < attackRange=320)
    // In range and aligned, but facing wrong direction.
    // Attack fires with right=true (turn+attack in same step).
    AiView v = makeView(960.f, 400.f, 1160.f, 400.f, /*facingLeft=*/true);

    PlayerInput out = decideAiInput(v, p, rng);
    // Opponent is to the right -> right=true (turning/facing toward opponent)
    REQUIRE(out.right == true);
    REQUIRE(out.left == false);
}

// ── 4. selfBounds intersecting a hazard -> moves out, overrides attack ────────

TEST_CASE("ai: selfBounds in hazard -> moves out, no attack", "[ai]") {
    AiParams p = paramsFor(AiDifficulty::Hard);
    p.hazardMargin = 80.f; // Hard preset is 0 (pure aggressor); set explicitly for this test
    p.mistakeProb = 0.f;
    p.missTimeProb = 0.f;

    std::mt19937 rng(1);
    AiView v = makeView(960.f, 400.f, 300.f, 400.f, /*facingLeft=*/true);
    // Place a hazard overlapping the inflated selfBounds
    // selfBounds = {900, 310, 120, 180}; inflated by 80 -> {820, 230, 280, 340}
    // Hazard at x=830, width=20 -> overlaps inflated bounds
    v.hazards[0] = sf::FloatRect{830.f, 250.f, 20.f, 300.f};

    PlayerInput out = decideAiInput(v, p, rng);
    REQUIRE(out.attack == false);
    // Hazard centre x=840, self centre x=960 -> hazard to the left -> move right
    REQUIRE(out.right == true);
}

// ── 5. inCooldown -> all-false ─────────────────────────────────────────────────

TEST_CASE("ai: inCooldown -> all-false", "[ai]") {
    AiParams p = paramsFor(AiDifficulty::Hard);
    std::mt19937 rng(42);
    AiView v = makeView(960.f, 400.f, 300.f, 400.f, true, /*inCooldown=*/true);

    PlayerInput out = decideAiInput(v, p, rng);
    REQUIRE(!out.up);
    REQUIRE(!out.down);
    REQUIRE(!out.left);
    REQUIRE(!out.right);
    REQUIRE(!out.attack);
}

// ── 6. Determinism: same view seq + same seed -> identical output seq ──────────

TEST_CASE("ai: determinism - same seed and view seq -> identical PlayerInput seq", "[ai]") {
    AiParams p = paramsFor(AiDifficulty::Easy); // mistakeProb=0.15 exercises rng path
    p.decisionEvery = 1;                        // re-decide every step for more rng draws

    AiView v = makeView(960.f, 400.f, 400.f, 400.f, true);

    auto run = [&]() {
        std::mt19937 rng(99);
        AiController ctrl(p, rng);
        std::vector<PlayerInput> seq;
        for (int i = 0; i < 40; ++i) {
            v.step = i;
            seq.push_back(ctrl.step(v));
        }
        return seq;
    };

    auto seq1 = run();
    auto seq2 = run();

    REQUIRE(seq1.size() == seq2.size());
    for (std::size_t i = 0; i < seq1.size(); ++i) {
        REQUIRE(seq1[i].up == seq2[i].up);
        REQUIRE(seq1[i].down == seq2[i].down);
        REQUIRE(seq1[i].left == seq2[i].left);
        REQUIRE(seq1[i].right == seq2[i].right);
        REQUIRE(seq1[i].attack == seq2[i].attack);
    }
}

// ── 7. Reaction delay: teleport at step k -> old pos seen until k+reactionSteps

TEST_CASE("ai: reaction delay - teleport seen after reactionSteps", "[ai]") {
    AiParams p = paramsFor(AiDifficulty::Easy); // reactionSteps=18
    p.mistakeProb = 0.f;
    p.decisionEvery = 1; // re-decide every step
    p.missTimeProb = 0.f;
    p.aggression = 1.f;
    p.preferredGapX = 50.f;

    std::mt19937 rng(7);
    AiController ctrl(p, rng);

    // Phase 1: opponent starts to the right (dx > 0). AI should move right.
    AiView v = makeView(500.f, 400.f, 1400.f, 400.f, false);

    // Run reactionSteps-1 steps: AI should still see old (right-side) position.
    PlayerInput last_before;
    for (int i = 0; i < p.reactionSteps - 1; ++i) {
        v.step = i;
        last_before = ctrl.step(v);
    }
    // Should be moving right (toward opponent on the right)
    REQUIRE(last_before.right == true);

    // Now teleport opponent to far left - AI should NOT react immediately
    v.oppPos = {100.f, 400.f};
    v.oppBounds = {40.f, 310.f, 120.f, 180.f};
    v.step = p.reactionSteps - 1;
    PlayerInput at_teleport = ctrl.step(v);
    // Still sees old position (reactionSteps - 1 steps of new pos; need reactionSteps)
    REQUIRE(at_teleport.right == true);

    // After reactionSteps more steps with opponent on the left, AI should see new pos
    PlayerInput after;
    for (int i = 0; i < p.reactionSteps; ++i) {
        v.step = p.reactionSteps + i;
        after = ctrl.step(v);
    }
    // Now should be moving left (toward opponent on the left)
    REQUIRE(after.left == true);
}

// ── 8. Swing pacing: no two attack==true closer than swingEverySteps ──────────

TEST_CASE("ai: swing pacing - attacks spaced by at least swingEverySteps", "[ai]") {
    AiParams p = paramsFor(AiDifficulty::Hard);
    p.mistakeProb = 0.f;
    p.missTimeProb = 0.f;
    p.decisionEvery = 1; // re-decide every step -> attack fires whenever in range

    std::mt19937 rng(3);
    AiController ctrl(p, rng);

    AiView v = makeView(960.f, 400.f, 700.f, 400.f, true); // opponent in range

    int last_attack_step = -1000;
    int min_gap = 9999;

    for (int i = 0; i < 300; ++i) {
        v.step = i;
        PlayerInput out = ctrl.step(v);
        if (out.attack) {
            int gap = i - last_attack_step;
            if (last_attack_step >= 0)
                min_gap = std::min(min_gap, gap);
            last_attack_step = i;
        }
    }

    REQUIRE(min_gap >= p.swingEverySteps); // no two attacks closer than swingEverySteps
}

// ── 9. paramsFor sanity ───────────────────────────────────────────────────────

TEST_CASE("ai: paramsFor sanity - params scale with difficulty (Easy < Medium < Hard)", "[ai]") {
    AiParams easy = paramsFor(AiDifficulty::Easy);
    AiParams med = paramsFor(AiDifficulty::Medium);
    AiParams hard = paramsFor(AiDifficulty::Hard);

    REQUIRE(easy.reactionSteps > med.reactionSteps);
    REQUIRE(med.reactionSteps > hard.reactionSteps);

    REQUIRE(easy.mistakeProb > med.mistakeProb);
    REQUIRE(med.mistakeProb > hard.mistakeProb);

    REQUIRE(easy.attackRange < med.attackRange);
    REQUIRE(med.attackRange < hard.attackRange);

    REQUIRE(easy.preferredGapX > med.preferredGapX);
    REQUIRE(med.preferredGapX > hard.preferredGapX);

    REQUIRE(easy.aggression < med.aggression);
    REQUIRE(med.aggression < hard.aggression);
}

// ── 9b. decisionEvery hold: output changes only on boundary ───────────────────

TEST_CASE("ai: decisionEvery hold - output constant within hold window", "[ai]") {
    AiParams p = paramsFor(AiDifficulty::Hard);
    p.decisionEvery = 6;
    p.reactionSteps = 0; // zero reaction delay so ring.front() = current view immediately
    p.aggression = 1.f;  // always close in (eliminates rng variability in positioning)
    p.mistakeProb = 0.f;
    p.missTimeProb = 0.f;
    p.swingEverySteps = 1000; // suppress swing filter interference

    std::mt19937 rng(5);
    AiController ctrl(p, rng);

    // View A: opponent to the left -> expect left movement
    AiView vA = makeView(960.f, 400.f, 400.f, 400.f, true);
    // View B: opponent far above and to the right (beyond range) -> expect up + right positioning
    AiView vB = makeView(960.f, 700.f, 1800.f, 100.f, false);
    vB.hazards = vA.hazards;

    // Steps 0..5 (first hold window): all use vA
    PlayerInput step0 = ctrl.step(vA);
    std::vector<PlayerInput> window1;
    window1.push_back(step0);
    for (int i = 1; i < p.decisionEvery; ++i) {
        vA.step = i;
        window1.push_back(ctrl.step(vA));
    }
    // Within the window, all outputs should equal step0
    for (int i = 1; i < p.decisionEvery; ++i) {
        REQUIRE(window1[static_cast<std::size_t>(i)].left == window1[0].left);
        REQUIRE(window1[static_cast<std::size_t>(i)].right == window1[0].right);
        REQUIRE(window1[static_cast<std::size_t>(i)].up == window1[0].up);
        REQUIRE(window1[static_cast<std::size_t>(i)].down == window1[0].down);
    }

    // Step decisionEvery: re-decide with vB - output should differ
    vB.step = p.decisionEvery;
    PlayerInput newDecision = ctrl.step(vB);
    // vB has opponent far to the right and no attack -> should move right (not left)
    REQUIRE(newDecision.right == true);
    REQUIRE(newDecision.left == false);

    // Steps decisionEvery+1..2*decisionEvery-1: should all match newDecision
    for (int i = 1; i < p.decisionEvery; ++i) {
        vB.step = p.decisionEvery + i;
        PlayerInput held = ctrl.step(vB);
        REQUIRE(held.left == newDecision.left);
        REQUIRE(held.right == newDecision.right);
        REQUIRE(held.up == newDecision.up);
        REQUIRE(held.down == newDecision.down);
    }
}
