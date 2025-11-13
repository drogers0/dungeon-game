#include "NetworkManager.h"
#include <iostream>

NetworkManager::NetworkManager() 
    : m_isHost(false), m_connected(false) {
    m_socket.setBlocking(false);
}

NetworkManager::~NetworkManager() {
    disconnect();
}

bool NetworkManager::startHost(unsigned short port) {
    m_isHost = true;
    if (m_listener.listen(port) != sf::Socket::Done) {
        std::cout << "Failed to bind to port " << port << std::endl;
        return false;
    }
    m_listener.setBlocking(false);
    std::cout << "Hosting on port " << port << std::endl;
    return true;
}

bool NetworkManager::waitForClient(sf::Time timeout) {
    if (!m_isHost) return false;
    
    sf::Clock clock;
    while (timeout == sf::Time::Zero || clock.getElapsedTime() < timeout) {
        if (m_listener.accept(m_socket) == sf::Socket::Done) {
            std::cout << "Client connected: " << m_socket.getRemoteAddress() << std::endl;
            m_socket.setBlocking(false);
            m_connected = true;
            return true;
        }
        sf::sleep(sf::milliseconds(10));
    }
    return false;
}

bool NetworkManager::connectToHost(const std::string& ip, unsigned short port) {
    m_isHost = false;
    std::cout << "Connecting to " << ip << ":" << port << std::endl;
    
    if (m_socket.connect(ip, port, sf::seconds(5)) != sf::Socket::Done) {
        std::cout << "Failed to connect to host" << std::endl;
        return false;
    }
    
    m_socket.setBlocking(false);
    m_connected = true;
    std::cout << "Connected to host" << std::endl;
    return true;
}

bool NetworkManager::sendInput(const PlayerInput& input) {
    if (!m_connected) return false;
    
    sf::Packet packet;
    packet << static_cast<sf::Uint8>(1); // Message type: input
    input.toPacket(packet);
    
    return m_socket.send(packet) == sf::Socket::Done;
}

bool NetworkManager::receiveInput(PlayerInput& input) {
    if (!m_connected) return false;
    
    sf::Packet packet;
    sf::Socket::Status status = m_socket.receive(packet);
    
    if (status == sf::Socket::Done) {
        sf::Uint8 messageType;
        packet >> messageType;
        if (messageType == 1) {
            input.fromPacket(packet);
            return true;
        }
    }
    return false;
}

bool NetworkManager::sendGameState(const GameState& state) {
    if (!m_connected) return false;
    
    sf::Packet packet;
    packet << static_cast<sf::Uint8>(2); // Message type: game state
    state.toPacket(packet);
    
    return m_socket.send(packet) == sf::Socket::Done;
}

bool NetworkManager::receiveGameState(GameState& state) {
    if (!m_connected) return false;
    
    sf::Packet packet;
    sf::Socket::Status status = m_socket.receive(packet);
    
    if (status == sf::Socket::Done) {
        sf::Uint8 messageType;
        packet >> messageType;
        if (messageType == 2) {
            state.fromPacket(packet);
            return true;
        }
    }
    return false;
}

bool NetworkManager::isConnected() const {
    return m_connected;
}

void NetworkManager::disconnect() {
    m_socket.disconnect();
    m_listener.close();
    m_connected = false;
}
