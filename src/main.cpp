#include "AnimatedGameObject.h"
#include "Game.h"
#include "NetworkManager.h"
#include "ai.h"
#include "asset_load.h"
#include "debug.h"
#include "key_bindings.h"
#include "letterbox.h"
#include "resource_path.h"
#include "rng.h"
#include <SFML/Graphics.hpp>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>

// Logical canvas size for the menu / endscreen windows.
static constexpr float kMenuW = 1024.f;
static constexpr float kMenuH = 576.f;

enum MenuState { MAIN_MENU, HOST_WAITING, JOIN_INPUT, AI_DIFFICULTY, READY_TO_START };

struct MenuResult {
    NetworkMode mode = NetworkMode::LOCAL;
    std::shared_ptr<NetworkManager> netManager;
    bool quit = false;
    AiDifficulty ai = AiDifficulty::None;
};

// ── showEndscreen ─────────────────────────────────────────────────────────────
// Show the post-game results screen.  Returns true if the player wants to play
// again immediately (LOCAL only; online always returns false so the caller goes
// back through the menu to re-establish the connection).

static bool showEndscreen(int highscore, int p1score, int p2score, float minutes, int seconds,
                          bool peerLeft, NetworkMode mode) {
    char gTime[10];
    std::snprintf(gTime, sizeof(gTime), "%02.0f:%02d", minutes, seconds);
    const std::string finalTime = gTime;

    sf::Font tfont;
    sf::Font pfont;
    sf::Text p1scoret;
    sf::Text p2scoret;
    sf::Text finalTimet;
    sf::Text highscoret;

    RegularGameObject wall = RegularGameObject();
    loadOrThrow(wall, resource_path + "david_background.png");
    wall.setScale(.7f);

    loadOrThrow(pfont, resource_path + "Joyful_Theatre.otf");
    loadOrThrow(tfont, resource_path + "timer.ttf");

    sf::RenderWindow endscreen(sf::VideoMode(1024, 576), "end game", sf::Style::Default);
    endscreen.setView(makeLetterboxView({kMenuW, kMenuH}, endscreen.getSize()));

    sf::Text peerDisconnectedText("", pfont, 36);
    if (peerLeft) {
        peerDisconnectedText.setString("Peer disconnected");
        peerDisconnectedText.setFillColor(sf::Color::Red);
        peerDisconnectedText.setStyle(sf::Text::Bold);
        peerDisconnectedText.setOrigin(peerDisconnectedText.getLocalBounds().width / 2.f, 0);
        peerDisconnectedText.setPosition(kMenuW / 2.f, 460.f);
    }

    sf::Music background;
    loadOrThrow(background, resource_path + "m_start_background.wav");

    p1scoret = sf::Text(std::to_string(p1score), pfont, 50);
    p1scoret.setFillColor(sf::Color::Blue);
    p2scoret = sf::Text(std::to_string(p2score), pfont, 50);
    p2scoret.setFillColor(sf::Color::Green);
    finalTimet = sf::Text(finalTime, tfont, 50);
    finalTimet.setFillColor(sf::Color::Blue);
    highscoret = sf::Text(std::to_string(highscore), pfont, 70);

    p1scoret.setPosition(5, kMenuH / 2);
    p2scoret.setPosition(kMenuW - p1scoret.getGlobalBounds().width - 5, kMenuH / 2);
    finalTimet.setPosition(kMenuW / 2 - finalTimet.getGlobalBounds().width + 20, -10);
    highscoret.setPosition(kMenuW / 2 - highscoret.getGlobalBounds().width, 25);

    if (p1score == highscore)
        highscoret.setFillColor(sf::Color::Blue);
    if (p2score == highscore)
        highscoret.setFillColor(sf::Color::Green);

    background.play();
    background.setVolume(60);

    sf::SoundBuffer down_buffer;
    sf::SoundBuffer up_buffer;
    loadOrThrow(down_buffer, resource_path + "ButtonOn.wav");
    loadOrThrow(up_buffer, resource_path + "ButtonOff.wav");

    sf::Sound press;
    sf::Sound up;
    press.setBuffer(down_buffer);
    up.setBuffer(up_buffer);

    bool start_updated = false;
    bool quit_updated = false;

    auto m_start = std::make_unique<AnimatedGameObject>(1331, 300, 2, 1, 2, 0);
    loadOrThrow(*m_start, resource_path + "start.png");
    m_start->setOrigin();
    m_start->setPosition(kMenuW / 2, kMenuH / 3);
    m_start->setScale(.5f);

    auto m_quit = std::make_unique<AnimatedGameObject>(1332, 300, 2, 1, 2, 0);
    loadOrThrow(*m_quit, resource_path + "quit.png");
    m_quit->setOrigin();
    m_quit->setPosition(kMenuW / 2, 2 * (kMenuH / 3));
    m_quit->setScale(.5f);

    bool playAgain = false;

    while (endscreen.isOpen()) {
        sf::Rect<float> srect(sf::Vector2f(m_start->getPosition().x - (m_start->getWidth() / 4),
                                           m_start->getPosition().y - (m_start->getHeight() / 4)),
                              sf::Vector2f(m_start->getWidth() * m_start->getScale().x,
                                           m_start->getHeight() * m_start->getScale().y));
        sf::Rect<float> qrect(sf::Vector2f(m_quit->getPosition().x - (m_quit->getWidth() / 4),
                                           m_quit->getPosition().y - (m_quit->getHeight() / 4)),
                              sf::Vector2f(m_quit->getWidth() * m_quit->getScale().x,
                                           m_quit->getHeight() * m_quit->getScale().y));

        sf::Vector2f mousePos = endscreen.mapPixelToCoords(sf::Mouse::getPosition(endscreen));

        if (srect.contains(mousePos) && !start_updated) {
            m_start->update(0.0f);
            start_updated = true;
            press.play();
        } else if (!srect.contains(mousePos) && start_updated) {
            start_updated = false;
            m_start->update(0.0f);
            up.play();
        }

        if (qrect.contains(mousePos) && !quit_updated) {
            m_quit->update(0.0f);
            quit_updated = true;
            press.play();
        } else if (!qrect.contains(mousePos) && quit_updated) {
            quit_updated = false;
            m_quit->update(0.0f);
            up.play();
        }

        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && srect.contains(mousePos)) {
            background.stop();
            endscreen.close();
            // Play again only for LOCAL; online must go back through menu.
            playAgain = (mode == NetworkMode::LOCAL);
        }
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && qrect.contains(mousePos)) {
            background.stop();
            endscreen.close();
        }

        sf::Event event;
        while (endscreen.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                background.stop();
                endscreen.close();
            }
            if (event.type == sf::Event::Resized) {
                endscreen.setView(
                    makeLetterboxView({kMenuW, kMenuH}, {event.size.width, event.size.height}));
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Y) {
                    background.stop();
                    endscreen.close();
                    playAgain = (mode == NetworkMode::LOCAL);
                }
                if (event.key.code == sf::Keyboard::N) {
                    background.stop();
                    endscreen.close();
                }
            }
        }

        endscreen.clear();
        wall.draw(endscreen);
        endscreen.draw(p1scoret);
        endscreen.draw(p2scoret);
        endscreen.draw(finalTimet);
        endscreen.draw(highscoret);
        m_start->draw(endscreen);
        m_quit->draw(endscreen);
        if (peerLeft)
            endscreen.draw(peerDisconnectedText);
        endscreen.display();
    }

    return playAgain;
}

