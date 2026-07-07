// Loopback unit tests: exercises real socket paths in a single process.
// HOST and CLIENT NetworkManagers communicate over 127.0.0.1 on an ephemeral
// port (port 0 → OS picks one), so the test never conflicts with a running game.
//
// Classified as "unit" because they require no display — only real sockets.

#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "NetworkManager.h"
#include <SFML/Network.hpp>

// Poll nm up to maxTries × sleepMs, returning true as soon as cond() is true.
template <typename F>
static bool waitFor(NetworkManager& nm, F cond, int maxTries = 200, int sleepMs = 5) {
    for (int i = 0; i < maxTries; ++i) {
        nm.poll();
        if (cond())
            return true;
        sf::sleep(sf::milliseconds(sleepMs));
    }
    return false;
}

// ── Setup fixture: host + client connected on an ephemeral port ───────────────

struct LoopbackPair {
    NetworkManager host;
    NetworkManager client;
    unsigned short port;

    LoopbackPair() {
        REQUIRE(host.startHost(0));
        port = host.localPort();
        REQUIRE(port != 0);
        REQUIRE(client.connectToHost("127.0.0.1", port));
        REQUIRE(host.waitForClient(sf::seconds(5)));
        REQUIRE(host.isConnected());
        REQUIRE(client.isConnected());
    }
};

// ── client sendInput → host poll / nextInput ──────────────────────────────────

TEST_CASE("loopback: client sendInput reaches host", "[loopback][unit]") {
    LoopbackPair lb;

    PlayerInput sent;
    sent.up = true;
    sent.attack = true;
    REQUIRE(lb.client.sendInput(sent));

    PlayerInput recv;
    bool got = waitFor(lb.host, [&] { return lb.host.nextInput(recv); });
    REQUIRE(got);
    REQUIRE(recv.up == true);
    REQUIRE(recv.attack == true);
    REQUIRE(recv.down == false);
    REQUIRE(recv.left == false);
}

// ── host sendGameState → client poll / stateBuf ───────────────────────────────

TEST_CASE("loopback: host sendGameState reaches client stateBuf", "[loopback][unit]") {
    LoopbackPair lb;

    GameState sent;
    sent.p1_x = 123.f;
    sent.p2_y = 456.f;
    sent.p1_score = 7;
    REQUIRE(lb.host.sendGameState(sent));

    bool got = waitFor(lb.client, [&] { return !lb.client.stateBuf().empty(); });
    REQUIRE(got);
    const GameState& r = lb.client.stateBuf().back().state;
    REQUIRE(std::abs(r.p1_x - 123.f) < 1e-4f);
    REQUIRE(std::abs(r.p2_y - 456.f) < 1e-4f);
    REQUIRE(r.p1_score == 7);
}

// ── unknown-type packet is discarded ─────────────────────────────────────────

TEST_CASE("loopback: unknown message type is discarded", "[loopback][unit]") {
    NetworkManager host2;
    REQUIRE(host2.startHost(0));
    unsigned short port2 = host2.localPort();

    sf::TcpSocket rawSender;
    rawSender.setBlocking(true);
    REQUIRE(rawSender.connect(sf::IpAddress::LocalHost, port2) == sf::Socket::Done);
    REQUIRE(host2.waitForClient(sf::seconds(5)));

    sf::Packet unknownPkt;
    unknownPkt << static_cast<sf::Uint8>(99);
    rawSender.send(unknownPkt);

    for (int i = 0; i < 100; ++i) {
        host2.poll();
        sf::sleep(sf::milliseconds(5));
    }

    REQUIRE(host2.stateBuf().empty());
    PlayerInput dummy;
    REQUIRE(!host2.nextInput(dummy));
}

// ── send-slot: rapid sends do not crash ──────────────────────────────────────
// The send slot is one-deep; a second sendInput call while the slot is occupied
// drops the new message.  On fast loopback the slot often flushes between calls,
// so we cannot reliably assert the drop count — we verify the channel works
// (first message delivered) and multiple rapid calls don't crash.

TEST_CASE("loopback: rapid sendInput calls do not crash", "[loopback][unit]") {
    LoopbackPair lb;

    PlayerInput a, b;
    a.up = true;
    b.down = true;

    lb.client.sendInput(a);
    lb.client.sendInput(b); // may or may not be dropped depending on flush timing

    // At least one message must be delivered
    PlayerInput recv;
    bool got = waitFor(lb.host, [&] { return lb.host.nextInput(recv); });
    REQUIRE(got); // at least one message arrived without crash
}

// ── disconnect() resets peerLeft ─────────────────────────────────────────────

TEST_CASE("loopback: disconnect() resets peerLeft on subsequent pair", "[loopback][unit]") {
    // Connect, send, disconnect host.  A fresh NetworkManager should not inherit
    // the old peerLeft state.
    NetworkManager host3;
    REQUIRE(host3.startHost(0));
    unsigned short port3 = host3.localPort();

    NetworkManager client3;
    REQUIRE(client3.connectToHost("127.0.0.1", port3));
    REQUIRE(host3.waitForClient(sf::seconds(5)));

    host3.disconnect();
    REQUIRE(!host3.isConnected());
    // peerLeft on host3 should be reset by disconnect()
    REQUIRE(!host3.peerLost());
}

// ── receive-side disconnect detection ────────────────────────────────────────

TEST_CASE("loopback: client disconnect detected by host poll", "[loopback][unit]") {
    LoopbackPair lb;

    lb.client.disconnect();

    bool detected = waitFor(lb.host, [&] { return !lb.host.isConnected() || lb.host.peerLost(); });
    REQUIRE(detected);
}

// ── send-side disconnect detection ───────────────────────────────────────────

TEST_CASE("loopback: host disconnect detected by client sendInput", "[loopback][unit]") {
    NetworkManager host4;
    REQUIRE(host4.startHost(0));
    unsigned short port4 = host4.localPort();

    NetworkManager client4;
    REQUIRE(client4.connectToHost("127.0.0.1", port4));
    REQUIRE(host4.waitForClient(sf::seconds(5)));
    REQUIRE(client4.isConnected());

    host4.disconnect();

    bool detected = false;
    PlayerInput inp;
    for (int i = 0; i < 300 && !detected; ++i) {
        client4.sendInput(inp);
        if (!client4.isConnected() || client4.peerLost())
            detected = true;
        sf::sleep(sf::milliseconds(5));
    }
    REQUIRE(detected);
}
