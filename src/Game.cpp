//
// Created by bswenson3 on 11/9/16.
//

#include "Game.h"
#include "asset_load.h"
#include "geometry.h"
#include "resource_path.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>

// ── helpers ──────────────────────────────────────────────────────────────────

// LCOV_EXCL_START — screenshot requires a live RenderWindow; not testable headless
static void captureScreenshot(const sf::RenderWindow& w, const std::string& path)
{
    sf::Texture t;
    t.create(w.getSize().x, w.getSize().y);
    t.update(w);
    t.copyToImage().saveToFile(path);
}
// LCOV_EXCL_STOP

// ── constructors ──────────────────────────────────────────────────────────────

Game::Game() : Game(NetworkMode::LOCAL, nullptr) {}

Game::Game(NetworkMode mode, std::shared_ptr<NetworkManager> netManager)
    : m_window(sf::VideoMode(1920, 1080),
               mode == NetworkMode::LOCAL ? "Dungeon Game"
               : mode == NetworkMode::HOST ? "Dungeon Game - Host"
                                           : "Dungeon Game - Client"),
      m_networkMode(mode),
      m_networkManager(netManager)
{
    loadOrThrow(background, resource_path + "background.wav");

    loadOrThrow(*m_player, resource_path + "rocket.png");
    m_player->setScale(2.0f);
    m_player->setPosition(m_player->getWidth(), 400);

    loadOrThrow(*player2, resource_path + "robot.png");
    player2->setScale(-1.0f, 1.0f);
    player2->setPosition(m_window.getSize().x - (m_player->getWidth() + 150), 400);

    stuff.push_back(std::make_unique<RegularGameObject>());
    loadOrThrow(*stuff[0], resource_path + "brick.png");
    stuff[0]->setScale(2.0f);

    {
        auto fire = std::make_unique<AnimatedGameObject>(216, 216, 5, 3, 10, 0);
        loadOrThrow(*fire, resource_path + "fire.png");
        fire->setScale(2.0f);
        fire->setPosition(-15, 400);
        stuff.push_back(std::move(fire));
    }
    {
        auto fire2 = std::make_unique<AnimatedGameObject>(216, 216, 5, 3, 10, 0);
        loadOrThrow(*fire2, resource_path + "fire.png");
        fire2->setScale(2.0f);
        fire2->setPosition((1920 / 2) - 5, 400);
        stuff.push_back(std::move(fire2));
    }
    {
        auto fire3 = std::make_unique<AnimatedGameObject>(216, 216, 5, 3, 10, 0);
        loadOrThrow(*fire3, resource_path + "fire.png");
        fire3->setScale(2.0f);
        fire3->setPosition(1820, 400);
        stuff.push_back(std::move(fire3));
    }

    loadOrThrow(font,  resource_path + "oswald.ttf");
    loadOrThrow(tfont, resource_path + "timer.ttf");
    loadOrThrow(block, resource_path + "Blockt.ttf");

    pause_text = sf::Text("Cooldown", block, 400);
    pause_text.setPosition(m_window.getSize().x / 2 - 850, 100);
    pause_text.setFillColor(sf::Color::Black);

    info = sf::Text("a short intermission", font, 50);
    info.setPosition(pause_text.getPosition().x + 150, pause_text.getPosition().y + 500);
    info.setFillColor(sf::Color::Black);

    text  = sf::Text("Rocket Score: 0", font, 40);
    text2 = sf::Text("Robot Score: 0", font, 40);
    timer = sf::Text("timer", tfont, 60);
    timer.setPosition((m_window.getSize().x / 2) - 60, 0);
    text.setPosition(10, 0);
    text2.setPosition(m_window.getSize().x - text2.getGlobalBounds().width, 0);
    text2.setFillColor(sf::Color::Green);
    timer.setFillColor(sf::Color::Black);
    text.setFillColor(sf::Color::Blue);

    loadOrThrow(sbuffer,     resource_path + "sword_miss.wav");
    sword.setBuffer(sbuffer);
    loadOrThrow(p2hitbuffer, resource_path + "skeleton.wav");
    p2hit.setBuffer(p2hitbuffer);
    loadOrThrow(laserbuffer, resource_path + "new_laser.wav");
    laser.setBuffer(laserbuffer);
    loadOrThrow(metalbuffer, resource_path + "metal_hit.wav");
    metal.setBuffer(metalbuffer);
    loadOrThrow(speedbuffer, resource_path + "SpeedUp.wav");
    speed_up.setBuffer(speedbuffer);
    loadOrThrow(slowbuffer,  resource_path + "SlowDown.wav");
    slow_down.setBuffer(slowbuffer);
    loadOrThrow(burnbuffer,  resource_path + "burn.wav");
    burn.setBuffer(burnbuffer);
    loadOrThrow(gongbuffer,  resource_path + "gong.wav");
    gong.setBuffer(gongbuffer);
    laser.setVolume(60);
}

