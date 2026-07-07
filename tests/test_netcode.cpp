// Socket-free unit tests for NetworkManager.
// Covers: PlayerInput/GameState packet round-trips, dispatchPacket routing,
// and lerpPos math.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <deque>

#include "NetworkManager.h"

static constexpr float kEps = 1e-5f;

// ── PlayerInput round-trip ─────────────────────────────────────────────────────

TEST_CASE("PlayerInput packet round-trip", "[netcode][unit]") {
    PlayerInput orig;
    orig.up = true;
    orig.down = false;
    orig.left = true;
    orig.right = false;
    orig.attack = true;

    sf::Packet pkt;
    orig.toPacket(pkt);

    PlayerInput got;
    got.fromPacket(pkt);

    REQUIRE(got.up == orig.up);
    REQUIRE(got.down == orig.down);
    REQUIRE(got.left == orig.left);
    REQUIRE(got.right == orig.right);
    REQUIRE(got.attack == orig.attack);
}

// ── GameState round-trip ───────────────────────────────────────────────────────

TEST_CASE("GameState packet round-trip", "[netcode][unit]") {
    GameState orig;
    orig.p1_x = 10.f;
    orig.p1_y = 20.f;
    orig.p2_x = 30.f;
    orig.p2_y = 40.f;
    orig.p1_score = 3;
    orig.p2_score = 5;
    orig.p1_left = true;
    orig.p2_left = false;
    orig.p1_scale_x = 2.f;
    orig.p1_scale_y = 2.f;
    orig.p2_scale_x = -1.f;
    orig.p2_scale_y = 1.f;
    orig.wait = true;

    sf::Packet pkt;
    orig.toPacket(pkt);

    GameState got;
    got.fromPacket(pkt);

    REQUIRE(got.p1_x == Catch::Approx(orig.p1_x).epsilon(kEps));
    REQUIRE(got.p1_y == Catch::Approx(orig.p1_y).epsilon(kEps));
    REQUIRE(got.p2_x == Catch::Approx(orig.p2_x).epsilon(kEps));
    REQUIRE(got.p2_y == Catch::Approx(orig.p2_y).epsilon(kEps));
    REQUIRE(got.p1_score == orig.p1_score);
    REQUIRE(got.p2_score == orig.p2_score);
    REQUIRE(got.p1_left == orig.p1_left);
    REQUIRE(got.p2_left == orig.p2_left);
    REQUIRE(got.p1_scale_x == Catch::Approx(orig.p1_scale_x).epsilon(kEps));
    REQUIRE(got.p1_scale_y == Catch::Approx(orig.p1_scale_y).epsilon(kEps));
    REQUIRE(got.p2_scale_x == Catch::Approx(orig.p2_scale_x).epsilon(kEps));
    REQUIRE(got.p2_scale_y == Catch::Approx(orig.p2_scale_y).epsilon(kEps));
    REQUIRE(got.wait == orig.wait);
}

// ── dispatchPacket: State routing ─────────────────────────────────────────────

TEST_CASE("dispatchPacket routes State into stateBuf", "[netcode][unit]") {
    std::deque<TimedState> stateBuf;
    std::deque<PlayerInput> inputQueue;

    GameState s;
    s.p1_x = 5.f;
    s.p1_y = 7.f;

    sf::Packet pkt;
    pkt << static_cast<sf::Uint8>(2); // MsgType::State
    s.toPacket(pkt);

    dispatchPacket(pkt, sf::seconds(1.f), stateBuf, inputQueue, 4);

    REQUIRE(stateBuf.size() == 1);
    REQUIRE(inputQueue.empty());
    REQUIRE(stateBuf[0].state.p1_x == Catch::Approx(5.f).epsilon(kEps));
    REQUIRE(stateBuf[0].state.p1_y == Catch::Approx(7.f).epsilon(kEps));
    REQUIRE(stateBuf[0].arrival == sf::seconds(1.f));
}

// ── dispatchPacket: Input routing ─────────────────────────────────────────────

