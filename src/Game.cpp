//
// Created by bswenson3 on 11/9/16.
//

#include "Game.h"
#include "asset_load.h"
#include "geometry.h"
#include "letterbox.h"
#include "resource_path.h"
#include "rng.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>

// ── helpers ──────────────────────────────────────────────────────────────────

// LCOV_EXCL_START — screenshot requires a live RenderWindow; not testable headless
static void captureScreenshot(const sf::RenderWindow& w, const std::string& path) {
    sf::Texture t;
    t.create(w.getSize().x, w.getSize().y);
    t.update(w);
    t.copyToImage().saveToFile(path);
}
// LCOV_EXCL_STOP

// ── toggleFullscreen ──────────────────────────────────────────────────────────
// LCOV_EXCL_START — requires a display; not reachable from harness tests
void Game::toggleFullscreen() {
    m_fullscreen = !m_fullscreen;
    const std::string title = m_networkMode == NetworkMode::LOCAL  ? "Dungeon Game"
                              : m_networkMode == NetworkMode::HOST ? "Dungeon Game - Host"
                                                                   : "Dungeon Game - Client";
    if (m_fullscreen) {
        m_window.create(sf::VideoMode::getDesktopMode(), title, sf::Style::Fullscreen);
    } else {
        m_window.create(sf::VideoMode(1920, 1080), title, sf::Style::Default);
    }
    // Re-apply display settings after create() resets them.
    // NOTE (macOS): SFML 2 create() reconstructs the GL context; textures
    // live on the shared context and should survive, but verify visually.
    if (!m_debug.active())
        m_window.setVerticalSyncEnabled(true);
    m_window.setView(makeLetterboxView({kLogicalW, kLogicalH}, m_window.getSize()));
}
// LCOV_EXCL_STOP

// ── constructors ──────────────────────────────────────────────────────────────

Game::Game() : Game(NetworkMode::LOCAL, nullptr) {}

Game::Game(NetworkMode mode, std::shared_ptr<NetworkManager> netManager)
    : m_window(sf::VideoMode(1920, 1080),
               mode == NetworkMode::LOCAL  ? "Dungeon Game"
               : mode == NetworkMode::HOST ? "Dungeon Game - Host"
                                           : "Dungeon Game - Client",
               sf::Style::Default),
      m_networkMode(mode), m_networkManager(netManager) {
    loadOrThrow(background, resource_path + "background.wav");

    loadOrThrow(*m_rocket, resource_path + "rocket.png");
    m_rocket->setScale(2.0f);
    m_rocket->setPosition(m_rocket->getWidth(), 400);

    loadOrThrow(*m_robot, resource_path + "robot.png");
    m_robot->setScale(-1.0f, 1.0f);
    m_robot->setPosition(kLogicalW - (m_rocket->getWidth() + 150), 400);

    m_arena.push_back(std::make_unique<RegularGameObject>());
    loadOrThrow(*m_arena[0], resource_path + "brick.png");
    m_arena[0]->setScale(2.0f);

    {
        auto fire = std::make_unique<AnimatedGameObject>(216, 216, 5, 3, 10, 0);
        loadOrThrow(*fire, resource_path + "fire.png");
        fire->setScale(2.0f);
        fire->setPosition(-15, 400);
        m_arena.push_back(std::move(fire));
    }
    {
        auto fire2 = std::make_unique<AnimatedGameObject>(216, 216, 5, 3, 10, 0);
        loadOrThrow(*fire2, resource_path + "fire.png");
        fire2->setScale(2.0f);
        fire2->setPosition(kLogicalW / 2 - 5, 400);
        m_arena.push_back(std::move(fire2));
    }
    {
        auto fire3 = std::make_unique<AnimatedGameObject>(216, 216, 5, 3, 10, 0);
        loadOrThrow(*fire3, resource_path + "fire.png");
        fire3->setScale(2.0f);
        fire3->setPosition(kLogicalW - 100, 400);
        m_arena.push_back(std::move(fire3));
    }

    loadOrThrow(font, resource_path + "oswald.ttf");
    loadOrThrow(tfont, resource_path + "timer.ttf");
    loadOrThrow(block, resource_path + "Blockt.ttf");

    pause_text = sf::Text("Cooldown", block, 400);
    pause_text.setPosition(kLogicalW / 2 - 850, 100);
    pause_text.setFillColor(sf::Color::Black);

    info = sf::Text("a short intermission", font, 50);
    info.setPosition(pause_text.getPosition().x + 150, pause_text.getPosition().y + 500);
    info.setFillColor(sf::Color::Black);

    m_rocketScoreText = sf::Text("Rocket Score: 0", font, 40);
    m_robotScoreText = sf::Text("Robot Score: 0", font, 40);
    timer = sf::Text("timer", tfont, 60);
    timer.setPosition(kLogicalW / 2 - 60, 0);
    m_rocketScoreText.setPosition(10, 0);
    m_robotScoreText.setPosition(kLogicalW - m_robotScoreText.getGlobalBounds().width, 0);
    m_robotScoreText.setFillColor(sf::Color::Green);
    timer.setFillColor(sf::Color::Black);
    m_rocketScoreText.setFillColor(sf::Color::Blue);

    loadOrThrow(sbuffer, resource_path + "sword_miss.wav");
    sword.setBuffer(sbuffer);
    loadOrThrow(p2hitbuffer, resource_path + "skeleton.wav");
    p2hit.setBuffer(p2hitbuffer);
    loadOrThrow(laserbuffer, resource_path + "new_laser.wav");
    laser.setBuffer(laserbuffer);
    loadOrThrow(metalbuffer, resource_path + "metal_hit.wav");
    metal.setBuffer(metalbuffer);
    loadOrThrow(speedbuffer, resource_path + "SpeedUp.wav");
    speed_up.setBuffer(speedbuffer);
    loadOrThrow(slowbuffer, resource_path + "SlowDown.wav");
    slow_down.setBuffer(slowbuffer);
    loadOrThrow(burnbuffer, resource_path + "burn.wav");
    burn.setBuffer(burnbuffer);
    loadOrThrow(gongbuffer, resource_path + "gong.wav");
    gong.setBuffer(gongbuffer);
    laser.setVolume(60);
}

