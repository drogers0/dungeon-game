#pragma once

#include <SFML/Network.hpp>
#include <deque>
#include <string>

enum class NetworkMode { LOCAL, HOST, CLIENT };

enum class MsgType : sf::Uint8 { Input = 1, State = 2 };

struct PlayerInput {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool attack = false;

    void toPacket(sf::Packet& packet) const { packet << up << down << left << right << attack; }
    void fromPacket(sf::Packet& packet) { packet >> up >> down >> left >> right >> attack; }
};

struct GameState {
    float p1_x = 0, p1_y = 0;
    float p2_x = 0, p2_y = 0;
    int p1_score = 0, p2_score = 0;
    bool p1_left = false, p2_left = false;
    float p1_scale_x = 1, p1_scale_y = 1;
    float p2_scale_x = 1, p2_scale_y = 1;
    bool wait = false;

    void toPacket(sf::Packet& packet) const {
        packet << p1_x << p1_y << p2_x << p2_y << p1_score << p2_score << p1_left << p2_left
               << p1_scale_x << p1_scale_y << p2_scale_x << p2_scale_y << wait;
    }
    void fromPacket(sf::Packet& packet) {
        packet >> p1_x >> p1_y >> p2_x >> p2_y >> p1_score >> p2_score >> p1_left >> p2_left >>
            p1_scale_x >> p1_scale_y >> p2_scale_x >> p2_scale_y >> wait;
    }
};

// A GameState snapshot paired with its receive timestamp.
struct TimedState {
    GameState state;
    sf::Time arrival;
};

// Socket-free demux: reads the leading MsgType byte and routes into the
// appropriate buffer.  Unknown types are logged once and discarded.
// cap limits stateBuf size (oldest entry evicted when full).
void dispatchPacket(sf::Packet& pkt, sf::Time arrival, std::deque<TimedState>& stateBuf,
                    std::deque<PlayerInput>& inputQueue, std::size_t cap);

// Pure linear interpolation between two 2-D positions; t is clamped to [0,1].
sf::Vector2f lerpPos(sf::Vector2f from, sf::Vector2f to, float t);

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // Host API
    bool startHost(unsigned short port = 53000);
    bool waitForClient(sf::Time timeout = sf::Time::Zero);

    // Client API — validates ip before blocking connect; bad IP fails instantly.
    bool connectToHost(const std::string& ip, unsigned short port = 53000);

    // Send (one-slot pending; drops new message if slot not yet cleared)
    bool sendInput(const PlayerInput& input);
    bool sendGameState(const GameState& state);

    // Drain socket into internal buffers; call every frame.
    void poll();

    // Pop oldest queued PlayerInput (returns false when queue empty).
    bool nextInput(PlayerInput& input);

    // Read-only view of the state buffer; newest snapshot is at back().
    const std::deque<TimedState>& stateBuf() const { return m_stateBuf; }

    // Elapsed time on the NetworkManager's internal clock (used by render interpolation).
    sf::Time elapsed() const { return m_clock.getElapsedTime(); }

    // Port the listener is bound to (useful for ephemeral-port testing).
    unsigned short localPort() const { return m_listener.getLocalPort(); }

    // Status
    bool isConnected() const { return m_connected; }
    bool peerLost() const { return m_peerLeft; }
    void disconnect();

private:
    void setDisconnected();
    bool flushSend(); // returns true when the pending slot is clear

    sf::TcpListener m_listener;
    sf::TcpSocket m_socket;
    sf::Clock m_clock;

    bool m_isHost = false;
    bool m_connected = false;
    bool m_peerLeft = false;

    // One-slot pending send (no queue).
    sf::Packet m_pendingSend;
    bool m_hasPending = false;

    std::deque<TimedState> m_stateBuf;
    std::deque<PlayerInput> m_inputQueue;

    static constexpr std::size_t kStateBufCap = 8;
};
