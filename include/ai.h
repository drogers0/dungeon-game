#pragma once

#include "NetworkManager.h" // PlayerInput
#include "ai_difficulty.h"
#include <SFML/Graphics.hpp>
#include <array>
#include <deque>
#include <random>

// ── AiParams ─────────────────────────────────────────────────────────────────
// All tunable knobs for one difficulty level.

struct AiParams {
    int reactionSteps;   // frames of opponent-position delay
    int decisionEvery;   // re-decide every N steps; hold in between
    float attackRange;   // max horizontal distance (px) to trigger attack
    float attackAlignY;  // max vertical distance (px) to trigger attack
    float missTimeProb;  // probability of passing on a valid attack opportunity
    int swingEverySteps; // minimum steps between successive attack=true outputs
    float hazardMargin;  // px to inflate selfBounds before hazard intersection check
    float preferredGapX; // preferred horizontal distance from opponent (px)
    float aggression;    // probability [0,1] of closing in when gap > preferredGapX
    float mistakeProb;   // probability [0,1] of going idle on any given decision
};

AiParams paramsFor(AiDifficulty d);

// ── AiView ───────────────────────────────────────────────────────────────────
// Snapshot of game state fed to the decider each step.
// selfBounds MUST be a normalised (positive-width) rect — see Game::makeAiView.

struct AiView {
    sf::Vector2f selfPos;
    sf::FloatRect selfBounds; // normalised (positive width/height)
    bool selfFacingLeft;
    sf::Vector2f oppPos;
    sf::FloatRect oppBounds;
    std::array<sf::FloatRect, 3> hazards; // fire hazards (arena[1..3])
    bool inCooldown;
    int selfScore;
    int oppScore;
    long long step;
};

// ── Pure decider ─────────────────────────────────────────────────────────────
// Stateless: maps (view, params, rng) → PlayerInput.
// Decision priority: inCooldown → mistake → hazard escape → attack → positioning.
// Draws from rng only as needed; never reads game state through any other path.

PlayerInput decideAiInput(const AiView& view, const AiParams& params, std::mt19937& rng);

// ── AiController ─────────────────────────────────────────────────────────────
// Thin stateful wrapper that:
//   • maintains the reaction-delay ring (opponent snapshots);
//   • holds the last decision for decisionEvery steps;
//   • applies the swing-pacing output filter.
// Stores std::mt19937& (reference to rng::engine()) — NOT a copy — so
// rng::seed() deterministically resets the draw sequence.

class AiController {
public:
    AiController(const AiParams& params, std::mt19937& rng);

    // Drive one sim step; returns the PlayerInput to feed applyPlayerInput().
    PlayerInput step(const AiView& view);

private:
    AiParams m_params;
    std::mt19937& m_rng;

    struct OppSnapshot {
        sf::Vector2f pos;
        sf::FloatRect bounds;
    };
    std::deque<OppSnapshot> m_ring; // opponent history for reaction delay

    int m_holdCounter = 0;      // steps until next re-decide (0 → decide now)
    PlayerInput m_heldDecision; // last computed decision

    int m_swingCooldown = 0; // steps remaining before attack=true is allowed again
};