// ── debug config ──────────────────────────────────────────────────────────────

void Game::setDebugConfig(const DebugConfig& cfg) {
    m_debug = cfg;
    if (!cfg.replayPath.empty()) {
        m_replay = loadReplay(cfg.replayPath);
        m_replayIdx = 0;
    }
    if (!cfg.replayPathP1.empty()) {
        m_replayP1 = loadReplay(cfg.replayPathP1);
        m_replayP1Idx = 0;
    }
    if (cfg.ai != AiDifficulty::None)
        setAiOpponent(cfg.ai);
}

// ── AI opponent ──────────────────────────────────────────────────────────────

void Game::setAiOpponent(AiDifficulty d) {
    if (d != AiDifficulty::None)
        m_ai = std::make_unique<AiController>(paramsFor(d), rng::engine());
    else
        m_ai.reset();
}

AiView Game::makeAiView() const {
    AiView view;
    view.selfPos = m_robot->getPosition();

    // Both bounds must be normalised to positive width/height: either fighter
    // may face left (scale.x = -1), which makes objectBounds negative-width.
    // The decider's nearest-edge math needs a canonical left/width, so use
    // normalizedBounds for self AND opponent (opp normalisation matters when
    // P1 faces left while the robot is to its left — otherwise nearOppEdge
    // picks the far edge and the AI thinks it is a full body-width too far).
    view.selfBounds = normalizedBounds(*m_robot);

    view.selfFacingLeft = m_p2FacingLeft;
    view.oppPos = m_rocket->getPosition();
    view.oppBounds = normalizedBounds(*m_rocket);

    // arena[0] is the brick floor; arena[1..3] are the three fire hazards.
    for (int i = 0; i < 3; ++i)
        view.hazards[static_cast<std::size_t>(i)] =
            objectBounds(*m_arena[static_cast<std::size_t>(i + 1)]);

    view.inCooldown = m_inCooldown;
    view.selfScore = m_robotScore;
    view.oppScore = m_rocketScore;
    view.step = m_steps;
    return view;
}

// ── run ───────────────────────────────────────────────────────────────────────

