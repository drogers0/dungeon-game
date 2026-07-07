// Loopback integration tests: exercises real socket paths in a single process.
// HOST and CLIENT NetworkManagers communicate over 127.0.0.1 on an ephemeral
// port (port 0 → OS picks one), so the test never conflicts with a running game.
//
// Covered paths:
//   1. client sendInput  → host poll / nextInput
//   2. host sendGameState → client poll / stateBuf
//   3. unknown-type discard via raw TcpSocket (same framing, type byte = 99)
//   4. receive-side disconnect detection: client disconnect → host peerLost()
//   5. send-side disconnect detection: host drops → client sendInput path → peerLost()

#include <cassert>
#include <cmath>
#include <iostream>

#include <SFML/Network.hpp>
#include "NetworkManager.h"

// Poll nm up to maxTries × sleepMs, returning true as soon as cond() is true.
template <typename F>
static bool waitFor(NetworkManager& nm, F cond, int maxTries = 200, int sleepMs = 5)
{
    for (int i = 0; i < maxTries; ++i) {
        nm.poll();
        if (cond()) return true;
        sf::sleep(sf::milliseconds(sleepMs));
    }
    return false;
}

int main()
{
    // ── Setup: host listens on an ephemeral port, client connects ─────────────
    NetworkManager host;
    assert(host.startHost(0));
    unsigned short port = host.localPort();
    assert(port != 0);

    NetworkManager client;
    assert(client.connectToHost("127.0.0.1", port));

    // Host's accept loop (non-blocking listener; returns quickly for loopback).
    assert(host.waitForClient(sf::seconds(5)));

    assert(host.isConnected());
    assert(client.isConnected());

    // ── Test 1: client sendInput → host poll / nextInput ─────────────────────
    {
        PlayerInput sent;
        sent.up = true; sent.attack = true;
        assert(client.sendInput(sent));

        PlayerInput recv;
        bool got = waitFor(host, [&]{ return host.nextInput(recv); });
        assert(got);
        assert(recv.up     == true);
        assert(recv.attack == true);
        assert(recv.down   == false);
        assert(recv.left   == false);
    }

    // ── Test 2: host sendGameState → client poll / stateBuf ──────────────────
    {
        GameState sent;
        sent.p1_x = 123.f; sent.p2_y = 456.f; sent.p1_score = 7;
        assert(host.sendGameState(sent));

        bool got = waitFor(client, [&]{ return !client.stateBuf().empty(); });
        assert(got);
        const GameState& r = client.stateBuf().back().state;
        assert(std::abs(r.p1_x - 123.f) < 1e-4f);
        assert(std::abs(r.p2_y - 456.f) < 1e-4f);
        assert(r.p1_score == 7);
    }

    // ── Test 3: unknown-type packet is discarded (raw TcpSocket sender) ───────
    {
        NetworkManager host2;
        assert(host2.startHost(0));
        unsigned short port2 = host2.localPort();

        sf::TcpSocket rawSender;
        rawSender.setBlocking(true);
        assert(rawSender.connect(sf::IpAddress::LocalHost, port2) == sf::Socket::Done);
        assert(host2.waitForClient(sf::seconds(5)));

        sf::Packet unknownPkt;
        unknownPkt << static_cast<sf::Uint8>(99);
        rawSender.send(unknownPkt); // SFML framing: 4-byte length prefix

        // Poll in a bounded loop to give the packet time to arrive.
        for (int i = 0; i < 100; ++i) {
            host2.poll();
            sf::sleep(sf::milliseconds(5));
        }

        // Buffers must be empty — the packet was discarded, not crashed on.
        assert(host2.stateBuf().empty());
        PlayerInput dummy;
        assert(!host2.nextInput(dummy));
    }

    // ── Test 4: receive-side disconnect detection ────────────────────────────
    // Disconnect the client; host should detect it via poll() (receive path).
    {
        client.disconnect();

        bool detected = waitFor(host,
            [&]{ return !host.isConnected() || host.peerLost(); });
        assert(detected);
    }

    // ── Test 5: send-side disconnect detection ───────────────────────────────
    // Host drops abruptly; the CLIENT (the frequent sender) must detect the dead
    // connection through its SEND path (flushSend → Error/Disconnected →
    // setDisconnected) with NO poll() — the OS may buffer the first writes before
    // reporting the reset, so retry a bounded number of sends.
    {
        NetworkManager host3;
        assert(host3.startHost(0));
        unsigned short port3 = host3.localPort();

        NetworkManager client3;
        assert(client3.connectToHost("127.0.0.1", port3));
        assert(host3.waitForClient(sf::seconds(5)));
        assert(client3.isConnected());

        host3.disconnect(); // host gone

        bool detected = false;
        PlayerInput inp;
        for (int i = 0; i < 300 && !detected; ++i) {
            client3.sendInput(inp); // send path only — no poll()
            if (!client3.isConnected() || client3.peerLost()) detected = true;
            sf::sleep(sf::milliseconds(5));
        }
        assert(detected);
    }

    std::cout << "All loopback integration tests passed.\n";
    return 0;
}