TEST_CASE("dispatchPacket routes Input into inputQueue", "[netcode][unit]") {
    std::deque<TimedState> stateBuf;
    std::deque<PlayerInput> inputQueue;

    PlayerInput inp;
    inp.up = true;
    inp.attack = true;

    sf::Packet pkt;
    pkt << static_cast<sf::Uint8>(1); // MsgType::Input
    inp.toPacket(pkt);

    dispatchPacket(pkt, sf::seconds(0.f), stateBuf, inputQueue, 4);

    REQUIRE(inputQueue.size() == 1);
    REQUIRE(stateBuf.empty());
    REQUIRE(inputQueue[0].up == true);
    REQUIRE(inputQueue[0].attack == true);
    REQUIRE(inputQueue[0].down == false);
}

// ── dispatchPacket: unknown type is discarded ─────────────────────────────────

TEST_CASE("dispatchPacket discards unknown message type", "[netcode][unit]") {
    std::deque<TimedState> stateBuf;
    std::deque<PlayerInput> inputQueue;

    sf::Packet pkt;
    pkt << static_cast<sf::Uint8>(99); // unknown

    dispatchPacket(pkt, sf::seconds(0.f), stateBuf, inputQueue, 4);

    REQUIRE(stateBuf.empty());
    REQUIRE(inputQueue.empty());
}

// ── dispatchPacket: cap eviction ──────────────────────────────────────────────

TEST_CASE("dispatchPacket evicts oldest state when cap reached", "[netcode][unit]") {
    std::deque<TimedState> stateBuf;
    std::deque<PlayerInput> inputQueue;

    for (int i = 0; i < 5; ++i) {
        sf::Packet pkt;
        pkt << static_cast<sf::Uint8>(2);
        GameState s;
        s.p1_x = static_cast<float>(i);
        s.toPacket(pkt);
        dispatchPacket(pkt, sf::seconds(static_cast<float>(i)), stateBuf, inputQueue, 4);
    }

    REQUIRE(stateBuf.size() == 4);
    REQUIRE(stateBuf[0].state.p1_x == Catch::Approx(1.f).epsilon(kEps)); // entry 0 evicted
    REQUIRE(stateBuf[3].state.p1_x == Catch::Approx(4.f).epsilon(kEps));
}

// ── lerpPos ───────────────────────────────────────────────────────────────────

TEST_CASE("lerpPos midpoint", "[netcode][unit]") {
    auto r = lerpPos({0.f, 0.f}, {10.f, 20.f}, 0.5f);
    REQUIRE(r.x == Catch::Approx(5.f).epsilon(kEps));
    REQUIRE(r.y == Catch::Approx(10.f).epsilon(kEps));
}

TEST_CASE("lerpPos t=0 snaps to from", "[netcode][unit]") {
    auto r = lerpPos({3.f, 4.f}, {10.f, 20.f}, 0.f);
    REQUIRE(r.x == Catch::Approx(3.f).epsilon(kEps));
    REQUIRE(r.y == Catch::Approx(4.f).epsilon(kEps));
}

TEST_CASE("lerpPos t=1 snaps to to", "[netcode][unit]") {
    auto r = lerpPos({3.f, 4.f}, {10.f, 20.f}, 1.f);
    REQUIRE(r.x == Catch::Approx(10.f).epsilon(kEps));
    REQUIRE(r.y == Catch::Approx(20.f).epsilon(kEps));
}

TEST_CASE("lerpPos clamps t below 0", "[netcode][unit]") {
    auto r = lerpPos({0.f, 0.f}, {10.f, 0.f}, -2.f);
    REQUIRE(r.x == Catch::Approx(0.f).epsilon(kEps));
}

TEST_CASE("lerpPos clamps t above 1", "[netcode][unit]") {
    auto r = lerpPos({0.f, 0.f}, {10.f, 0.f}, 3.f);
    REQUIRE(r.x == Catch::Approx(10.f).epsilon(kEps));
}