std::tuple<int, int, float, int, bool, bool> Game::run() {
    // Apply display settings now that debug config is known.
    if (!m_debug.active())
        m_window.setVerticalSyncEnabled(true);
    m_window.setView(makeLetterboxView({kLogicalW, kLogicalH}, m_window.getSize()));

    sf::Clock clock;
    background.play();
    background.setLoop(true);
    background.setVolume(40);
    gong.setVolume(50);

    float apieceofcrap = 0.f;
    int seconds = 0;
    bool p1win = false;
    int framesLeft = m_debug.frames;
    const bool framesMode = (framesLeft > 0);

    while (m_window.isOpen()) {
        // ── Timer display from step counter ──────────────────────────────────
        {
            int totalSecs = static_cast<int>(gameSeconds());
            apieceofcrap = static_cast<float>(totalSecs / 60);
            seconds = totalSecs % 60;
            char gClock[10];
            std::snprintf(gClock, sizeof(gClock), "%02.0f:%02d", apieceofcrap, seconds);
            timer.setString(gClock);
        }

        float iterDt = std::min(clock.restart().asSeconds(), 0.25f);

        processEvents();

        // ── Network I/O: once per iteration (not per step) ───────────────────
        if (m_networkManager) {
            handleNetworkCommunication(sf::seconds(iterDt));
        }

        // ── Sim steps ────────────────────────────────────────────────────────
        if (framesMode) {
            simStep();
        } else if (!m_paused || m_networkMode != NetworkMode::LOCAL) {
            m_accumulator += iterDt;
            // Guard on isOpen() so a simStep() that closes the window mid-drain
            // (e.g. debug quitAtStep) can't overshoot the target step count.
            while (m_accumulator >= kFixedDt && m_window.isOpen()) {
                simStep();
                m_accumulator -= kFixedDt;
            }
        }
        // When LOCAL-paused, iterDt was consumed by clock.restart() above;
        // skipping the accumulator prevents a time-jump on unpause.

        // ── Render + capture (back buffer ready; swap comes last) ─────────────
        render();
        captureIfDue();
        m_window.display();

        // ── Frame-count exit ─────────────────────────────────────────────────
        if (framesMode) {
            --framesLeft;
            if (framesLeft <= 0)
                m_window.close();
        }
    }

    background.stop();
    p1win = (m_rocketScore > m_robotScore);
    return {m_rocketScore, m_robotScore, apieceofcrap, seconds, p1win, m_peerLeft};
}

// ── simStep ───────────────────────────────────────────────────────────────────

void Game::simStep() {
    // Apply replay input for P2 (before update so the flags are set when update() runs).
    if (!m_replay.empty()) {
        if (m_replayIdx < m_replay.size()) {
            applyPlayerInput(m_replay[m_replayIdx++]);
        } else {
            applyPlayerInput(PlayerInput{}); // EOF → all-false
        }
    } else if (m_ai) {
        applyPlayerInput(m_ai->step(makeAiView()));
    }

    // Apply replay input for P1.
    if (!m_replayP1.empty()) {
        if (m_replayP1Idx < m_replayP1.size()) {
            applyInputToP1(m_p1Up, m_p1Left, m_p1Down, m_p1Right, m_p1Attack,
                           m_replayP1[m_replayP1Idx++]);
        } else {
            PlayerInput empty{};
            applyInputToP1(m_p1Up, m_p1Left, m_p1Down, m_p1Right, m_p1Attack, empty);
        }
    }

    // Update or drain the cooldown wait.
    if (!m_inCooldown) {
        update(sf::seconds(kFixedDt), m_animTime);
        m_cooldownEnd = static_cast<float>(gameSeconds()) + 1.75f;
    } else if (gameSeconds() > static_cast<double>(m_cooldownEnd)) {
        m_inCooldown = false;
        gong.play();
    }

    // Animation accumulator (matches original pattern: reset-then-increment).
    if (m_animTime > .1f)
        m_animTime = 0;
    m_animTime += kFixedDt;

    // Timeout checks.
    if (gameSeconds() > static_cast<double>(m_p1HazardCooldownEnd))
        m_p1HazardReady = true;
    if (gameSeconds() > static_cast<double>(m_p2HazardCooldownEnd))
        m_p2HazardReady = true;

    // Hazard collision + scoring.
    for (int x = 1; x < static_cast<int>(m_arena.size()); x++) {
        if (collision(*m_arena[x], *m_rocket) && m_p1HazardReady) {
            burn.play();
            m_rocketScore--;
            m_p1HazardReady = false;
            m_p1HazardCooldownEnd = static_cast<float>(gameSeconds()) + 2.2f;
        }
        if (collision(*m_arena[x], *m_robot) && m_p2HazardReady) {
            burn.play();
            m_robotScore--;
            m_p2HazardReady = false;
            m_p2HazardCooldownEnd = static_cast<float>(gameSeconds()) + 2.2f;
        }
    }

    // Score clamps.
    if (m_robotScore < 0)
        m_robotScore = 0;
    if (m_rocketScore < 0)
        m_rocketScore = 0;

    ++m_steps;

    // quitAtStep: harness-controlled exit that sets quitToMenu so callers can
    // distinguish it from the frames-limit exit.
    if (m_debug.quitAtStep >= 0 && m_steps >= static_cast<long long>(m_debug.quitAtStep)) {
        m_quitToMenu = true;
        m_window.close();
    }
}

