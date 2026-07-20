#include "menu.h"
#include "RegularGameObject.h"
#include "asset_load.h"
#include "letterbox.h"
#include "menu_button.h"
#include "resource_path.h"
#include "screenshot.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ── helpers ───────────────────────────────────────────────────────────────────

// Build a MenuButton from a ButtonSpec, coloring label by button label text.
static MenuButton makeButton(const ButtonSpec& spec, const sf::Font& font) {
    MenuButton btn({spec.rect.width, spec.rect.height}, font, spec.label);
    btn.setPosition(
        {spec.rect.left + spec.rect.width / 2.f, spec.rect.top + spec.rect.height / 2.f});
    if (spec.label == "1 PLAYER")
        btn.label.setFillColor(sf::Color::Yellow);
    else if (spec.label == "HOST")
        btn.label.setFillColor(sf::Color::Green);
    else if (spec.label == "JOIN")
        btn.label.setFillColor(sf::Color::Blue);
    else if (spec.label == "QUIT")
        btn.label.setFillColor(sf::Color::Red);
    else if (spec.label == "EASY")
        btn.label.setFillColor(sf::Color::Green);
    else if (spec.label == "MEDIUM")
        btn.label.setFillColor(sf::Color::Yellow);
    else if (spec.label == "HARD")
        btn.label.setFillColor(sf::Color::Red);
    else
        btn.label.setFillColor(sf::Color::White);
    return btn;
}

// Map a state-name string to MenuState enum.
static bool parseMenuState(const std::string& name, MenuState& out) {
    if (name == "main_menu") {
        out = MenuState::MAIN_MENU;
        return true;
    }
    if (name == "host_waiting") {
        out = MenuState::HOST_WAITING;
        return true;
    }
    if (name == "join_input") {
        out = MenuState::JOIN_INPUT;
        return true;
    }
    if (name == "ai_difficulty") {
        out = MenuState::AI_DIFFICULTY;
        return true;
    }
    if (name == "ready_to_start") {
        out = MenuState::READY_TO_START;
        return true;
    }
    return false;
}

// Center a text object horizontally on a given position.
static void centerText(sf::Text& t, float x, float y) {
    auto lb = t.getLocalBounds();
    t.setOrigin(lb.left + lb.width / 2.f, lb.top);
    t.setPosition(x, y);
}

