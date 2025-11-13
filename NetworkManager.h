#pragma once

#include <SFML/Network.hpp>
#include <string>
#include <memory>

enum class NetworkMode {
    LOCAL,
    HOST,
    CLIENT
};

struct PlayerInput {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool attack = false;
    
    // Serialize to packet
    void toPacket(sf::Packet& packet) const {
        packet << up << down << left << right << attack;
    }
    
    // Deserialize from packet
    void fromPacket(sf::Packet& packet) {
        packet >> up >> down >> left >> right >> attack;
    }
};

struct GameState {
    float p1_x, p1_y;
    float p2_x, p2_y;
    int p1_score;
    int p2_score;
    bool p1_left;
    bool p2_left;
    float p1_scale_x, p1_scale_y;
    float p2_scale_x, p2_scale_y;
    bool wait;
    
    void toPacket(sf::Packet& packet) const {
        packet << p1_x << p1_y << p2_x << p2_y 
               << p1_score << p2_score << p1_left << p2_left
               << p1_scale_x << p1_scale_y << p2_scale_x << p2_scale_y
               << wait;
    }
    
    void fromPacket(sf::Packet& packet) {
        packet >> p1_x >> p1_y >> p2_x >> p2_y 
               >> p1_score >> p2_score >> p1_left >> p2_left
               >> p1_scale_x >> p1_scale_y >> p2_scale_x >> p2_scale_y
               >> wait;
    }
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();
    
    // Host functions
    bool startHost(unsigned short port = 53000);
    bool waitForClient(sf::Time timeout = sf::Time::Zero);
    
    // Client functions
    bool connectToHost(const std::string& ip, unsigned short port = 53000);
    
    // Communication
    bool sendInput(const PlayerInput& input);
    bool receiveInput(PlayerInput& input);
    bool sendGameState(const GameState& state);
    bool receiveGameState(GameState& state);
    
    // Status
    bool isConnected() const;
    void disconnect();
    
private:
    sf::TcpListener m_listener;
    sf::TcpSocket m_socket;
    bool m_isHost;
    bool m_connected;
};