// ── debug config ──────────────────────────────────────────────────────────────

void Game::setDebugConfig(const DebugConfig& cfg)
{
    m_debug = cfg;
    if (!cfg.replayPath.empty()) {
        m_replay    = loadReplay(cfg.replayPath);
        m_replayIdx = 0;
    }
    if (!cfg.replayPathP1.empty()) {
        m_replayP1    = loadReplay(cfg.replayPathP1);
        m_replayP1Idx = 0;
    }
}

// ── run ───────────────────────────────────────────────────────────────────────

std::tuple<int, int, float, int, bool, bool> Game::run()
{
    sf::Clock clock;
    background.play();
    background.setLoop(true);
    background.setVolume(40);
    gong.setVolume(50);

    float     apieceofcrap = 0.f;
    int       seconds      = 0;
    bool      p1win        = false;
    int       framesLeft   = m_debug.frames;
    const bool framesMode  = (framesLeft > 0);

    while (m_window.isOpen()) {
        // ── Timer display from step counter ──────────────────────────────────
        {
            int totalSecs  = static_cast<int>(gameSeconds());
            apieceofcrap   = static_cast<float>(totalSecs / 60);
            seconds        = totalSecs % 60;
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
        } else {
            m_accumulator += iterDt;
            while (m_accumulator >= kFixedDt) {
                simStep();
                m_accumulator -= kFixedDt;
            }
        }

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
    p1win = (p2points > points);
    return {p2points, points, apieceofcrap, seconds, p1win, m_peerLeft};
}

// ── simStep ───────────────────────────────────────────────────────────────────

void Game::simStep()
{
    // Apply replay input for P2 (before update so the flags are set when update() runs).
    if (!m_replay.empty()) {
        if (m_replayIdx < m_replay.size()) {
            applyPlayerInput(m_replay[m_replayIdx++]);
        } else {
            applyPlayerInput(PlayerInput{}); // EOF → all-false
        }
    }

    // Apply replay input for P1.
    if (!m_replayP1.empty()) {
        if (m_replayP1Idx < m_replayP1.size()) {
            applyInputToP1(m_up, m_left, m_down, m_right, right, m_replayP1[m_replayP1Idx++]);
        } else {
            PlayerInput empty{};
            applyInputToP1(m_up, m_left, m_down, m_right, right, empty);
        }
    }

    // Update or drain the cooldown wait.
    if (!wait) {
        update(sf::seconds(kFixedDt), m_animTime);
        temp_time = static_cast<float>(gameSeconds()) + 1.75f;
    } else if (gameSeconds() > static_cast<double>(temp_time)) {
        wait = false;
        gong.play();
    }

    // Animation accumulator (matches original pattern: reset-then-increment).
    if (m_animTime > .1f)
        m_animTime = 0;
    m_animTime += kFixedDt;

    // Timeout checks.
    if (gameSeconds() > static_cast<double>(p1_timeout))
        p1_time_passed = true;
    if (gameSeconds() > static_cast<double>(p2_timeout))
        p2_time_passed = true;

    // Hazard collision + scoring.
    for (int x = 1; x < static_cast<int>(stuff.size()); x++) {
        if (collision(*stuff[x], *m_player) && p1_time_passed) {
            burn.play();
            p2points--;
            p1_time_passed = false;
            p1_timeout     = static_cast<float>(gameSeconds()) + 2.2f;
        }
        if (collision(*stuff[x], *player2) && p2_time_passed) {
            burn.play();
            points--;
            p2_time_passed = false;
            p2_timeout     = static_cast<float>(gameSeconds()) + 2.2f;
        }
    }

    // Score clamps.
    if (points < 0)
        points = 0;
    if (p2points < 0)
        p2points = 0;

    ++m_steps;
}

// ── applyPlayerInput / captureIfDue ──────────────────────────────────────────

void Game::applyPlayerInput(const PlayerInput& in)
{
    applyInputTo(w, a, s, d, space, in);
}

void Game::captureIfDue()
{
    if (m_debug.screenshotEvery <= 0) return;
    if (m_nextShot == 0) m_nextShot = m_debug.screenshotEvery; // lazy init
    // Threshold (not modulo) so a multi-step accumulator drain that jumps past a
    // boundary still captures once; --frames mode (1 step/iter) lands exactly on
    // each multiple, keeping frame_60/120/... filenames stable for determinism.
    if (m_steps >= m_nextShot) {
        captureScreenshot(m_window,
                          m_debug.screenshotDir + "/frame_" + std::to_string(m_steps) + ".png");
        while (m_nextShot <= m_steps) m_nextShot += m_debug.screenshotEvery;
    }
}

// ── processEvents ─────────────────────────────────────────────────────────────

void Game::processEvents()
{
    sf::Event event;
    while (m_window.pollEvent(event)) {
        switch (event.type) {
        // LCOV_EXCL_START — keyboard events are not generated in harness/headless runs
        case sf::Event::KeyPressed:
            handlePlayerInput(event.key.code, true);
            break;
        case sf::Event::KeyReleased:
            handlePlayerInput(event.key.code, false);
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
void Game::handlePlayerInput(sf::Keyboard::Key key, bool isDown)
{
    // Gate all game-key bindings when the harness is active so that live
    // keyboard cannot perturb a --frames / --replay run.
    if (!m_debug.active()) {
        // Player 1 controls (rocket — numpad)
        if (m_networkMode == NetworkMode::LOCAL || m_networkMode == NetworkMode::HOST) {
            if (key == sf::Keyboard::Numpad4)
                m_left = isDown;
            if (key == sf::Keyboard::Numpad6)
                m_right = isDown;
            if (key == sf::Keyboard::Numpad8)
                m_up = isDown;
            if (key == sf::Keyboard::Numpad5)
                m_down = isDown;
            if (key == sf::Keyboard::Right)
                right = isDown;
        }

        // Player 2 controls (robot — WASD)
        if (m_networkMode == NetworkMode::LOCAL || m_networkMode == NetworkMode::CLIENT) {
            if (key == sf::Keyboard::W)
                w = isDown;
            if (key == sf::Keyboard::A)
                a = isDown;
            if (key == sf::Keyboard::S)
                s = isDown;
            if (key == sf::Keyboard::D)
                d = isDown;
            if (key == sf::Keyboard::Space)
                space = !isDown;
        }

        // Speed tweaks and wait-skip
        if (key == sf::Keyboard::O)
            o = !isDown;
        if (key == sf::Keyboard::P)
            p = !isDown;
        if (key == sf::Keyboard::K) {
            if (wait) {
                wait = false;
                gong.play();
            }
        }
    }

    // Always-active: window close and manual screenshot.
    if (key == sf::Keyboard::Escape)
        m_window.close();
    if (key == sf::Keyboard::F12 && isDown)
        captureScreenshot(m_window,
                          m_debug.screenshotDir + "/manual_" + std::to_string(m_steps) + ".png");
}
// LCOV_EXCL_STOP

// ── update ────────────────────────────────────────────────────────────────────

void Game::update(sf::Time deltaT, float time)
{
    bool reset = false;

    sf::Vector2f p1movement(0.0f, 0.0f);
    sf::Vector2f p2movement(0.0f, 0.0f);

    if (m_up) {
        if (m_player->getPosition().y < -(m_player->getHeight())) {
            m_player->setPosition(m_player->getPosition().x,
                                  (m_window.getSize().y) - m_player->getHeight());
        }

        if (collision(*m_player, *player2)) {
            m_player->setPosition(m_player->getPosition().x, m_player->getPosition().y + 40);
        } else {
            p1movement.y -= m_speed;
        }
    }

    if (m_down) {
        if (m_player->getPosition().y > (m_window.getSize().y) - (m_player->getHeight())) {
            m_player->setPosition(m_player->getPosition().x, -(m_player->getHeight()));
        }

        if (collision(*m_player, *player2)) {
            m_player->setPosition(m_player->getPosition().x, m_player->getPosition().y - 40);
        } else {
            p1movement.y += m_speed;
        }
    }
    if (m_left) {
        if (m_player->getPosition().x < -(m_player->getWidth())) {
            m_player->setPosition(m_window.getSize().x, m_player->getPosition().y);
        }
        if (!p1left) {
            m_player->setScale(-2.0f, 2);
            p1left = true;
            m_player->setPosition(
                m_player->getPosition().x -
                    (m_player->getWidth() * m_player->getScale().x),
                m_player->getPosition().y);
        }

        if (collision(*m_player, *player2)) {
            m_player->setPosition(m_player->getPosition().x + 40, m_player->getPosition().y);
        } else {
            p1movement.x -= m_speed;
        }
    }
    if (m_right) {
        m_player->setScale(2.0f, 2);

        if (m_player->getPosition().x > m_window.getSize().x) {
            m_player->setPosition(-(m_player->getWidth()), m_player->getPosition().y);
        }

        if (p1left) {
            p1left = false;
            m_player->setPosition(
                m_player->getPosition().x -
                    (m_player->getWidth() * m_player->getScale().x),
                m_player->getPosition().y);
        }

        if (collision(*m_player, *player2)) {
            m_player->setPosition(m_player->getPosition().x - 40, m_player->getPosition().y);
        } else {
            p1movement.x += m_speed;
        }
    }
    if (w) {
        if (player2->getPosition().y < -(player2->getHeight())) {
            player2->setPosition(player2->getPosition().x,
                                 (m_window.getSize().y) - player2->getHeight());
        }

        if (collision(*m_player, *player2)) {
            player2->setPosition(player2->getPosition().x, player2->getPosition().y + 40);
        } else {
            p2movement.y -= m_speed;
        }
    }
    if (s) {
        if (player2->getPosition().y > (m_window.getSize().y) - (player2->getHeight())) {
            player2->setPosition(player2->getPosition().x, -(player2->getHeight()));
        }

        if (collision(*m_player, *player2)) {
            player2->setPosition(player2->getPosition().x, player2->getPosition().y - 40);
        } else {
            p2movement.y += m_speed;
        }
    }
    if (a) {
        if (player2->getPosition().x < -(player2->getWidth())) {
            player2->setPosition(m_window.getSize().x, player2->getPosition().y);
        }
        player2->setScale(-1.0f, 1.0f);
        if (!p2left) {
            p2left = true;
            player2->setPosition(
                player2->getPosition().x -
                    (player2->getWidth() * player2->getScale().x),
                player2->getPosition().y);
        }

        if (collision(*m_player, *player2)) {
            player2->setPosition(player2->getPosition().x + 40, player2->getPosition().y);
        } else {
            p2movement.x -= m_speed;
        }
    }
    if (d) {
        player2->setScale(1.0f, 1.0f);
        if (player2->getPosition().x > m_window.getSize().x) {
            player2->setPosition(-(player2->getWidth()), player2->getPosition().y);
        }
        if (p2left) {
            p2left = false;
            player2->setPosition(
                player2->getPosition().x -
                    (player2->getWidth() * player2->getScale().x),
                player2->getPosition().y);
        }

        if (collision(*m_player, *player2)) {
            player2->setPosition(player2->getPosition().x - 40, player2->getPosition().y);
        } else {
            p2movement.x += m_speed;
        }
    }
    if (space) {
        sf::Rect<float> hit;
        if (!p2left) {
            hit = sf::Rect<float>(
                player2->getPosition(),
                sf::Vector2f(200 + ((player2->getWidth()) * (player2->getScale().x)),
                             (player2->getHeight()) * (player2->getScale().y)));
        } else {
            hit = sf::Rect<float>(
                player2->getPosition(),
                sf::Vector2f(((player2->getWidth()) * (player2->getScale().x)) - 200,
                             (player2->getHeight()) * (player2->getScale().y)));
        }

        if (collision(hit, *m_player)) {
            metal.play();
            points++;
            reset = true;
        } else {
            sword.play();
        }

        space = false;
    }
    if (right) {
        sf::Rect<float> hit;
        if (!p1left) {
            hit = sf::Rect<float>(
                m_player->getPosition(),
                sf::Vector2f(200 + ((m_player->getWidth()) * (m_player->getScale().x)),
                             (m_player->getHeight()) * (m_player->getScale().y)));
        } else {
            hit = sf::Rect<float>(
                m_player->getPosition(),
                sf::Vector2f(((m_player->getWidth()) * (m_player->getScale().x)) - 200,
                             (m_player->getHeight()) * (m_player->getScale().y)));
        }

        if (collision(hit, *player2)) {
            p2points++;
            reset = true;
            p2hit.play();
        } else {
            laser.play();
        }

        right = false;
    }
    if (o) {
        m_speed = m_speed - 150.0f;
        if (m_speed < 0)
            m_speed = 1.0f;
        o = false;
        slow_down.play();
    }
    if (p) {
        m_speed = m_speed + 150.0f;
        p       = false;
        speed_up.play();
    }

    if ((m_up || m_down || m_left || m_right) && !collision(*m_player, *player2)) {
        m_player->move(p1movement * deltaT.asSeconds());
        m_player->update(time);
    }

    if ((w || a || s || d) && !collision(*m_player, *player2)) {
        player2->move(p2movement * deltaT.asSeconds());
        player2->update(time);
    }
    for (int x = 0; x < static_cast<int>(stuff.size()); x++) {
        stuff[x]->update(time);
    }

    if (reset) {
        m_player->setScale(2.0f, 2);
        player2->setScale(-1.0f, 1.0f);
        player2->setPosition(m_window.getSize().x - (m_player->getWidth() + 150), 400);
        m_player->setPosition(m_player->getWidth() + 20, 400);
        p1left = false;
        p2left = true;
        wait   = true;
    }

    text.setString("Rocket Score: " + std::to_string(p2points));
    text2.setString("Robot Score: " + std::to_string(points));
    text2.setPosition(m_window.getSize().x - text2.getGlobalBounds().width - 20, 0);
}

// ── render ────────────────────────────────────────────────────────────────────

void Game::render()
{
    // CLIENT: temporarily shift sprites to interpolated positions for drawing.
    // Positions are saved and restored so update()/collision use authoritative coords.
    sf::Vector2f p1Saved, p2Saved;
    bool         didShift = false;

    // LCOV_EXCL_START — CLIENT interpolation; only runs in networked CLIENT mode
    if (m_networkMode == NetworkMode::CLIENT && m_networkManager &&
        !m_networkManager->stateBuf().empty()) {
        p1Saved  = m_player->getPosition();
        p2Saved  = player2->getPosition();
        didShift = true;

        const auto& buf          = m_networkManager->stateBuf();
        constexpr float kInterpDelay = 0.04f; // 40 ms behind
        sf::Time        target   = m_networkManager->elapsed() - sf::seconds(kInterpDelay);

        sf::Vector2f rp1, rp2;
        if (buf.size() >= 2) {
            std::size_t ai = 0;
            for (std::size_t i = 0; i + 1 < buf.size(); ++i) {
                if (buf[i].arrival <= target)
                    ai = i;
            }
            std::size_t bi  = std::min(ai + 1, buf.size() - 1);
            const auto& sa  = buf[ai];
            const auto& sb  = buf[bi];
            float       span = (sb.arrival - sa.arrival).asSeconds();
            float       t =
                (span > 0.f) ? (target - sa.arrival).asSeconds() / span : 1.f;
            rp1 = lerpPos({sa.state.p1_x, sa.state.p1_y}, {sb.state.p1_x, sb.state.p1_y}, t);
            rp2 = lerpPos({sa.state.p2_x, sa.state.p2_y}, {sb.state.p2_x, sb.state.p2_y}, t);
        } else {
            rp1 = {buf[0].state.p1_x, buf[0].state.p1_y};
            rp2 = {buf[0].state.p2_x, buf[0].state.p2_y};
        }
        m_player->setPosition(rp1.x, rp1.y);
        player2->setPosition(rp2.x, rp2.y);
    }
    // LCOV_EXCL_STOP

    m_window.clear();
    for (std::size_t x = 0; x < stuff.size(); ++x) {
        stuff[x]->draw(m_window);
    }
    m_window.draw(timer);
    m_window.draw(text);
    m_window.draw(text2);
    m_player->draw(m_window);
    player2->draw(m_window);
    if (wait) {
        m_window.draw(pause_text);
        m_window.draw(info);
    }
    // NOTE: m_window.display() is called by run() so screenshots can be
    // captured between draw and swap.

    // Restore authoritative positions (symmetric with the save above).
    // LCOV_EXCL_START — only reachable when CLIENT interpolation block ran
    if (didShift) {
        m_player->setPosition(p1Saved.x, p1Saved.y);
        player2->setPosition(p2Saved.x, p2Saved.y);
    }
    // LCOV_EXCL_STOP
}

// ── collision ─────────────────────────────────────────────────────────────────

bool Game::collision(const GameObject& a, const GameObject& b)
{
    return objectBounds(a).intersects(objectBounds(b));
}

bool Game::collision(sf::Rect<float> a, const GameObject& b)
{
    return a.intersects(objectBounds(b));
}

// ── handleNetworkCommunication ────────────────────────────────────────────────

void Game::handleNetworkCommunication(sf::Time deltaT)
{
    if (!m_networkManager)
        return;

    if (m_networkMode == NetworkMode::HOST && m_networkManager->isConnected()) {
        m_networkManager->poll();

        // Consume all queued inputs; keep only the most recent.
        PlayerInput clientInput;
        PlayerInput tmp;
        bool        got = false;
        while (m_networkManager->nextInput(tmp)) {
            clientInput = tmp;
            got         = true;
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
        myInput.up     = w;
        myInput.down   = s;
        myInput.left   = a;
        myInput.right  = d;
        myInput.attack = space;
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

GameState Game::captureGameState() const
{
    GameState state;
    state.p1_x      = m_player->getPosition().x;
    state.p1_y      = m_player->getPosition().y;
    state.p2_x      = player2->getPosition().x;
    state.p2_y      = player2->getPosition().y;
    state.p1_score  = p2points;
    state.p2_score  = points;
    state.p1_left   = p1left;
    state.p2_left   = p2left;
    state.p1_scale_x = m_player->getScale().x;
    state.p1_scale_y = m_player->getScale().y;
    state.p2_scale_x = player2->getScale().x;
    state.p2_scale_y = player2->getScale().y;
    state.wait       = wait;
    return state;
}

void Game::applyNetworkState(const GameState& state)
{
    m_player->setPosition(state.p1_x, state.p1_y);
    player2->setPosition(state.p2_x, state.p2_y);
    p2points = state.p1_score;
    points   = state.p2_score;
    p1left   = state.p1_left;
    p2left   = state.p2_left;
    m_player->setScale(state.p1_scale_x, state.p1_scale_y);
    player2->setScale(state.p2_scale_x, state.p2_scale_y);
    wait = state.wait;
}