// Render the current menu state into the window (shared between normal + harness loops).
static void renderMenu(sf::RenderWindow& win, RegularGameObject& bg, const sf::Font& font,
                       MenuState menuState, const std::string& ipInput,
                       const std::string& statusMessage, const std::string& hostIpAddress,
                       const sf::Vector2f& mousePos) {
    win.clear();
    bg.draw(win);

    auto specs = menuButtonRects(menuState);

    if (menuState == MenuState::HOST_WAITING || menuState == MenuState::JOIN_INPUT) {
        // Semi-transparent panel first, then text, then button on top.
        auto pr = menuInfoPanelRect();
        sf::RectangleShape panel({pr.width, pr.height});
        panel.setPosition(pr.left, pr.top);
        panel.setFillColor(sf::Color(0, 0, 0, 180));
        win.draw(panel);

        if (menuState == MenuState::HOST_WAITING) {
            sf::Text title("HOSTING GAME", font, 50);
            title.setFillColor(sf::Color::Green);
            title.setStyle(sf::Text::Bold);
            centerText(title, kMenuW / 2.f, 150.f);
            win.draw(title);

            sf::Text ipText("Your IP: " + hostIpAddress, font, 35);
            ipText.setFillColor(sf::Color::White);
            centerText(ipText, kMenuW / 2.f, 250.f);
            win.draw(ipText);

            if (!statusMessage.empty()) {
                sf::Text statusText(statusMessage, font, 30);
                statusText.setFillColor(sf::Color(200, 200, 200));
                centerText(statusText, kMenuW / 2.f, 350.f);
                win.draw(statusText);
            }
        } else {
            sf::Text title("JOIN GAME", font, 50);
            title.setFillColor(sf::Color::Blue);
            title.setStyle(sf::Text::Bold);
            centerText(title, kMenuW / 2.f, 150.f);
            win.draw(title);

            sf::Text prompt("Enter Host IP Address:", font, 30);
            prompt.setFillColor(sf::Color::White);
            centerText(prompt, kMenuW / 2.f, 250.f);
            win.draw(prompt);

            sf::Text ipText(ipInput, font, 40);
            ipText.setFillColor(sf::Color::Green);
            centerText(ipText, kMenuW / 2.f, 320.f);
            win.draw(ipText);

            sf::Text instruct("Press ENTER to connect", font, 25);
            instruct.setFillColor(sf::Color(200, 200, 200));
            centerText(instruct, kMenuW / 2.f, 400.f);
            win.draw(instruct);

            if (!statusMessage.empty()) {
                sf::Text statusText(statusMessage, font, 22);
                statusText.setFillColor(statusMessage.find("Failed") != std::string::npos
                                            ? sf::Color::Red
                                            : sf::Color::Yellow);
                centerText(statusText, kMenuW / 2.f, 460.f);
                win.draw(statusText);
            }
        }
    } else if (menuState == MenuState::AI_DIFFICULTY) {
        sf::Text diffTitle("SELECT DIFFICULTY", font, 40);
        diffTitle.setFillColor(sf::Color::White);
        diffTitle.setStyle(sf::Text::Bold);
        centerText(diffTitle, kMenuW / 2.f, 80.f);
        win.draw(diffTitle);
    } else if (menuState == MenuState::READY_TO_START && !statusMessage.empty()) {
        sf::Text statusText(statusMessage, font, 30);
        statusText.setFillColor(sf::Color::Green);
        statusText.setStyle(sf::Text::Bold);
        centerText(statusText, kMenuW / 2.f, 350.f);
        win.draw(statusText);
    }

    // Draw buttons on top of everything.
    for (const auto& spec : specs) {
        auto btn = makeButton(spec, font);
        btn.setHovered(btn.bounds().contains(mousePos));
        btn.draw(win);
    }
    // Note: caller is responsible for calling win.display().
}

// ── showMenu ──────────────────────────────────────────────────────────────────

