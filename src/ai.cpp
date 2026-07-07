#include "ai.h"
#include <cmath>
#include <stdexcept>

// ── paramsFor ─────────────────────────────────────────────────────────────────

AiParams paramsFor(AiDifficulty d) {
    switch (d) {
    case AiDifficulty::Easy:
        return {18, 12, 200.f, 40.f, 0.50f, 60, 0.f, 500.f, 0.35f, 0.15f};
    case AiDifficulty::Medium:
        return {9, 6, 280.f, 60.f, 0.20f, 40, 40.f, 300.f, 0.65f, 0.05f};
    case AiDifficulty::Hard:
        // hazardMargin=0: Hard is a pure aggressor — ignores fire, pursues P1 through the
        // centre-arena hazard zone (fire2) that would otherwise block the approach path.
        return {3, 3, 320.f, 80.f, 0.02f, 25, 0.f, 180.f, 0.90f, 0.00f};
    case AiDifficulty::None:
        break;
    }
    throw std::invalid_argument("paramsFor: AiDifficulty::None has no params");
}

// ── decideAiInput ────────────────────────────────────────────────────────────

PlayerInput decideAiInput(const AiView& view, const AiParams& p, std::mt19937& rng) {
    PlayerInput out; // all false

    // Priority 1: in cooldown → idle
    if (view.inCooldown)
        return out;

    // Priority 2: mistake roll
    if (p.mistakeProb > 0.f) {
        std::uniform_real_distribution<float> uni(0.f, 1.f);
        if (uni(rng) < p.mistakeProb)
            return out;
    }

    // Priority 3: hazard escape (uses self — not delayed — position + bounds)
    if (p.hazardMargin > 0.f) {
        sf::FloatRect inflated = view.selfBounds;
        inflated.left -= p.hazardMargin;
        inflated.top -= p.hazardMargin;
        inflated.width += 2.f * p.hazardMargin;
        inflated.height += 2.f * p.hazardMargin;

        for (const auto& hazard : view.hazards) {
            if (inflated.intersects(hazard)) {
                // Move away from hazard centre horizontally
                float hcx = hazard.left + hazard.width * 0.5f;
                float scx = view.selfBounds.left + view.selfBounds.width * 0.5f;
                out.right = (hcx <= scx);
                out.left = (hcx > scx);
                return out;
            }
        }
    }

    // Delayed opponent info (already substituted by AiController)
    float dx = view.oppPos.x - view.selfPos.x;
    float absDx = (dx < 0.f ? -dx : dx);
    bool oppToRight = (dx > 0.f);
    float dy = view.oppPos.y - view.selfPos.y;
    float absDy = (dy < 0.f ? -dy : dy);

    // Priority 4: attack
    // Use distance from selfPos to the nearest edge of oppBounds (body-to-body awareness).
    // This accounts for the opponent's body width so the AI fires when its weapon can
    // actually connect, not just when the sprite anchors are within attackRange.
    {
        float nearOppEdge = oppToRight ? view.oppBounds.left
                                       : view.oppBounds.left + view.oppBounds.width;
        float horizDist = (view.selfPos.x - nearOppEdge);
        if (horizDist < 0.f)
            horizDist = -horizDist;
        if (horizDist < p.attackRange && absDy < p.attackAlignY) {
            std::uniform_real_distribution<float> uni(0.f, 1.f);
            if (uni(rng) >= p.missTimeProb) {
                // Face toward opponent (update() processes movement before attack,
                // so facing flips before the hit rect is evaluated).
                if (oppToRight)
                    out.right = true;
                else
                    out.left = true;
                out.attack = true;
                return out;
            }
        }
    }

    // Priority 5: positioning
    {
        std::uniform_real_distribution<float> uni(0.f, 1.f);
        float gap = absDx;

        if (gap > p.preferredGapX + 20.f) {
            // Too far — close in, gated by aggression
            if (uni(rng) < p.aggression) {
                out.right = oppToRight;
                out.left = !oppToRight;
            }
        } else if (gap < p.preferredGapX - 20.f) {
            // Too close — back off
            out.right = !oppToRight;
            out.left = oppToRight;
        }

        // Y steering
        if (dy < -20.f)
            out.up = true;
        else if (dy > 20.f)
            out.down = true;
    }

    return out;
}

// ── AiController ────────────────────────────────────────────────────────────

AiController::AiController(const AiParams& params, std::mt19937& rng)
    : m_params(params), m_rng(rng) {}

PlayerInput AiController::step(const AiView& view) {
    // 1. Record fresh opponent snapshot; cap ring to reactionSteps+1 entries so
    //    ring.front() is always the snapshot from exactly reactionSteps steps ago
    //    at steady state, or the oldest available during startup.
    m_ring.push_back({view.oppPos, view.oppBounds});
    if (static_cast<int>(m_ring.size()) > m_params.reactionSteps + 1)
        m_ring.pop_front();

    // 2. Build delayed view: substitute ring.front() for opponent fields.
    AiView delayedView = view;
    delayedView.oppPos = m_ring.front().pos;
    delayedView.oppBounds = m_ring.front().bounds;

    // 3. Decision hold: re-decide every decisionEvery steps.
    if (--m_holdCounter <= 0) {
        m_heldDecision = decideAiInput(delayedView, m_params, m_rng);
        m_holdCounter = m_params.decisionEvery;
    }

    // 4. Output filter: swing pacing — gate attack to at most one true per
    //    swingEverySteps (update() clears m_p2Attack each step, so an un-gated
    //    held attack=true would swing on every step of the hold window).
    PlayerInput out = m_heldDecision;
    if (m_swingCooldown > 0) {
        --m_swingCooldown;
        out.attack = false;
    } else if (out.attack) {
        m_swingCooldown = m_params.swingEverySteps;
    }

    return out;
}