// ── applyPlayerInput / captureIfDue ──────────────────────────────────────────

void Game::applyPlayerInput(const PlayerInput& in) {
    applyInputTo(m_p2Up, m_p2Left, m_p2Down, m_p2Right, m_p2Attack, in);
}

void Game::captureIfDue() {
    if (m_debug.screenshotEvery <= 0)
        return;
    if (m_nextShot == 0)
        m_nextShot = m_debug.screenshotEvery; // lazy init
    // Threshold (not modulo) so a multi-step accumulator drain that jumps past a
    // boundary still captures once; --frames mode (1 step/iter) lands exactly on
    // each multiple, keeping frame_60/120/... filenames stable for determinism.
    if (m_steps >= m_nextShot) {
        captureScreenshot(m_window,
                          m_debug.screenshotDir + "/frame_" + std::to_string(m_steps) + ".png");
        while (m_nextShot <= m_steps)
            m_nextShot += m_debug.screenshotEvery;
    }
}

// ── processEvents ─────────────────────────────────────────────────────────────

void Game::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        switch (event.type) {
        // LCOV_EXCL_START — UI events not generated in harness/headless runs
        case sf::Event::KeyPressed:
            if (event.key.code == sf::Keyboard::F11 && !m_debug.active()) {
                toggleFullscreen();
                return; // restart event loop with the new window
            }
            handlePlayerInput(event.key.code, true);
            break;
        case sf::Event::KeyReleased:
            handlePlayerInput(event.key.code, false);
            break;
        case sf::Event::Resized:
            m_window.setView(
                makeLetterboxView({kLogicalW, kLogicalH}, {event.size.width, event.size.height}));
            break;
        case sf::Event::Closed:
            m_window.close();
            break;
        // LCOV_EXCL_STOP
        default:
            break;
        }
    }
}

// ── handlePlayerInput ─────────────────────────────────────────────────────────