// ── showMenu ──────────────────────────────────────────────────────────────────
// Runs the start screen menu loop.  Returns the selected game mode, network
// manager, and a quit flag (true if the user closed the window without starting).

static MenuResult showMenu() {
    MenuState menuState = MAIN_MENU;
    NetworkMode mode = NetworkMode::LOCAL;
    std::shared_ptr<NetworkManager> netManager = nullptr;
    std::string ipInput = "127.0.0.1";
    std::string statusMessage;
    std::string hostIpAddress;

    RegularGameObject david = RegularGameObject();
    loadOrThrow(david, resource_path + "david_background.png");

    sf::Music background;
    loadOrThrow(background, resource_path + "m_start_background.wav");
    background.play();
    background.setVolume(60);
    background.setLoop(true);

    sf::SoundBuffer down_buffer;
    sf::SoundBuffer up_buffer;
    loadOrThrow(down_buffer, resource_path + "ButtonOn.wav");
    loadOrThrow(up_buffer, resource_path + "ButtonOff.wav");

    sf::Sound press;
    sf::Sound up;
    press.setBuffer(down_buffer);
    up.setBuffer(up_buffer);

    sf::Font font;
    loadOrThrow(font, resource_path + "oswald.ttf");

    bool start_updated = false;
    bool quit_updated = false;
    bool host_updated = false;
    bool join_updated = false;
    bool back_updated = false;
    bool one_player_updated = false;
    bool easy_updated = false;
    bool medium_updated = false;
    bool hard_updated = false;

    sf::RenderWindow startscreen(sf::VideoMode(1024, 576), "Dungeon Game", sf::Style::Default);
    startscreen.setView(makeLetterboxView({kMenuW, kMenuH}, startscreen.getSize()));

    auto m_start = std::make_unique<AnimatedGameObject>(1331, 300, 2, 1, 2, 0);
    loadOrThrow(*m_start, resource_path + "start.png");
    m_start->setOrigin();
    m_start->setPosition(kMenuW / 2, 200);
    m_start->setScale(.4f);

    auto m_host = std::make_unique<AnimatedGameObject>(1331, 300, 2, 1, 2, 0);
    loadOrThrow(*m_host, resource_path + "start.png");
    m_host->setOrigin();
    m_host->setPosition(kMenuW / 2, 320);
    m_host->setScale(.4f);

    auto m_join = std::make_unique<AnimatedGameObject>(1331, 300, 2, 1, 2, 0);
    loadOrThrow(*m_join, resource_path + "start.png");
    m_join->setOrigin();
    m_join->setPosition(kMenuW / 2, 440);
    m_join->setScale(.4f);

    auto m_quit = std::make_unique<AnimatedGameObject>(1332, 300, 2, 1, 2, 0);
    loadOrThrow(*m_quit, resource_path + "quit.png");
    m_quit->setOrigin();
    m_quit->setPosition(kMenuW / 2, 520);
    m_quit->setScale(.3f);

    auto m_back = std::make_unique<AnimatedGameObject>(1332, 300, 2, 1, 2, 0);
    loadOrThrow(*m_back, resource_path + "quit.png");
    m_back->setOrigin();
    m_back->setPosition(100, 50);
    m_back->setScale(.25f);

    // 1-player / AI-difficulty buttons
    auto m_one_player = std::make_unique<AnimatedGameObject>(1331, 300, 2, 1, 2, 0);
    loadOrThrow(*m_one_player, resource_path + "start.png");
    m_one_player->setOrigin();
    m_one_player->setPosition(kMenuW / 2, 120);
    m_one_player->setScale(.4f);

    auto m_easy = std::make_unique<AnimatedGameObject>(1331, 300, 2, 1, 2, 0);
    loadOrThrow(*m_easy, resource_path + "start.png");
    m_easy->setOrigin();
    m_easy->setPosition(kMenuW / 2, 180);
    m_easy->setScale(.4f);

    auto m_medium = std::make_unique<AnimatedGameObject>(1331, 300, 2, 1, 2, 0);
    loadOrThrow(*m_medium, resource_path + "start.png");
    m_medium->setOrigin();
    m_medium->setPosition(kMenuW / 2, 300);
    m_medium->setScale(.4f);

    auto m_hard = std::make_unique<AnimatedGameObject>(1331, 300, 2, 1, 2, 0);
    loadOrThrow(*m_hard, resource_path + "start.png");
    m_hard->setOrigin();
    m_hard->setPosition(kMenuW / 2, 420);
    m_hard->setScale(.4f);

    sf::Text localLabel("LOCAL", font, 40);
    localLabel.setFillColor(sf::Color::White);
    localLabel.setStyle(sf::Text::Bold);

    sf::Text hostLabel("HOST", font, 40);
    hostLabel.setFillColor(sf::Color::Green);
    hostLabel.setStyle(sf::Text::Bold);

    sf::Text joinLabel("JOIN", font, 40);
    joinLabel.setFillColor(sf::Color::Blue);
    joinLabel.setStyle(sf::Text::Bold);

    sf::Text backLabel("BACK", font, 20);
    backLabel.setFillColor(sf::Color::White);

    sf::Text onePlayerLabel("1 PLAYER", font, 36);
    onePlayerLabel.setFillColor(sf::Color::Yellow);
    onePlayerLabel.setStyle(sf::Text::Bold);

    sf::Text easyLabel("EASY", font, 38);
    easyLabel.setFillColor(sf::Color::Green);
    easyLabel.setStyle(sf::Text::Bold);

    sf::Text mediumLabel("MEDIUM", font, 38);
    mediumLabel.setFillColor(sf::Color::Yellow);
    mediumLabel.setStyle(sf::Text::Bold);

    sf::Text hardLabel("HARD", font, 38);
    hardLabel.setFillColor(sf::Color::Red);
    hardLabel.setStyle(sf::Text::Bold);

    MenuResult result;

    while (startscreen.isOpen()) {
        sf::Event event;
        while (startscreen.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                background.stop();
                startscreen.close();
                result.quit = true;
                return result;
            }
            if (event.type == sf::Event::Resized) {
                startscreen.setView(
                    makeLetterboxView({kMenuW, kMenuH}, {event.size.width, event.size.height}));
            }
            if (menuState == JOIN_INPUT && event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b' && !ipInput.empty()) {
                    ipInput.pop_back();
                } else if (event.text.unicode == '\r' || event.text.unicode == '\n') {
                    netManager = std::make_shared<NetworkManager>();
                    statusMessage = "Connecting...";
                    if (netManager->connectToHost(ipInput)) {
                        mode = NetworkMode::CLIENT;
                        menuState = READY_TO_START;
                        statusMessage = "Connected! Click START to begin";
                    } else {
                        statusMessage = "Failed to connect (invalid IP or host unreachable)";
                        netManager = nullptr;
                    }
                } else if (event.text.unicode < 128 && event.text.unicode != '\b' &&
                           ipInput.length() < 30) {
                    char c = static_cast<char>(event.text.unicode);
                    if ((c >= '0' && c <= '9') || c == '.' || c == ':')
                        ipInput += c;
                }
            }
        }

        sf::Vector2f mousePos = startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen));

        sf::Rect<float> startRect(m_start->getPosition().x - (m_start->getWidth() / 4),
                                  m_start->getPosition().y - (m_start->getHeight() / 4),
                                  m_start->getWidth() * m_start->getScale().x,
                                  m_start->getHeight() * m_start->getScale().y);
        sf::Rect<float> hostRect(m_host->getPosition().x - (m_host->getWidth() / 4),
                                 m_host->getPosition().y - (m_host->getHeight() / 4),
                                 m_host->getWidth() * m_host->getScale().x,
                                 m_host->getHeight() * m_host->getScale().y);
        sf::Rect<float> joinRect(m_join->getPosition().x - (m_join->getWidth() / 4),
                                 m_join->getPosition().y - (m_join->getHeight() / 4),
                                 m_join->getWidth() * m_join->getScale().x,
                                 m_join->getHeight() * m_join->getScale().y);
        sf::Rect<float> quitRect(m_quit->getPosition().x - (m_quit->getWidth() / 4),
                                 m_quit->getPosition().y - (m_quit->getHeight() / 4),
                                 m_quit->getWidth() * m_quit->getScale().x,
                                 m_quit->getHeight() * m_quit->getScale().y);
        sf::Rect<float> backRect(m_back->getPosition().x - (m_back->getWidth() / 4),
                                 m_back->getPosition().y - (m_back->getHeight() / 4),
                                 m_back->getWidth() * m_back->getScale().x,
                                 m_back->getHeight() * m_back->getScale().y);
        sf::Rect<float> onePlayerRect(
            m_one_player->getPosition().x - (m_one_player->getWidth() / 4),
            m_one_player->getPosition().y - (m_one_player->getHeight() / 4),
            m_one_player->getWidth() * m_one_player->getScale().x,
            m_one_player->getHeight() * m_one_player->getScale().y);
        sf::Rect<float> easyRect(m_easy->getPosition().x - (m_easy->getWidth() / 4),
                                 m_easy->getPosition().y - (m_easy->getHeight() / 4),
                                 m_easy->getWidth() * m_easy->getScale().x,
                                 m_easy->getHeight() * m_easy->getScale().y);
        sf::Rect<float> mediumRect(m_medium->getPosition().x - (m_medium->getWidth() / 4),
                                   m_medium->getPosition().y - (m_medium->getHeight() / 4),
                                   m_medium->getWidth() * m_medium->getScale().x,
                                   m_medium->getHeight() * m_medium->getScale().y);
        sf::Rect<float> hardRect(m_hard->getPosition().x - (m_hard->getWidth() / 4),
                                 m_hard->getPosition().y - (m_hard->getHeight() / 4),
                                 m_hard->getWidth() * m_hard->getScale().x,
                                 m_hard->getHeight() * m_hard->getScale().y);

        if (menuState == MAIN_MENU) {
            if (onePlayerRect.contains(mousePos) && !one_player_updated) {
                m_one_player->update(0.0f);
                one_player_updated = true;
                press.play();
            } else if (!onePlayerRect.contains(mousePos) && one_player_updated) {
                one_player_updated = false;
                m_one_player->update(0.0f);
                up.play();
            }
            if (startRect.contains(mousePos) && !start_updated) {
                m_start->update(0.0f);
                start_updated = true;
                press.play();
            } else if (!startRect.contains(mousePos) && start_updated) {
                start_updated = false;
                m_start->update(0.0f);
                up.play();
            }
            if (hostRect.contains(mousePos) && !host_updated) {
                m_host->update(0.0f);
                host_updated = true;
                press.play();
            } else if (!hostRect.contains(mousePos) && host_updated) {
                host_updated = false;
                m_host->update(0.0f);
                up.play();
            }
            if (joinRect.contains(mousePos) && !join_updated) {
                m_join->update(0.0f);
                join_updated = true;
                press.play();
            } else if (!joinRect.contains(mousePos) && join_updated) {
                join_updated = false;
                m_join->update(0.0f);
                up.play();
            }
            if (quitRect.contains(mousePos) && !quit_updated) {
                m_quit->update(0.0f);
                quit_updated = true;
                press.play();
            } else if (!quitRect.contains(mousePos) && quit_updated) {
                quit_updated = false;
                m_quit->update(0.0f);
                up.play();
            }
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                if (onePlayerRect.contains(mousePos)) {
                    menuState = AI_DIFFICULTY;
                    sf::sleep(sf::milliseconds(200));
                } else if (startRect.contains(mousePos)) {
                    mode = NetworkMode::LOCAL;
                    menuState = READY_TO_START;
                    sf::sleep(sf::milliseconds(200));
                } else if (hostRect.contains(mousePos)) {
                    netManager = std::make_shared<NetworkManager>();
                    if (netManager->startHost()) {
                        mode = NetworkMode::HOST;
                        menuState = HOST_WAITING;
                        hostIpAddress = sf::IpAddress::getLocalAddress().toString();
                        statusMessage = "Waiting for player 2...";
                    } else {
                        statusMessage = "Failed to start host!";
                        netManager = nullptr;
                    }
                    sf::sleep(sf::milliseconds(200));
                } else if (joinRect.contains(mousePos)) {
                    menuState = JOIN_INPUT;
                    statusMessage = "Enter host IP address";
                    sf::sleep(sf::milliseconds(200));
                } else if (quitRect.contains(mousePos)) {
                    background.stop();
                    startscreen.close();
                    result.quit = true;
                    return result;
                }
            }
        } else if (menuState == HOST_WAITING) {
            if (netManager && netManager->waitForClient(sf::milliseconds(100))) {
                menuState = READY_TO_START;
                statusMessage = "Player 2 connected! Click START to begin";
            }
            if (backRect.contains(mousePos) && !back_updated) {
                m_back->update(0.0f);
                back_updated = true;
                press.play();
            } else if (!backRect.contains(mousePos) && back_updated) {
                back_updated = false;
                m_back->update(0.0f);
                up.play();
            }
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && backRect.contains(mousePos)) {
                menuState = MAIN_MENU;
                netManager = nullptr;
                statusMessage = "";
                sf::sleep(sf::milliseconds(200));
            }
        } else if (menuState == JOIN_INPUT) {
            if (backRect.contains(mousePos) && !back_updated) {
                m_back->update(0.0f);
                back_updated = true;
                press.play();
            } else if (!backRect.contains(mousePos) && back_updated) {
                back_updated = false;
                m_back->update(0.0f);
                up.play();
            }
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && backRect.contains(mousePos)) {
                menuState = MAIN_MENU;
                ipInput = "127.0.0.1";
                statusMessage = "";
                sf::sleep(sf::milliseconds(200));
            }
        } else if (menuState == AI_DIFFICULTY) {
            if (easyRect.contains(mousePos) && !easy_updated) {
                m_easy->update(0.0f);
                easy_updated = true;
                press.play();
            } else if (!easyRect.contains(mousePos) && easy_updated) {
                easy_updated = false;
                m_easy->update(0.0f);
                up.play();
            }
            if (mediumRect.contains(mousePos) && !medium_updated) {
                m_medium->update(0.0f);
                medium_updated = true;
                press.play();
            } else if (!mediumRect.contains(mousePos) && medium_updated) {
                medium_updated = false;
                m_medium->update(0.0f);
                up.play();
            }
            if (hardRect.contains(mousePos) && !hard_updated) {
                m_hard->update(0.0f);
                hard_updated = true;
                press.play();
            } else if (!hardRect.contains(mousePos) && hard_updated) {
                hard_updated = false;
                m_hard->update(0.0f);
                up.play();
            }
            if (backRect.contains(mousePos) && !back_updated) {
                m_back->update(0.0f);
                back_updated = true;
                press.play();
            } else if (!backRect.contains(mousePos) && back_updated) {
                back_updated = false;
                m_back->update(0.0f);
                up.play();
            }
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                if (easyRect.contains(mousePos)) {
                    result.ai = AiDifficulty::Easy;
                    mode = NetworkMode::LOCAL;
                    menuState = READY_TO_START;
                    sf::sleep(sf::milliseconds(200));
                } else if (mediumRect.contains(mousePos)) {
                    result.ai = AiDifficulty::Medium;
                    mode = NetworkMode::LOCAL;
                    menuState = READY_TO_START;
                    sf::sleep(sf::milliseconds(200));
                } else if (hardRect.contains(mousePos)) {
                    result.ai = AiDifficulty::Hard;
                    mode = NetworkMode::LOCAL;
                    menuState = READY_TO_START;
                    sf::sleep(sf::milliseconds(200));
                } else if (backRect.contains(mousePos)) {
                    menuState = MAIN_MENU;
                    sf::sleep(sf::milliseconds(200));
                }
            }
        } else if (menuState == READY_TO_START) {
            if (startRect.contains(mousePos) && !start_updated) {
                m_start->update(0.0f);
                start_updated = true;
                press.play();
            } else if (!startRect.contains(mousePos) && start_updated) {
                start_updated = false;
                m_start->update(0.0f);
                up.play();
            }
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && startRect.contains(mousePos)) {
                background.stop();
                startscreen.close();
                result.mode = mode;
                result.netManager = netManager;
                return result;
            }
        }

        startscreen.clear();
        david.draw(startscreen);

        if (menuState == MAIN_MENU) {
            m_one_player->draw(startscreen);
            onePlayerLabel.setOrigin(onePlayerLabel.getGlobalBounds().width / 2,
                                     onePlayerLabel.getGlobalBounds().height / 2);
            onePlayerLabel.setPosition(m_one_player->getPosition().x,
                                       m_one_player->getPosition().y);
            startscreen.draw(onePlayerLabel);

            m_start->draw(startscreen);
            m_host->draw(startscreen);
            m_join->draw(startscreen);
            m_quit->draw(startscreen);

            localLabel.setString("2 PLAYER");
            localLabel.setOrigin(localLabel.getGlobalBounds().width / 2,
                                 localLabel.getGlobalBounds().height / 2);
            localLabel.setPosition(m_start->getPosition().x, m_start->getPosition().y);
            startscreen.draw(localLabel);

            hostLabel.setOrigin(hostLabel.getGlobalBounds().width / 2,
                                hostLabel.getGlobalBounds().height / 2);
            hostLabel.setPosition(m_host->getPosition().x, m_host->getPosition().y);
            startscreen.draw(hostLabel);

            joinLabel.setOrigin(joinLabel.getGlobalBounds().width / 2,
                                joinLabel.getGlobalBounds().height / 2);
            joinLabel.setPosition(m_join->getPosition().x, m_join->getPosition().y);
            startscreen.draw(joinLabel);
        } else if (menuState == HOST_WAITING) {
            m_back->draw(startscreen);
            backLabel.setOrigin(backLabel.getGlobalBounds().width / 2,
                                backLabel.getGlobalBounds().height / 2);
            backLabel.setPosition(m_back->getPosition().x, m_back->getPosition().y);
            startscreen.draw(backLabel);

            sf::Text title("HOSTING GAME", font, 50);
            title.setFillColor(sf::Color::Green);
            title.setStyle(sf::Text::Bold);
            title.setOrigin(title.getGlobalBounds().width / 2, 0);
            title.setPosition(kMenuW / 2, 150);
            startscreen.draw(title);

            sf::Text ipText("Your IP: " + hostIpAddress, font, 35);
            ipText.setFillColor(sf::Color::White);
            ipText.setOrigin(ipText.getGlobalBounds().width / 2, 0);
            ipText.setPosition(kMenuW / 2, 250);
            startscreen.draw(ipText);

            sf::Text statusText(statusMessage, font, 30);
            statusText.setFillColor(sf::Color(200, 200, 200));
            statusText.setOrigin(statusText.getGlobalBounds().width / 2, 0);
            statusText.setPosition(kMenuW / 2, 350);
            startscreen.draw(statusText);
        } else if (menuState == JOIN_INPUT) {
            m_back->draw(startscreen);
            backLabel.setOrigin(backLabel.getGlobalBounds().width / 2,
                                backLabel.getGlobalBounds().height / 2);
            backLabel.setPosition(m_back->getPosition().x, m_back->getPosition().y);
            startscreen.draw(backLabel);

            sf::Text title("JOIN GAME", font, 50);
            title.setFillColor(sf::Color::Blue);
            title.setStyle(sf::Text::Bold);
            title.setOrigin(title.getGlobalBounds().width / 2, 0);
            title.setPosition(kMenuW / 2, 150);
            startscreen.draw(title);

            sf::Text prompt("Enter Host IP Address:", font, 30);
            prompt.setFillColor(sf::Color::White);
            prompt.setOrigin(prompt.getGlobalBounds().width / 2, 0);
            prompt.setPosition(kMenuW / 2, 250);
            startscreen.draw(prompt);

            sf::Text ipText(ipInput, font, 40);
            ipText.setFillColor(sf::Color::Green);
            ipText.setOrigin(ipText.getGlobalBounds().width / 2, 0);
            ipText.setPosition(kMenuW / 2, 320);
            startscreen.draw(ipText);

            sf::Text instruct("Press ENTER to connect", font, 25);
            instruct.setFillColor(sf::Color(200, 200, 200));
            instruct.setOrigin(instruct.getGlobalBounds().width / 2, 0);
            instruct.setPosition(kMenuW / 2, 400);
            startscreen.draw(instruct);

            if (!statusMessage.empty()) {
                sf::Text statusText(statusMessage, font, 22);
                if (statusMessage.find("Failed") != std::string::npos)
                    statusText.setFillColor(sf::Color::Red);
                else
                    statusText.setFillColor(sf::Color::Yellow);
                statusText.setOrigin(statusText.getGlobalBounds().width / 2, 0);
                statusText.setPosition(kMenuW / 2, 460);
                startscreen.draw(statusText);
            }
        } else if (menuState == AI_DIFFICULTY) {
            m_back->draw(startscreen);
            backLabel.setOrigin(backLabel.getGlobalBounds().width / 2,
                                backLabel.getGlobalBounds().height / 2);
            backLabel.setPosition(m_back->getPosition().x, m_back->getPosition().y);
            startscreen.draw(backLabel);

            sf::Text diffTitle("SELECT DIFFICULTY", font, 40);
            diffTitle.setFillColor(sf::Color::White);
            diffTitle.setStyle(sf::Text::Bold);
            diffTitle.setOrigin(diffTitle.getGlobalBounds().width / 2, 0);
            diffTitle.setPosition(kMenuW / 2, 80);
            startscreen.draw(diffTitle);

            m_easy->draw(startscreen);
            easyLabel.setOrigin(easyLabel.getGlobalBounds().width / 2,
                                easyLabel.getGlobalBounds().height / 2);
            easyLabel.setPosition(m_easy->getPosition().x, m_easy->getPosition().y);
            startscreen.draw(easyLabel);

            m_medium->draw(startscreen);
            mediumLabel.setOrigin(mediumLabel.getGlobalBounds().width / 2,
                                  mediumLabel.getGlobalBounds().height / 2);
            mediumLabel.setPosition(m_medium->getPosition().x, m_medium->getPosition().y);
            startscreen.draw(mediumLabel);

            m_hard->draw(startscreen);
            hardLabel.setOrigin(hardLabel.getGlobalBounds().width / 2,
                                hardLabel.getGlobalBounds().height / 2);
            hardLabel.setPosition(m_hard->getPosition().x, m_hard->getPosition().y);
            startscreen.draw(hardLabel);
        } else if (menuState == READY_TO_START) {
            m_start->draw(startscreen);
            localLabel.setString("START");
            localLabel.setOrigin(localLabel.getGlobalBounds().width / 2,
                                 localLabel.getGlobalBounds().height / 2);
            localLabel.setPosition(m_start->getPosition().x, m_start->getPosition().y);
            startscreen.draw(localLabel);

            sf::Text statusText(statusMessage, font, 30);
            statusText.setFillColor(sf::Color::Green);
            statusText.setStyle(sf::Text::Bold);
            statusText.setOrigin(statusText.getGlobalBounds().width / 2, 0);
            statusText.setPosition(kMenuW / 2, 350);
            startscreen.draw(statusText);
        }

        startscreen.display();
    }

    result.quit = true;
    return result;
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    try {
        initResourcePath(argv[0]);

        int highscore = 0;

        // ── CLI parsing ───────────────────────────────────────────────────────────
        DebugConfig config;
        unsigned seed = 0;
        bool haveSeed = false;

        auto needValue = [&](const std::string& flag, int& i) -> const char* {
            if (i + 1 >= argc)
                throw std::runtime_error(flag + " requires a value");
            return argv[++i];
        };
        for (int i = 1; i < argc; ++i) {
            std::string arg(argv[i]);
            try {
                if (arg == "--frames") {
                    config.frames = std::stoi(needValue(arg, i));
                } else if (arg == "--replay") {
                    config.replayPath = needValue(arg, i);
                } else if (arg == "--replay-p1") {
                    config.replayPathP1 = needValue(arg, i);
                } else if (arg == "--screenshot-every") {
                    config.screenshotEvery = std::stoi(needValue(arg, i));
                } else if (arg == "--screenshot-dir") {
                    config.screenshotDir = needValue(arg, i);
                } else if (arg == "--seed") {
                    seed = static_cast<unsigned>(std::stoul(needValue(arg, i)));
                    haveSeed = true;
                } else if (arg == "--ai") {
                    std::string val(needValue(arg, i));
                    if (val == "easy")
                        config.ai = AiDifficulty::Easy;
                    else if (val == "medium")
                        config.ai = AiDifficulty::Medium;
                    else if (val == "hard")
                        config.ai = AiDifficulty::Hard;
                    else
                        throw std::runtime_error("--ai must be easy, medium, or hard");
                } else if (arg == "--help") {
                    std::cout
                        << "Usage: dungeon_game [options]\n"
                        << "  --frames N             Run N sim steps then exit\n"
                        << "  --replay <file>        Drive P2 (robot) from replay file\n"
                        << "  --replay-p1 <file>     Drive P1 (rocket) from replay file\n"
                        << "  --screenshot-every N   Save screenshot every N steps\n"
                        << "  --screenshot-dir <d>   Directory for screenshots (default: .)\n"
                        << "  --seed <n>             Seed the RNG\n"
                        << "  --ai <easy|medium|hard>   AI opponent for P2 (requires --frames)\n"
                        << "\n"
                        << "Key bindings can be customised by placing a controls.cfg file\n"
                        << "next to the dungeon_game binary.  Example:\n"
                        << "  p1_up = Num8\n"
                        << "  p1_down = Num5\n"
                        << "  p1_left = Num4\n"
                        << "  p1_right = Num6\n"
                        << "  p1_attack = Right\n"
                        << "  p2_up = W\n"
                        << "  p2_down = S\n"
                        << "  p2_left = A\n"
                        << "  p2_right = D\n"
                        << "  p2_attack = Space\n"
                        << "  slow_down = O\n"
                        << "  speed_up = P\n"
                        << "  skip_cooldown = K\n"
                        << "\n"
                        << "Replay format: one line per step — 'up down left right attack' "
                           "(0 or 1)\n"
                        << "  Everything from '#' to end-of-line is ignored; blank lines "
                           "skipped.\n";
                    return 0;
                } else {
                    std::cerr << "Error: unknown option '" << arg << "' (try --help)\n";
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << " (option '" << arg << "')\n";
                return 1;
            }
        }

        // ── Debug / harness bypass ────────────────────────────────────────────────
        if (config.active()) {
            if (haveSeed)
                rng::seed(seed);
            if (config.screenshotEvery > 0)
                std::filesystem::create_directories(config.screenshotDir);

            Game game;
            game.setDebugConfig(config);
            auto result = game.run();

            std::cout << "P1 score: " << std::get<0>(result) << "\n"
                      << "P2 score: " << std::get<1>(result) << "\n"
                      << "Steps:    " << game.steps() << "\n";
            return 0;
        }

        // ── Load key bindings (exe-relative controls.cfg, absent → defaults) ──────
        KeyBindings bindings = defaultBindings();
        {
            namespace fs = std::filesystem;
            fs::path cfgPath = fs::path(exe_dir_path) / "controls.cfg";
            if (fs::exists(cfgPath)) {
                std::ifstream f(cfgPath);
                bindings = loadBindings(f);
            }
        }

        // ── Normal menu / game loop ───────────────────────────────────────────────
        while (true) {
            MenuResult menu = showMenu();
            if (menu.quit)
                break;

            bool playAgain = true;
            while (playAgain) {
                playAgain = false;

                Game game(menu.mode, menu.netManager);
                game.setBindings(bindings);
                if (menu.ai != AiDifficulty::None)
                    game.setAiOpponent(menu.ai);
                auto [p1score, p2score, minutes, seconds, p1win, peerLeft] = game.run();

                if (p1score > highscore)
                    highscore = p1score;
                if (p2score > highscore)
                    highscore = p2score;

                if (game.quitToMenu())
                    break; // user pressed Q → back to menu

                bool wantAgain = showEndscreen(highscore, p1score, p2score, minutes, seconds,
                                               peerLeft, menu.mode);
                if (wantAgain && menu.mode == NetworkMode::LOCAL)
                    playAgain = true;
            }
            // Reset session state so the next menu iteration is clean.
            menu.netManager = nullptr;
            menu.mode = NetworkMode::LOCAL;
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
