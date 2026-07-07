#include "NetworkManager.h"
#include <algorithm>
#include <iostream>

// ── free functions ────────────────────────────────────────────────────────────

void dispatchPacket(sf::Packet& pkt, sf::Time arrival, std::deque<TimedState>& stateBuf,
                    std::deque<PlayerInput>& inputQueue, std::size_t cap) {
    sf::Uint8 typeRaw = 0;
    if (!(pkt >> typeRaw))
        return;

    switch (static_cast<MsgType>(typeRaw)) {
    case MsgType::State: {
        TimedState ts;
        ts.arrival = arrival;
        ts.state.fromPacket(pkt);
        if (stateBuf.size() >= cap)
            stateBuf.pop_front();
        stateBuf.push_back(ts);
        break;
    }
    case MsgType::Input: {
        PlayerInput inp;
        inp.fromPacket(pkt);
        inputQueue.push_back(inp);
        break;
    }
    default: {
        static bool warned = false;
        if (!warned) {
            std::cerr << "NetworkManager: unknown message type " << static_cast<int>(typeRaw)
                      << " — discarding\n";
            warned = true;
        }
        break;
    }
    }
}

sf::Vector2f lerpPos(sf::Vector2f from, sf::Vector2f to, float t) {
    t = std::max(0.f, std::min(1.f, t));
    return from + (to - from) * t;
}

// ── NetworkManager ────────────────────────────────────────────────────────────

NetworkManager::NetworkManager() = default;
NetworkManager::~NetworkManager() { disconnect(); }

void NetworkManager::setDisconnected() {
    m_connected = false;
    m_peerLeft = true;
}

// Try to send the pending packet; returns true when the slot is now clear.
bool NetworkManager::flushSend() {
    if (!m_hasPending)
        return true;
    auto status = m_socket.send(m_pendingSend);
    if (status == sf::Socket::Done) {
        m_hasPending = false;
        return true;
    }
    if (status == sf::Socket::Disconnected || status == sf::Socket::Error)
        setDisconnected();
    // Partial: SFML 2.x copies the payload into the SOCKET's internal buffer on the
    // first send() and stores progress there; the next send() on this socket resumes
    // from that buffer regardless of which packet instance is passed (we pass the same
    // untouched m_pendingSend). Slot stays occupied; caller drops its new message.
    return false;
}

bool NetworkManager::startHost(unsigned short port) {
    m_isHost = true;
    m_listener.setBlocking(false);
    if (m_listener.listen(port) != sf::Socket::Done) {
        std::cout << "Failed to bind to port " << port << "\n";
        return false;
    }
    std::cout << "Hosting on port " << m_listener.getLocalPort() << "\n";
    std::cout << "Your IP: " << sf::IpAddress::getLocalAddress() << "\n";
    return true;
}

bool NetworkManager::waitForClient(sf::Time timeout) {
    if (!m_isHost || m_connected)
        return false; // already accepted; listener is closed
    sf::Clock clock;
    while (timeout == sf::Time::Zero || clock.getElapsedTime() < timeout) {
        if (m_listener.accept(m_socket) == sf::Socket::Done) {
            std::cout << "Client connected: " << m_socket.getRemoteAddress() << "\n";
            m_socket.setBlocking(false);
            m_connected = true;
            m_listener.close(); // 1v1: stop accepting new connections
            return true;
        }
        sf::sleep(sf::milliseconds(10));
    }
    return false;
}

bool NetworkManager::connectToHost(const std::string& ip, unsigned short port) {
    // Validate ip before doing any blocking work — bad input fails instantly.
    sf::IpAddress addr(ip);
    if (addr == sf::IpAddress::None) {
        std::cout << "Invalid IP address: " << ip << "\n";
        return false;
    }
    if (port < 1024) { // unsigned short caps at 65535, so only the low bound is meaningful
        std::cout << "Invalid port: " << port << "\n";
        return false;
    }

    m_isHost = false;
    std::cout << "Connecting to " << ip << ":" << port << "\n";
    m_socket.setBlocking(true);
    auto status = m_socket.connect(addr, port, sf::seconds(10));
    m_socket.setBlocking(false);

    if (status != sf::Socket::Done) {
        std::cout << "Failed to connect (status: " << status << ")\n";
        return false;
    }
    m_connected = true;
    std::cout << "Connected!\n";
    return true;
}

bool NetworkManager::sendInput(const PlayerInput& input) {
    if (!m_connected)
        return false;
    if (!flushSend())
        return false; // pending slot still occupied — drop

    m_pendingSend.clear();
    m_pendingSend << static_cast<sf::Uint8>(MsgType::Input);
    input.toPacket(m_pendingSend);
    m_hasPending = true;
    return flushSend();
}

bool NetworkManager::sendGameState(const GameState& state) {
    if (!m_connected)
        return false;
    if (!flushSend())
        return false;

    m_pendingSend.clear();
    m_pendingSend << static_cast<sf::Uint8>(MsgType::State);
    state.toPacket(m_pendingSend);
    m_hasPending = true;
    return flushSend();
}

void NetworkManager::poll() {
    if (!m_connected)
        return;
    sf::Packet pkt;
    for (;;) {
        auto status = m_socket.receive(pkt);
        if (status == sf::Socket::Done) {
            dispatchPacket(pkt, m_clock.getElapsedTime(), m_stateBuf, m_inputQueue, kStateBufCap);
        } else if (status == sf::Socket::NotReady || status == sf::Socket::Partial) {
            // NotReady: no data yet.
            // Partial: SFML stores receive progress inside the socket; resume next poll().
            break;
        } else { // Disconnected or Error
            setDisconnected();
            break;
        }
    }
}

bool NetworkManager::nextInput(PlayerInput& input) {
    if (m_inputQueue.empty())
        return false;
    input = m_inputQueue.front();
    m_inputQueue.pop_front();
    return true;
}

void NetworkManager::disconnect() {
    m_socket.disconnect();
    m_listener.close();
    m_connected = false;
    m_peerLeft = false;
    m_hasPending = false; // reset all transient state so a reused manager starts clean
    m_stateBuf.clear();
    m_inputQueue.clear();
}