// LCOV_EXCL_START — keyboard glue; entirely gated off when the harness is active
void Game::handlePlayerInput(sf::Keyboard::Key key, bool isDown) {
    // Gate all interactive bindings when the harness is active so that live
    // keyboard cannot perturb a --frames / --replay run.
    if (!m_debug.active()) {
        // Pause / resume
        if (key == sf::Keyboard::Escape && isDown) {
            m_paused = !m_paused;
            if (m_paused)
                background.pause();
            else
                background.play();
        }

        // Quit to menu while paused
        if (m_paused && key == sf::Keyboard::Q && isDown) {
            if (m_networkManager)
                m_networkManager->disconnect();
            m_quitToMenu = true;
            m_window.close();
        }

        // Player 1 controls (rocket — configurable, default numpad)
        if (m_networkMode == NetworkMode::LOCAL || m_networkMode == NetworkMode::HOST) {
            if (key == m_bindings.p1.left)
                m_p1Left = isDown;
            if (key == m_bindings.p1.right)
                m_p1Right = isDown;
            if (key == m_bindings.p1.up)
                m_p1Up = isDown;
            if (key == m_bindings.p1.down)
                m_p1Down = isDown;
            if (key == m_bindings.p1.attack)
                m_p1Attack = isDown;
        }

        // Player 2 controls (robot — configurable, default WASD+Space)
        // Gated off when AI is driving P2.
        if ((m_networkMode == NetworkMode::LOCAL || m_networkMode == NetworkMode::CLIENT) &&
            !m_ai) {
            if (key == m_bindings.p2.up)
                m_p2Up = isDown;
            if (key == m_bindings.p2.left)
                m_p2Left = isDown;
            if (key == m_bindings.p2.down)
                m_p2Down = isDown;
            if (key == m_bindings.p2.right)
                m_p2Right = isDown;
            if (key == m_bindings.p2.attack)
                m_p2Attack = !isDown;
        }

        // Speed tweaks and wait-skip
        if (key == m_bindings.slowDown)
            m_slowDownPressed = !isDown;
        if (key == m_bindings.speedUp)
            m_speedUpPressed = !isDown;
        if (key == m_bindings.skipCooldown && isDown) {
            if (m_inCooldown) {
                m_inCooldown = false;
                gong.play();
            }
        }
    }

    // Always-active: manual screenshot (F12).
    if (key == sf::Keyboard::F12 && isDown)
        captureScreenshot(m_window,
                          m_debug.screenshotDir + "/manual_" + std::to_string(m_steps) + ".png");
}
// LCOV_EXCL_STOP

// ── update ────────────────────────────────────────────────────────────────────