MenuResult showMenu(const DebugConfig& cfg) {
    MenuState menuState = MenuState::MAIN_MENU;
    NetworkMode mode = NetworkMode::LOCAL;
    std::shared_ptr<NetworkManager> netManager = nullptr;
    std::string ipInput = "127.0.0.1";
    std::string statusMessage;
    std::string hostIpAddress;
    MenuResult result;

    // Harness: parse the requested state.
    if (cfg.menuMode()) {
        MenuState requested;
        if (!parseMenuState(cfg.menuState, requested)) {
            std::cerr << "showMenu: unknown menuState '" << cfg.menuState
                      << "', defaulting to main_menu\n";
        } else {
            menuState = requested;
        }
        if (!cfg.screenshotDir.empty())
            std::filesystem::create_directories(cfg.screenshotDir);
    }

    RegularGameObject david;
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

    sf::RenderWindow startscreen(sf::VideoMode(1024, 576), "Dungeon Game", sf::Style::Default);
    startscreen.setView(makeLetterboxView({kMenuW, kMenuH}, startscreen.getSize()));

    // ── Harness loop (bounded; exits after cfg.frames or window close) ─────────
    if (cfg.menuMode()) {
        const int effectiveFrames = (cfg.frames > 0 ? cfg.frames : 5);
        for (int frame = 0; frame < effectiveFrames; ++frame) {
            sf::Event event;
            while (startscreen.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    startscreen.close();
                    goto harness_done;
                }
            }
            sf::Vector2f mousePos =
                startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen));
            renderMenu(startscreen, david, font, menuState, ipInput, statusMessage, hostIpAddress,
                       mousePos);
            if (cfg.screenshotEvery > 0 && frame % cfg.screenshotEvery == 0) {
                captureScreenshot(startscreen,
                                  cfg.screenshotDir + "/menu_" + std::to_string(frame) + ".png");
            }
            startscreen.display();
        }
    harness_done:
        startscreen.close();
        result.quit = true;
        return result;
    }

    // ── Normal interactive loop ────────────────────────────────────────────────
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
            if (menuState == MenuState::JOIN_INPUT && event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b' && !ipInput.empty()) {
                    ipInput.pop_back();
                } else if (event.text.unicode == '\r' || event.text.unicode == '\n') {
                    netManager = std::make_shared<NetworkManager>();
                    statusMessage = "Connecting...";
                    if (netManager->connectToHost(ipInput)) {
                        mode = NetworkMode::CLIENT;
                        menuState = MenuState::READY_TO_START;
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
            if (event.type == sf::Event::MouseButtonReleased &&
                event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mp =
                    startscreen.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
                auto specs = menuButtonRects(menuState);

                if (menuState == MenuState::MAIN_MENU) {
                    // specs: [0]=1PLAYER [1]=2PLAYER [2]=HOST [3]=JOIN [4]=QUIT
                    if (specs[0].rect.contains(mp)) {
                        menuState = MenuState::AI_DIFFICULTY;
                        press.play();
                    } else if (specs[1].rect.contains(mp)) {
                        mode = NetworkMode::LOCAL;
                        menuState = MenuState::READY_TO_START;
                        press.play();
                    } else if (specs[2].rect.contains(mp)) {
                        netManager = std::make_shared<NetworkManager>();
                        if (netManager->startHost()) {
                            mode = NetworkMode::HOST;
                            menuState = MenuState::HOST_WAITING;
                            hostIpAddress = sf::IpAddress::getLocalAddress().toString();
                            statusMessage = "Waiting for player 2...";
                        } else {
                            statusMessage = "Failed to start host!";
                            netManager = nullptr;
                        }
                        press.play();
                    } else if (specs[3].rect.contains(mp)) {
                        menuState = MenuState::JOIN_INPUT;
                        statusMessage = "Enter host IP address";
                        press.play();
                    } else if (specs[4].rect.contains(mp)) {
                        background.stop();
                        startscreen.close();
                        result.quit = true;
                        return result;
                    }
                } else if (menuState == MenuState::HOST_WAITING) {
                    // specs: [0]=BACK
                    if (specs[0].rect.contains(mp)) {
                        menuState = MenuState::MAIN_MENU;
                        netManager = nullptr;
                        statusMessage = "";
                        press.play();
                    }
                } else if (menuState == MenuState::JOIN_INPUT) {
                    // specs: [0]=BACK
                    if (specs[0].rect.contains(mp)) {
                        menuState = MenuState::MAIN_MENU;
                        ipInput = "127.0.0.1";
                        statusMessage = "";
                        press.play();
                    }
                } else if (menuState == MenuState::AI_DIFFICULTY) {
                    // specs: [0]=EASY [1]=MEDIUM [2]=HARD [3]=BACK
                    if (specs[0].rect.contains(mp)) {
                        result.ai = AiDifficulty::Easy;
                        mode = NetworkMode::LOCAL;
                        menuState = MenuState::READY_TO_START;
                        press.play();
                    } else if (specs[1].rect.contains(mp)) {
                        result.ai = AiDifficulty::Medium;
                        mode = NetworkMode::LOCAL;
                        menuState = MenuState::READY_TO_START;
                        press.play();
                    } else if (specs[2].rect.contains(mp)) {
                        result.ai = AiDifficulty::Hard;
                        mode = NetworkMode::LOCAL;
                        menuState = MenuState::READY_TO_START;
                        press.play();
                    } else if (specs[3].rect.contains(mp)) {
                        menuState = MenuState::MAIN_MENU;
                        press.play();
                    }
                } else if (menuState == MenuState::READY_TO_START) {
                    // specs: [0]=START
                    if (specs[0].rect.contains(mp)) {
                        background.stop();
                        startscreen.close();
                        result.mode = mode;
                        result.netManager = netManager;
                        return result;
                    }
                }
            }
        }

        // Poll for client connection in HOST_WAITING.
        if (menuState == MenuState::HOST_WAITING) {
            if (netManager && netManager->waitForClient(sf::milliseconds(0))) {
                menuState = MenuState::READY_TO_START;
                statusMessage = "Player 2 connected! Click START to begin";
            }
        }

        sf::Vector2f mousePos = startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen));
        renderMenu(startscreen, david, font, menuState, ipInput, statusMessage, hostIpAddress,
                   mousePos);
        startscreen.display();
    }

    result.quit = true;
    return result;
}
