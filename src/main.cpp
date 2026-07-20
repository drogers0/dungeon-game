#include "AnimatedGameObject.h"
#include "Game.h"
#include "NetworkManager.h"
#include "ai.h"
#include "asset_load.h"
#include "debug.h"
#include "key_bindings.h"
#include "letterbox.h"
#include "menu.h"
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
        sf::FloatRect srect = m_start->getGlobalBounds();
        sf::FloatRect qrect = m_quit->getGlobalBounds();

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
                } else if (arg == "--menu-state") {
                    std::string val(needValue(arg, i));
                    static const char* validStates[] = {"main_menu", "host_waiting", "join_input",
                                                        "ai_difficulty", "ready_to_start"};
                    bool found = false;
                    for (const char* s : validStates)
                        if (val == s) {
                            found = true;
                            break;
                        }
                    if (!found) {
                        std::cerr << "Error: unknown --menu-state '" << val
                                  << "' (valid: main_menu host_waiting join_input ai_difficulty "
                                     "ready_to_start)\n";
                        return 1;
                    }
                    config.menuState = val;
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
                        << "  --menu-state <name>    Screenshot the given menu state and exit\n"
                        << "                         (main_menu|host_waiting|join_input|"
                           "ai_difficulty|ready_to_start)\n"
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

        // ── Menu harness bypass ───────────────────────────────────────────────────
        if (config.menuMode()) {
            showMenu(config);
            return 0;
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
                if (menu.ai != AiDifficulty::None) {
                    KeyBindings solo = bindings;
                    solo.p1 = bindings.p2; // human P1 uses WASD in single-player
                    game.setBindings(solo);
                    game.setAiOpponent(menu.ai);
                } else {
                    game.setBindings(bindings);
                }
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