void Game::update(sf::Time deltaT, float time) {
    bool reset = false;

    sf::Vector2f p1movement(0.0f, 0.0f);
    sf::Vector2f p2movement(0.0f, 0.0f);

    if (m_p1Up) {
        if (m_rocket->getPosition().y < -(m_rocket->getHeight())) {
            m_rocket->setPosition(m_rocket->getPosition().x, kLogicalH - m_rocket->getHeight());
        }

        if (collision(*m_rocket, *m_robot)) {
            m_rocket->setPosition(m_rocket->getPosition().x, m_rocket->getPosition().y + 40);
        } else {
            p1movement.y -= m_speed;
        }
    }

    if (m_p1Down) {
        if (m_rocket->getPosition().y > kLogicalH - m_rocket->getHeight()) {
            m_rocket->setPosition(m_rocket->getPosition().x, -(m_rocket->getHeight()));
        }

        if (collision(*m_rocket, *m_robot)) {
            m_rocket->setPosition(m_rocket->getPosition().x, m_rocket->getPosition().y - 40);
        } else {
            p1movement.y += m_speed;
        }
    }
    if (m_p1Left) {
        if (m_rocket->getPosition().x < -(m_rocket->getWidth())) {
            m_rocket->setPosition(kLogicalW, m_rocket->getPosition().y);
        }
        if (!m_p1FacingLeft) {
            m_rocket->setScale(-2.0f, 2);
            m_p1FacingLeft = true;
            m_rocket->setPosition(m_rocket->getPosition().x -
                                      (m_rocket->getWidth() * m_rocket->getScale().x),
                                  m_rocket->getPosition().y);
        }

        if (collision(*m_rocket, *m_robot)) {
            m_rocket->setPosition(m_rocket->getPosition().x + 40, m_rocket->getPosition().y);
        } else {
            p1movement.x -= m_speed;
        }
    }
    if (m_p1Right) {
        m_rocket->setScale(2.0f, 2);

        if (m_rocket->getPosition().x > kLogicalW) {
            m_rocket->setPosition(-(m_rocket->getWidth()), m_rocket->getPosition().y);
        }

        if (m_p1FacingLeft) {
            m_p1FacingLeft = false;
            m_rocket->setPosition(m_rocket->getPosition().x -
                                      (m_rocket->getWidth() * m_rocket->getScale().x),
                                  m_rocket->getPosition().y);
        }

        if (collision(*m_rocket, *m_robot)) {
            m_rocket->setPosition(m_rocket->getPosition().x - 40, m_rocket->getPosition().y);
        } else {
            p1movement.x += m_speed;
        }
    }
    if (m_p2Up) {
        if (m_robot->getPosition().y < -(m_robot->getHeight())) {
            m_robot->setPosition(m_robot->getPosition().x, kLogicalH - m_robot->getHeight());
        }

        if (collision(*m_rocket, *m_robot)) {
            m_robot->setPosition(m_robot->getPosition().x, m_robot->getPosition().y + 40);
        } else {
            p2movement.y -= m_speed;
        }
    }
    if (m_p2Down) {
        if (m_robot->getPosition().y > kLogicalH - m_robot->getHeight()) {
            m_robot->setPosition(m_robot->getPosition().x, -(m_robot->getHeight()));
        }

        if (collision(*m_rocket, *m_robot)) {
            m_robot->setPosition(m_robot->getPosition().x, m_robot->getPosition().y - 40);
        } else {
            p2movement.y += m_speed;
        }
    }
    if (m_p2Left) {
        if (m_robot->getPosition().x < -(m_robot->getWidth())) {
            m_robot->setPosition(kLogicalW, m_robot->getPosition().y);
        }
        m_robot->setScale(-1.0f, 1.0f);
        if (!m_p2FacingLeft) {
            m_p2FacingLeft = true;
            m_robot->setPosition(m_robot->getPosition().x -
                                     (m_robot->getWidth() * m_robot->getScale().x),
                                 m_robot->getPosition().y);
        }

        if (collision(*m_rocket, *m_robot)) {
            m_robot->setPosition(m_robot->getPosition().x + 40, m_robot->getPosition().y);
        } else {
            p2movement.x -= m_speed;
        }
    }
    if (m_p2Right) {
        m_robot->setScale(1.0f, 1.0f);
        if (m_robot->getPosition().x > kLogicalW) {
            m_robot->setPosition(-(m_robot->getWidth()), m_robot->getPosition().y);
        }
        if (m_p2FacingLeft) {
            m_p2FacingLeft = false;
            m_robot->setPosition(m_robot->getPosition().x -
                                     (m_robot->getWidth() * m_robot->getScale().x),
                                 m_robot->getPosition().y);
        }

        if (collision(*m_rocket, *m_robot)) {
            m_robot->setPosition(m_robot->getPosition().x - 40, m_robot->getPosition().y);
        } else {
            p2movement.x += m_speed;
        }
    }
    if (m_p2Attack) {
        sf::Rect<float> hit;
        if (!m_p2FacingLeft) {
            hit = sf::Rect<float>(
                m_robot->getPosition(),
                sf::Vector2f(200 + ((m_robot->getWidth()) * (m_robot->getScale().x)),
                             (m_robot->getHeight()) * (m_robot->getScale().y)));
        } else {
            hit = sf::Rect<float>(
                m_robot->getPosition(),
                sf::Vector2f(((m_robot->getWidth()) * (m_robot->getScale().x)) - 200,
                             (m_robot->getHeight()) * (m_robot->getScale().y)));
        }

        if (collision(hit, *m_rocket)) {
            metal.play();
            m_robotScore++;
            reset = true;
        } else {
            sword.play();
        }

        m_p2Attack = false;
    }
    if (m_p1Attack) {
        sf::Rect<float> hit;
        if (!m_p1FacingLeft) {
            hit = sf::Rect<float>(
                m_rocket->getPosition(),
                sf::Vector2f(200 + ((m_rocket->getWidth()) * (m_rocket->getScale().x)),
                             (m_rocket->getHeight()) * (m_rocket->getScale().y)));
        } else {
            hit = sf::Rect<float>(
                m_rocket->getPosition(),
                sf::Vector2f(((m_rocket->getWidth()) * (m_rocket->getScale().x)) - 200,
                             (m_rocket->getHeight()) * (m_rocket->getScale().y)));
        }

        if (collision(hit, *m_robot)) {
            m_rocketScore++;
            reset = true;
            p2hit.play();
        } else {
            laser.play();
        }

        m_p1Attack = false;
    }
    if (m_slowDownPressed) {
        m_speed = m_speed - 150.0f;
        if (m_speed < 0)
            m_speed = 1.0f;
        m_slowDownPressed = false;
        slow_down.play();
    }
    if (m_speedUpPressed) {
        m_speed = m_speed + 150.0f;
        m_speedUpPressed = false;
        speed_up.play();
    }

    if ((m_p1Up || m_p1Down || m_p1Left || m_p1Right) && !collision(*m_rocket, *m_robot)) {
        m_rocket->move(p1movement * deltaT.asSeconds());
        m_rocket->update(time);
    }

    if ((m_p2Up || m_p2Left || m_p2Down || m_p2Right) && !collision(*m_rocket, *m_robot)) {
        m_robot->move(p2movement * deltaT.asSeconds());
        m_robot->update(time);
    }
    for (int x = 0; x < static_cast<int>(m_arena.size()); x++) {
        m_arena[x]->update(time);
    }

    if (reset) {
        m_rocket->setScale(2.0f, 2);
        m_robot->setScale(-1.0f, 1.0f);
        m_robot->setPosition(kLogicalW - (m_rocket->getWidth() + 150), 400);
        m_rocket->setPosition(m_rocket->getWidth() + 20, 400);
        m_p1FacingLeft = false;
        m_p2FacingLeft = true;
        m_inCooldown = true;
    }

    m_rocketScoreText.setString("Rocket Score: " + std::to_string(m_rocketScore));
    m_robotScoreText.setString("Robot Score: " + std::to_string(m_robotScore));
    m_robotScoreText.setPosition(kLogicalW - m_robotScoreText.getGlobalBounds().width - 20, 0);
}

