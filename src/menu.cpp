#include "menu.h"
#include "RegularGameObject.h"
#include "asset_load.h"
#include "key_bindings.h"
#include "letterbox.h"
#include "menu_button.h"
#include "resource_path.h"
#include "screenshot.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ── helpers ───────────────────────────────────────────────────────────────────

// Build a MenuButton from a ButtonSpec, coloring label by button label text.
static MenuButton makeButton(const ButtonSpec& spec, const sf::Font& font) {
    MenuButton btn({spec.rect.size.x, spec.rect.size.y}, font, spec.label);
    btn.setPosition({spec.rect.position.x + spec.rect.size.x / 2.f,
                     spec.rect.position.y + spec.rect.size.y / 2.f});
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
bool parseMenuState(const std::string& name, MenuState& out) {
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
    if (name == "settings") {
        out = MenuState::SETTINGS;
        return true;
    }
    return false;
}

// Center a text object horizontally on a given position.
static void centerText(sf::Text& t, float x, float y) {
    auto lb = t.getLocalBounds();
    t.setOrigin({lb.position.x + lb.size.x / 2.f, lb.position.y});
    t.setPosition({x, y});
}

static void saveControls(const KeyBindings& bindings, std::string& statusMessage) {
    std::ofstream f(std::filesystem::path(exe_dir_path) / "controls.cfg");
    if (!f) {
        statusMessage = "Failed to save controls.cfg";
        return;
    }

    saveBindings(bindings, f);
    f.flush();
    if (!f) {
        statusMessage = "Failed to save controls.cfg";
        return;
    }

    statusMessage.clear();
}

// Render the current menu state into the window (shared between normal + harness loops).
static void renderMenu(sf::RenderWindow& win, RegularGameObject& bg, const sf::Font& font,
                       MenuState menuState, const std::string& ipInput,
                       const std::string& statusMessage, const std::string& hostIpAddress,
                       const sf::Vector2f& mousePos, const KeyBindings& bindings,
                       int capturingIdx) {
    win.clear();
    bg.draw(win);

    if (menuState == MenuState::SETTINGS) {
        sf::Text title(font, "CONTROLS", 40);
        title.setFillColor(sf::Color::White);
        title.setStyle(sf::Text::Style::Bold);
        centerText(title, kMenuW / 2.f, 30.f);
        win.draw(title);

        const char* headers[2] = {"PLAYER 1", "PLAYER 2"};
        for (int col = 0; col < 2; ++col) {
            sf::Text hdr(font, headers[col], 24);
            hdr.setFillColor(sf::Color(200, 200, 200));
            auto pos = settingsColumnHeaderPos(col);
            centerText(hdr, pos.x, pos.y);
            win.draw(hdr);
        }

        auto specs = settingsButtonSpecs(bindings);
        for (int i = 0; i < static_cast<int>(specs.size()); ++i) {
            auto btn = makeButton(specs[static_cast<std::size_t>(i)], font);
            btn.setHovered(specs[static_cast<std::size_t>(i)].rect.contains(mousePos));
            if (i == capturingIdx) {
                btn.label.setString("Press a key...");
                btn.shape.setFillColor({80, 80, 0});
                btn.label.setFillColor(sf::Color::Yellow);
                btn.setPosition(btn.shape.getPosition());
            }
            btn.draw(win);
        }
        if (!statusMessage.empty()) {
            sf::Text statusText(font, statusMessage, 22);
            statusText.setFillColor(statusMessage.find("Failed") != std::string::npos
                                        ? sf::Color::Red
                                        : sf::Color::Yellow);
            centerText(statusText, kMenuW / 2.f, 480.f);
            win.draw(statusText);
        }
        return;
    }

    auto specs = menuButtonRects(menuState);

    if (menuState == MenuState::HOST_WAITING || menuState == MenuState::JOIN_INPUT) {
        // Semi-transparent panel first, then text, then button on top.
        auto pr = menuInfoPanelRect();
        sf::RectangleShape panel({pr.size.x, pr.size.y});
        panel.setPosition({pr.position.x, pr.position.y});
        panel.setFillColor(sf::Color(0, 0, 0, 180));
        win.draw(panel);

        if (menuState == MenuState::HOST_WAITING) {
            sf::Text title(font, "HOSTING GAME", 50);
            title.setFillColor(sf::Color::Green);
            title.setStyle(sf::Text::Style::Bold);
            centerText(title, kMenuW / 2.f, 150.f);
            win.draw(title);

            sf::Text ipText(font, "Your IP: " + hostIpAddress, 35);
            ipText.setFillColor(sf::Color::White);
            centerText(ipText, kMenuW / 2.f, 250.f);
            win.draw(ipText);

            if (!statusMessage.empty()) {
                sf::Text statusText(font, statusMessage, 30);
                statusText.setFillColor(sf::Color(200, 200, 200));
                centerText(statusText, kMenuW / 2.f, 350.f);
                win.draw(statusText);
            }
        } else {
            sf::Text title(font, "JOIN GAME", 50);
            title.setFillColor(sf::Color::Blue);
            title.setStyle(sf::Text::Style::Bold);
            centerText(title, kMenuW / 2.f, 150.f);
            win.draw(title);

            sf::Text prompt(font, "Enter Host IP Address:", 30);
            prompt.setFillColor(sf::Color::White);
            centerText(prompt, kMenuW / 2.f, 250.f);
            win.draw(prompt);

            sf::Text ipText(font, ipInput, 40);
            ipText.setFillColor(sf::Color::Green);
            centerText(ipText, kMenuW / 2.f, 320.f);
            win.draw(ipText);

            sf::Text instruct(font, "Press ENTER to connect", 25);
            instruct.setFillColor(sf::Color(200, 200, 200));
            centerText(instruct, kMenuW / 2.f, 400.f);
            win.draw(instruct);

            if (!statusMessage.empty()) {
                sf::Text statusText(font, statusMessage, 22);
                statusText.setFillColor(statusMessage.find("Failed") != std::string::npos
                                            ? sf::Color::Red
                                            : sf::Color::Yellow);
                centerText(statusText, kMenuW / 2.f, 460.f);
                win.draw(statusText);
            }
        }
    } else if (menuState == MenuState::AI_DIFFICULTY) {
        sf::Text diffTitle(font, "SELECT DIFFICULTY", 40);
        diffTitle.setFillColor(sf::Color::White);
        diffTitle.setStyle(sf::Text::Style::Bold);
        centerText(diffTitle, kMenuW / 2.f, 80.f);
        win.draw(diffTitle);
    } else if (menuState == MenuState::READY_TO_START && !statusMessage.empty()) {
        sf::Text statusText(font, statusMessage, 30);
        statusText.setFillColor(sf::Color::Green);
        statusText.setStyle(sf::Text::Style::Bold);
        centerText(statusText, kMenuW / 2.f, 350.f);
        win.draw(statusText);
    }

    // Draw buttons on top of everything.
    for (const auto& spec : specs) {
        auto btn = makeButton(spec, font);
        // Hover-test the raw layout rect (== click rect), not bounds(), which the
        // outline inflates by 1.5px — otherwise a thin fringe highlights but doesn't click.
        btn.setHovered(spec.rect.contains(mousePos));
        btn.draw(win);
    }
    // Note: caller is responsible for calling win.display().
}

// ── showMenu ──────────────────────────────────────────────────────────────────

MenuResult showMenu(KeyBindings& bindings, const DebugConfig& cfg) {
    MenuState menuState = MenuState::MAIN_MENU;
    NetworkMode mode = NetworkMode::LOCAL;
    std::shared_ptr<NetworkManager> netManager = nullptr;
    std::string ipInput = "127.0.0.1";
    std::string statusMessage;
    std::string hostIpAddress;
    MenuResult result;
    int capturingIdx = -1;

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
    background.setLooping(true);

    sf::SoundBuffer down_buffer;
    loadOrThrow(down_buffer, resource_path + "ButtonOn.wav");

    sf::Sound press(down_buffer);

    sf::Font font;
    loadOrThrow(font, resource_path + "oswald.ttf");

    sf::RenderWindow startscreen(sf::VideoMode({1024u, 576u}), "Dungeon Game", sf::Style::Default);
    startscreen.setView(makeLetterboxView({kMenuW, kMenuH}, startscreen.getSize()));

    // ── Harness loop (bounded; exits after cfg.frames or window close) ─────────
    if (cfg.menuMode()) {
        const int effectiveFrames = (cfg.frames > 0 ? cfg.frames : 5);
        for (int frame = 0; frame < effectiveFrames; ++frame) {
            while (const auto event = startscreen.pollEvent()) {
                if (event->is<sf::Event::Closed>())
                    startscreen.close();
            }
            if (!startscreen.isOpen())
                break;
            sf::Vector2f mousePos =
                startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen));
            renderMenu(startscreen, david, font, menuState, ipInput, statusMessage, hostIpAddress,
                       mousePos, bindings, capturingIdx);
            if (cfg.screenshotEvery > 0 && frame % cfg.screenshotEvery == 0) {
                captureScreenshot(startscreen,
                                  cfg.screenshotDir + "/menu_" + std::to_string(frame) + ".png");
            }
            startscreen.display();
        }
        startscreen.close();
        result.quit = true;
        return result;
    }

    // ── Normal interactive loop ────────────────────────────────────────────────
    while (startscreen.isOpen()) {
        while (const auto event = startscreen.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                background.stop();
                startscreen.close();
                result.quit = true;
                return result;
            }
            if (const auto* rs = event->getIf<sf::Event::Resized>()) {
                startscreen.setView(makeLetterboxView({kMenuW, kMenuH}, rs->size));
            }
            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (menuState == MenuState::SETTINGS) {
                    if (capturingIdx >= 0) {
                        if (kp->code == sf::Keyboard::Key::Escape) {
                            capturingIdx = -1;
                        } else if (nameFromKey(kp->code) == "Unknown") {
                            // Key not in kKeyTable (IME, multimedia, etc.) — silently cancel.
                            capturingIdx = -1;
                        } else if (isReservedKey(kp->code, bindings)) {
                            capturingIdx = -1;
                            statusMessage = "Reserved key";
                        } else {
                            bindings = applyBindingEdit(bindings, capturingIdx, kp->code);
                            saveControls(bindings, statusMessage);
                            capturingIdx = -1;
                        }
                    } else if (kp->code == sf::Keyboard::Key::Escape) {
                        capturingIdx = -1;
                        menuState = MenuState::MAIN_MENU;
                    }
                }
            }
            if (const auto* te = event->getIf<sf::Event::TextEntered>();
                te && menuState == MenuState::JOIN_INPUT) {
                if (te->unicode == '\b' && !ipInput.empty()) {
                    ipInput.pop_back();
                } else if (te->unicode == '\r' || te->unicode == '\n') {
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
                } else if (te->unicode < 128 && te->unicode != '\b' && ipInput.length() < 30) {
                    char c = static_cast<char>(te->unicode);
                    if ((c >= '0' && c <= '9') || c == '.' || c == ':')
                        ipInput += c;
                }
            }
            if (const auto* mb = event->getIf<sf::Event::MouseButtonReleased>();
                mb && mb->button == sf::Mouse::Button::Left) {
                sf::Vector2f mp = startscreen.mapPixelToCoords(mb->position);

                if (menuState == MenuState::MAIN_MENU) {
                    auto specs = menuButtonRects(menuState);
                    // specs: [0]=1PLAYER [1]=2PLAYER [2]=HOST [3]=JOIN [4]=CONTROLS [5]=QUIT
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
                            {
                                auto local = sf::IpAddress::getLocalAddress();
                                hostIpAddress = local ? local->toString() : "unknown";
                            }
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
                        menuState = MenuState::SETTINGS;
                        capturingIdx = -1;
                        statusMessage.clear();
                        press.play();
                    } else if (specs[5].rect.contains(mp)) {
                        background.stop();
                        startscreen.close();
                        result.quit = true;
                        return result;
                    }
                } else if (menuState == MenuState::HOST_WAITING) {
                    auto specs = menuButtonRects(menuState);
                    // specs: [0]=BACK
                    if (specs[0].rect.contains(mp)) {
                        menuState = MenuState::MAIN_MENU;
                        netManager = nullptr;
                        statusMessage = "";
                        press.play();
                    }
                } else if (menuState == MenuState::JOIN_INPUT) {
                    auto specs = menuButtonRects(menuState);
                    // specs: [0]=BACK
                    if (specs[0].rect.contains(mp)) {
                        menuState = MenuState::MAIN_MENU;
                        ipInput = "127.0.0.1";
                        statusMessage = "";
                        press.play();
                    }
                } else if (menuState == MenuState::AI_DIFFICULTY) {
                    auto specs = menuButtonRects(menuState);
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
                    auto specs = menuButtonRects(menuState);
                    // specs: [0]=START
                    if (specs[0].rect.contains(mp)) {
                        background.stop();
                        startscreen.close();
                        result.mode = mode;
                        result.netManager = netManager;
                        return result;
                    }
                } else if (menuState == MenuState::SETTINGS) {
                    auto specs = settingsButtonSpecs(bindings);
                    // specs: [0-9]=binding rows [10]=RESET [11]=BACK
                    bool handled = false;
                    for (int i = 0; i < 10; ++i) {
                        if (specs[static_cast<std::size_t>(i)].rect.contains(mp)) {
                            capturingIdx = i;
                            statusMessage.clear();
                            press.play();
                            handled = true;
                            break;
                        }
                    }
                    if (!handled) {
                        if (specs[10].rect.contains(mp)) {
                            bindings = defaultBindings();
                            saveControls(bindings, statusMessage);
                            capturingIdx = -1;
                            press.play();
                        } else if (specs[11].rect.contains(mp)) {
                            menuState = MenuState::MAIN_MENU;
                            capturingIdx = -1;
                            press.play();
                        }
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
                   mousePos, bindings, capturingIdx);
        startscreen.display();
    }

    result.quit = true;
    return result;
}
