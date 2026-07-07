// Socket-free unit tests for NetworkManager.
// Covers: PlayerInput/GameState packet round-trips, dispatchPacket routing,
// and lerpPos math.  No Catch2 — plain assert + main() until issue #7.

#include <cassert>
#include <cmath>
#include <deque>
#include <iostream>

#include "NetworkManager.h"

// ── helpers ───────────────────────────────────────────────────────────────────

static constexpr float kEps = 1e-5f;
static bool feq(float a, float b) { return std::abs(a - b) < kEps; }

// ── PlayerInput round-trip ────────────────────────────────────────────────────

static void test_playerinput_roundtrip()
{
    PlayerInput orig;
    orig.up = true; orig.down = false; orig.left = true;
    orig.right = false; orig.attack = true;

    sf::Packet pkt;
    orig.toPacket(pkt);

    PlayerInput got;
    got.fromPacket(pkt);

    assert(got.up     == orig.up);
    assert(got.down   == orig.down);
    assert(got.left   == orig.left);
    assert(got.right  == orig.right);
    assert(got.attack == orig.attack);
}

// ── GameState round-trip ──────────────────────────────────────────────────────

static void test_gamestate_roundtrip()
{
    GameState orig;
    orig.p1_x = 10.f; orig.p1_y = 20.f;
    orig.p2_x = 30.f; orig.p2_y = 40.f;
    orig.p1_score = 3; orig.p2_score = 5;
    orig.p1_left = true; orig.p2_left = false;
    orig.p1_scale_x = 2.f;  orig.p1_scale_y = 2.f;
    orig.p2_scale_x = -1.f; orig.p2_scale_y = 1.f;
    orig.wait = true;

    sf::Packet pkt;
    orig.toPacket(pkt);

    GameState got;
    got.fromPacket(pkt);

    assert(feq(got.p1_x, orig.p1_x));
    assert(feq(got.p1_y, orig.p1_y));
    assert(feq(got.p2_x, orig.p2_x));
    assert(feq(got.p2_y, orig.p2_y));
    assert(got.p1_score   == orig.p1_score);
    assert(got.p2_score   == orig.p2_score);
    assert(got.p1_left    == orig.p1_left);
    assert(got.p2_left    == orig.p2_left);
    assert(feq(got.p1_scale_x, orig.p1_scale_x));
    assert(feq(got.p1_scale_y, orig.p1_scale_y));
    assert(feq(got.p2_scale_x, orig.p2_scale_x));
    assert(feq(got.p2_scale_y, orig.p2_scale_y));
    assert(got.wait == orig.wait);
}

// ── dispatchPacket: State routing ─────────────────────────────────────────────

static void test_dispatch_state()
{
    std::deque<TimedState>  stateBuf;
    std::deque<PlayerInput> inputQueue;

    GameState s;
    s.p1_x = 5.f; s.p1_y = 7.f;

    sf::Packet pkt;
    pkt << static_cast<sf::Uint8>(2); // MsgType::State
    s.toPacket(pkt);

    dispatchPacket(pkt, sf::seconds(1.f), stateBuf, inputQueue, 4);

    assert(stateBuf.size() == 1);
    assert(inputQueue.empty());
    assert(feq(stateBuf[0].state.p1_x, 5.f));
    assert(feq(stateBuf[0].state.p1_y, 7.f));
    assert(stateBuf[0].arrival == sf::seconds(1.f));
}

// ── dispatchPacket: Input routing ─────────────────────────────────────────────

static void test_dispatch_input()
{
    std::deque<TimedState>  stateBuf;
    std::deque<PlayerInput> inputQueue;

    PlayerInput inp;
    inp.up = true; inp.attack = true;

    sf::Packet pkt;
    pkt << static_cast<sf::Uint8>(1); // MsgType::Input
    inp.toPacket(pkt);

    dispatchPacket(pkt, sf::seconds(0.f), stateBuf, inputQueue, 4);

    assert(inputQueue.size() == 1);
    assert(stateBuf.empty());
    assert(inputQueue[0].up     == true);
    assert(inputQueue[0].attack == true);
    assert(inputQueue[0].down   == false);
}

// ── dispatchPacket: unknown type is discarded ─────────────────────────────────

static void test_dispatch_unknown_discarded()
{
    std::deque<TimedState>  stateBuf;
    std::deque<PlayerInput> inputQueue;

    sf::Packet pkt;
    pkt << static_cast<sf::Uint8>(99); // unknown

    dispatchPacket(pkt, sf::seconds(0.f), stateBuf, inputQueue, 4);

    assert(stateBuf.empty());
    assert(inputQueue.empty());
}

// ── dispatchPacket: cap eviction ──────────────────────────────────────────────

static void test_dispatch_cap_eviction()
{
    std::deque<TimedState>  stateBuf;
    std::deque<PlayerInput> inputQueue;

    for (int i = 0; i < 5; ++i) {
        sf::Packet pkt;
        pkt << static_cast<sf::Uint8>(2);
        GameState s;
        s.p1_x = static_cast<float>(i);
        s.toPacket(pkt);
        dispatchPacket(pkt, sf::seconds(static_cast<float>(i)),
                       stateBuf, inputQueue, 4);
    }

    assert(stateBuf.size() == 4);          // cap = 4, oldest evicted
    assert(feq(stateBuf[0].state.p1_x, 1.f)); // entry 0 was evicted
    assert(feq(stateBuf[3].state.p1_x, 4.f));
}

// ── lerpPos: midpoint ─────────────────────────────────────────────────────────

static void test_lerppos_midpoint()
{
    auto r = lerpPos({0.f, 0.f}, {10.f, 20.f}, 0.5f);
    assert(feq(r.x, 5.f));
    assert(feq(r.y, 10.f));
}

// ── lerpPos: t = 0 snaps to 'from' ───────────────────────────────────────────

static void test_lerppos_clamp_t0()
{
    auto r = lerpPos({3.f, 4.f}, {10.f, 20.f}, 0.f);
    assert(feq(r.x, 3.f));
    assert(feq(r.y, 4.f));
}

// ── lerpPos: t = 1 snaps to 'to' ─────────────────────────────────────────────

static void test_lerppos_clamp_t1()
{
    auto r = lerpPos({3.f, 4.f}, {10.f, 20.f}, 1.f);
    assert(feq(r.x, 10.f));
    assert(feq(r.y, 20.f));
}

// ── lerpPos: t < 0 is clamped to 0 ───────────────────────────────────────────

static void test_lerppos_clamp_below_0()
{
    auto r = lerpPos({0.f, 0.f}, {10.f, 0.f}, -2.f);
    assert(feq(r.x, 0.f));
}

// ── lerpPos: t > 1 is clamped to 1 ───────────────────────────────────────────

static void test_lerppos_clamp_above_1()
{
    auto r = lerpPos({0.f, 0.f}, {10.f, 0.f}, 3.f);
    assert(feq(r.x, 10.f));
}

// ─────────────────────────────────────────────────────────────────────────────

int main()
{
    test_playerinput_roundtrip();
    test_gamestate_roundtrip();
    test_dispatch_state();
    test_dispatch_input();
    test_dispatch_unknown_discarded();
    test_dispatch_cap_eviction();
    test_lerppos_midpoint();
    test_lerppos_clamp_t0();
    test_lerppos_clamp_t1();
    test_lerppos_clamp_below_0();
    test_lerppos_clamp_above_1();

    std::cout << "All netcode unit tests passed.\n";
    return 0;
}