// ── render ────────────────────────────────────────────────────────────────────

void Game::render() {
    // CLIENT: temporarily shift sprites to interpolated positions for drawing.
    // Positions are saved and restored so update()/collision use authoritative coords.
    sf::Vector2f p1Saved, p2Saved;
    bool didShift = false;

    // LCOV_EXCL_START — CLIENT interpolation; only runs in networked CLIENT mode
    if (m_networkMode == NetworkMode::CLIENT && m_networkManager &&
        !m_networkManager->stateBuf().empty()) {
        p1Saved = m_rocket->getPosition();
        p2Saved = m_robot->getPosition();
        didShift = true;

        const auto& buf = m_networkManager->stateBuf();
        constexpr float kInterpDelay = 0.04f; // 40 ms behind
        sf::Time target = m_networkManager->elapsed() - sf::seconds(kInterpDelay);

        sf::Vector2f rp1, rp2;
        if (buf.size() >= 2) {
            std::size_t ai = 0;
            for (std::size_t i = 0; i + 1 < buf.size(); ++i) {
                if (buf[i].arrival <= target)
                    ai = i;
            }
            std::size_t bi = std::min(ai + 1, buf.size() - 1);
            const auto& sa = buf[ai];
            const auto& sb = buf[bi];
            float span = (sb.arrival - sa.arrival).asSeconds();
            float t = (span > 0.f) ? (target - sa.arrival).asSeconds() / span : 1.f;
            rp1 = lerpPos({sa.state.p1_x, sa.state.p1_y}, {sb.state.p1_x, sb.state.p1_y}, t);
            rp2 = lerpPos({sa.state.p2_x, sa.state.p2_y}, {sb.state.p2_x, sb.state.p2_y}, t);
        } else {
            rp1 = {buf[0].state.p1_x, buf[0].state.p1_y};
            rp2 = {buf[0].state.p2_x, buf[0].state.p2_y};
        }
        m_rocket->setPosition(rp1.x, rp1.y);
        m_robot->setPosition(rp2.x, rp2.y);
    }
    // LCOV_EXCL_STOP

    m_window.clear();
    for (std::size_t x = 0; x < m_arena.size(); ++x) {
        m_arena[x]->draw(m_window);
    }
    m_window.draw(timer);
    m_window.draw(m_rocketScoreText);
    m_window.draw(m_robotScoreText);
    m_rocket->draw(m_window);
    m_robot->draw(m_window);
    if (m_inCooldown) {
        m_window.draw(pause_text);
        m_window.draw(info);
    }
    if (m_paused) {
        sf::RectangleShape overlay(sf::Vector2f(kLogicalW, kLogicalH));
        overlay.setFillColor(sf::Color(0, 0, 0, 140));
        m_window.draw(overlay);
        sf::Text txt("PAUSED\nEsc: resume   Q: quit to menu", font, 60);
        txt.setFillColor(sf::Color::White);
        txt.setStyle(sf::Text::Bold);
        auto bounds = txt.getLocalBounds();
        txt.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        txt.setPosition(kLogicalW / 2.f, kLogicalH / 2.f);
        m_window.draw(txt);
    }
    // NOTE: m_window.display() is called by run() so screenshots can be
    // captured between draw and swap.

    // Restore authoritative positions (symmetric with the save above).
    // LCOV_EXCL_START — only reachable when CLIENT interpolation block ran
    if (didShift) {
        m_rocket->setPosition(p1Saved.x, p1Saved.y);
        m_robot->setPosition(p2Saved.x, p2Saved.y);
    }
    // LCOV_EXCL_STOP
}

