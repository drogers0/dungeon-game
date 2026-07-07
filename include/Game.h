//
// Created by bswenson3 on 11/9/16.
//
#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <memory>
#include <tuple>
#include <vector>
#include "AnimatedGameObject.h"
#include "debug.h"
#include "GameObject.h"
#include "NetworkManager.h"
#include "RegularGameObject.h"
#include "replay.h"

class Game {
public:
    // use default screen size
    Game();
    Game(NetworkMode mode, std::shared_ptr<NetworkManager> netManager);

    // Configure debug/harness options; call before run().
    void setDebugConfig(const DebugConfig& cfg);

    // Accessor for step count (useful for post-run reporting).
    long long steps() const { return m_steps; }

    // Public snapshot of game state (for tests and network round-trip verification).
    GameState snapshot() const;

    // Apply a received network state (public so integration tests can verify round-trip).
    void      applyNetworkState(const GameState& state);

    // run the game
    std::tuple<int, int, float, int, bool, bool> run();

private:
    void processEvents();
    // update the game objects
    void update(sf::Time deltaT, float time);
    // draw the scene
    void render();
    // handle input from the user
    void handlePlayerInput(sf::Keyboard::Key key, bool isDown);
    // check collision with walls or other objects
    bool collision(const GameObject& a, const GameObject& b);
    bool collision(sf::Rect<float> a, const GameObject& b);

    // Fixed-timestep deterministic sim body.
    void simStep();
    // Apply a PlayerInput to the P2 bool state (w/a/s/d/space).
    void applyPlayerInput(const PlayerInput& in);
    // Capture a screenshot if the debug config says it is due this step.
    void captureIfDue();

    sf::RenderWindow m_window;

    sf::SoundBuffer gongbuffer;
    sf::SoundBuffer sbuffer;
    sf::SoundBuffer p2hitbuffer;
    sf::SoundBuffer laserbuffer;
    sf::SoundBuffer metalbuffer;
    sf::SoundBuffer speedbuffer;
    sf::SoundBuffer slowbuffer;
    sf::SoundBuffer burnbuffer;
    sf::Sound       gong;
    sf::Sound       sword;
    sf::Sound       p2hit;
    sf::Sound       laser;
    sf::Sound       metal;
    sf::Sound       speed_up;
    sf::Sound       slow_down;
    sf::Sound       burn;

    sf::Music background;

    std::unique_ptr<GameObject> m_rocket = std::make_unique<AnimatedGameObject>(404, 206, 3, 3, 9, 0);
    std::unique_ptr<GameObject> m_robot  = std::make_unique<AnimatedGameObject>(959, 180, 8, 1, 8, 0);
    std::vector<std::unique_ptr<GameObject>> m_arena;

    sf::Font font;
    sf::Font tfont;
    sf::Font block;
    sf::Text m_rocketScoreText;
    sf::Text m_robotScoreText;
    sf::Text timer;
    sf::Text pause_text;
    sf::Text info;

    float m_speed          = 1200.0f;
    float m_cooldownEnd    = 0;
    float m_p1HazardCooldownEnd       = 0;
    float m_p2HazardCooldownEnd       = 0;
    bool  m_p1Left           = false;
    bool  m_p1Right          = false;
    bool  m_p1Up             = false;
    bool  m_p1Down           = false;
    bool  m_p2Up           = false;
    bool  m_p2Left         = false;
    bool  m_p2Down         = false;
    bool  m_p2Right        = false;
    bool  m_p2Attack       = false;
    int   m_robotScore     = 0;
    int   m_rocketScore         = 0;
    bool  m_p1Attack       = false;
    bool  m_p1FacingLeft           = false;
    bool  m_p2FacingLeft           = true;
    bool  m_speedUpPressed   = false;
    bool  m_slowDownPressed  = false;
    bool  m_inCooldown     = false;
    bool  m_p1HazardReady   = true;
    bool  m_p2HazardReady   = true;

    // Fixed-timestep state
    static constexpr float kFixedDt = 1.f / 60.f;
    long long              m_steps      = 0;
    float                  m_accumulator = 0.f;
    float                  m_animTime   = 0.f;

    double gameSeconds() const { return m_steps * static_cast<double>(kFixedDt); }

    // Debug / harness configuration
    DebugConfig              m_debug;
    std::vector<PlayerInput> m_replay;
    std::size_t              m_replayIdx = 0;
    std::vector<PlayerInput> m_replayP1;
    std::size_t              m_replayP1Idx = 0;
    long long                m_nextShot  = 0; // next step at which to auto-screenshot

    // Network support
    NetworkMode                      m_networkMode = NetworkMode::LOCAL;
    std::shared_ptr<NetworkManager>  m_networkManager;
    void      handleNetworkCommunication(sf::Time deltaT);
    GameState captureGameState() const;

    bool      m_peerLeft   = false;
    GameState m_latestState;
    float     m_stateAccum = 0.f;
    static constexpr float kStateSendInterval = 1.f / 25.f; // ~25 Hz
};