// ── collision ─────────────────────────────────────────────────────────────────

bool Game::collision(const GameObject& a, const GameObject& b) {
    return objectBounds(a).intersects(objectBounds(b));
}

bool Game::collision(sf::Rect<float> a, const GameObject& b) {
    return a.intersects(objectBounds(b));
}

// ── handleNetworkCommunication ────────────────────────────────────────────────

void Game::handleNetworkCommunication(sf::Time deltaT) {
    if (!m_networkManager)
        return;

    if (m_networkMode == NetworkMode::HOST && m_networkManager->isConnected()) {
        m_networkManager->poll();

        // Consume all queued inputs; keep only the most recent.
        PlayerInput clientInput;
        PlayerInput tmp;
        bool got = false;
        while (m_networkManager->nextInput(tmp)) {
            clientInput = tmp;
            got = true;
        }
        if (got) {
            applyPlayerInput(clientInput);
        }

        // Fixed-rate state send (~25 Hz).
        m_stateAccum += deltaT.asSeconds();
        if (m_stateAccum >= kStateSendInterval) {
            m_stateAccum -= kStateSendInterval;
            m_networkManager->sendGameState(captureGameState());
        }
    } else if (m_networkMode == NetworkMode::CLIENT && m_networkManager->isConnected()) {
        // Send local input every frame.
        PlayerInput myInput;
        myInput.up = m_p2Up;
        myInput.down = m_p2Down;
        myInput.left = m_p2Left;
        myInput.right = m_p2Right;
        myInput.attack = m_p2Attack;
        m_networkManager->sendInput(myInput);

        // Drain incoming state packets.
        m_networkManager->poll();

        // Apply the latest snapshot — once per iteration, before the step-drain.
        if (!m_networkManager->stateBuf().empty()) {
            m_latestState = m_networkManager->stateBuf().back().state;
            applyNetworkState(m_latestState);
        }
    }

    // Peer-loss check (runs after poll/send on either side).
    if (!m_peerLeft && m_networkManager->peerLost()) {
        m_peerLeft = true;
        m_window.close();
    }
}

// ── snapshot / captureGameState / applyNetworkState ───────────────────────────

GameState Game::snapshot() const { return captureGameState(); }

GameState Game::captureGameState() const {
    GameState state;
    state.p1_x = m_rocket->getPosition().x;
    state.p1_y = m_rocket->getPosition().y;
    state.p2_x = m_robot->getPosition().x;
    state.p2_y = m_robot->getPosition().y;
    state.p1_score = m_rocketScore;
    state.p2_score = m_robotScore;
    state.p1_left = m_p1FacingLeft;
    state.p2_left = m_p2FacingLeft;
    state.p1_scale_x = m_rocket->getScale().x;
    state.p1_scale_y = m_rocket->getScale().y;
    state.p2_scale_x = m_robot->getScale().x;
    state.p2_scale_y = m_robot->getScale().y;
    state.wait = m_inCooldown;
    return state;
}

void Game::applyNetworkState(const GameState& state) {
    m_rocket->setPosition(state.p1_x, state.p1_y);
    m_robot->setPosition(state.p2_x, state.p2_y);
    m_rocketScore = state.p1_score;
    m_robotScore = state.p2_score;
    m_p1FacingLeft = state.p1_left;
    m_p2FacingLeft = state.p2_left;
    m_rocket->setScale(state.p1_scale_x, state.p1_scale_y);
    m_robot->setScale(state.p2_scale_x, state.p2_scale_y);
    m_inCooldown = state.wait;
}
