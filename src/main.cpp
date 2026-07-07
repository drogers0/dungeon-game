#include <SFML/Graphics.hpp>
#include "Game.h"
#include "NetworkManager.h"
#include <iostream>
#include "AnimatedGameObject.h"
#include "asset_load.h"
#include "resource_path.h"
#include "debug.h"
#include "rng.h"
#include <filesystem>
#include <tuple>
#include <cstdio>
#include <memory>
#include <string>
#include <stdexcept>

void startgame(int& highscore, NetworkMode mode = NetworkMode::LOCAL, std::shared_ptr<NetworkManager> netManager = nullptr) {
    std::tuple<int,int,float,int,bool,bool> tuple;
    if (mode != NetworkMode::LOCAL && netManager) {
        Game game(mode, netManager);
        tuple = game.run();
    } else {
        Game game;
        tuple = game.run();
    }
    int p1score = std::get<0>(tuple);
    int p2score = std::get<1>(tuple);
    float tempM = std::get<2>(tuple);
    int tempS = std::get<3>(tuple);
    bool peerLeft = std::get<5>(tuple);
    char gTime [10];
    std::snprintf(gTime, sizeof(gTime), "%02.0f:%02d", tempM, tempS);
    std::string clack = gTime;

    if (p1score > highscore )
        highscore = p1score;
    if (p2score > highscore)
        highscore = p2score;

    sf::Font tfont;
    sf::Font pfont;
    sf::Text p1scoret;
    sf::Text p2scoret;
    sf::Text clackt;
    sf::Text highscoret;

    RegularGameObject wall = RegularGameObject();
    loadOrThrow(wall, resource_path + "david_background.png");
    wall.setScale(.7f);

    loadOrThrow(pfont, resource_path + "Joyful_Theatre.otf");
    loadOrThrow(tfont, resource_path + "timer.ttf");

    sf::RenderWindow endscreen(sf::VideoMode(1024, 576), "end game");

    sf::Text peerDisconnectedText("", pfont, 36);
    if (peerLeft) {
        peerDisconnectedText.setString("Peer disconnected");
        peerDisconnectedText.setFillColor(sf::Color::Red);
        peerDisconnectedText.setStyle(sf::Text::Bold);
        // Use getLocalBounds for centering — unaffected by world transform.
        peerDisconnectedText.setOrigin(peerDisconnectedText.getLocalBounds().width / 2.f, 0);
        peerDisconnectedText.setPosition(endscreen.getSize().x / 2.f, 460.f);
    }

    sf::Music background;
    loadOrThrow(background, resource_path + "m_start_background.wav");

    p1scoret = sf::Text(std::to_string(p1score), pfont, 50);
    p1scoret.setFillColor(sf::Color::Blue);
    p2scoret = sf::Text(std::to_string(p2score), pfont, 50);
    p2scoret.setFillColor(sf::Color::Green);
    clackt = sf::Text(clack, tfont, 50);
    clackt.setFillColor(sf::Color::Blue);

    highscoret = sf::Text(std::to_string(highscore), pfont,70);


    p1scoret.setPosition(5,endscreen.getSize().y/2);
    p2scoret.setPosition(endscreen.getSize().x-(p1scoret.getGlobalBounds().width)-5,endscreen.getSize().y/2);
    clackt.setPosition(endscreen.getSize().x/2 - clackt.getGlobalBounds().width+20,-10);
    highscoret.setPosition(endscreen.getSize().x/2 - highscoret.getGlobalBounds().width,25);

    if (p1score == highscore )
        highscoret.setFillColor(sf::Color::Blue);
    if (p2score == highscore)
        highscoret.setFillColor(sf::Color::Green);

    background.play();
    background.setVolume(60);

    sf::SoundBuffer down_buffer;
    sf::SoundBuffer up_buffer;
    loadOrThrow(down_buffer, resource_path + "ButtonOn.wav");
    loadOrThrow(up_buffer,   resource_path + "ButtonOff.wav");

    sf::Sound press;
    sf::Sound up;
    press.setBuffer(down_buffer);
    up.setBuffer(up_buffer);

    bool start_updated = false;
    bool quit_updated = false;

    auto m_start = std::make_unique<AnimatedGameObject>(1331, 300, 2, 1, 2, 0);
    loadOrThrow(*m_start, resource_path + "start.png");
    m_start->setOrigin();
    m_start->setPosition((endscreen.getSize().x / 2),(endscreen.getSize().y / 3));
    m_start->setScale(.5f);

    auto m_quit = std::make_unique<AnimatedGameObject>(1332, 300, 2, 1, 2, 0);
    loadOrThrow(*m_quit, resource_path + "quit.png");
    m_quit->setOrigin();
    m_quit->setPosition((endscreen.getSize().x / 2), 2 * (endscreen.getSize().y / 3));
    m_quit->setScale(.5f);

    while (endscreen.isOpen()) {

        endscreen.clear();
        wall.draw(endscreen);
        endscreen.draw(p1scoret);
        endscreen.draw(p2scoret);
        endscreen.draw(clackt);
        endscreen.draw(highscoret);
        m_start->draw(endscreen);
        m_quit->draw(endscreen);
        if (peerLeft) endscreen.draw(peerDisconnectedText);
        endscreen.display();

        sf::Event event;
        sf::Rect<float> srect;
        srect = sf::Rect<float>(sf::Vector2f(m_start->getPosition().x - (m_start->getWidth() / 4),
                                             m_start->getPosition().y - (m_start->getHeight() / 4)),
                                sf::Vector2f(m_start->getWidth() * m_start->getScale().x,
                                             m_start->getHeight() * m_start->getScale().y));
        if (srect.contains(endscreen.mapPixelToCoords(sf::Mouse::getPosition(endscreen))) && !start_updated) {
            m_start->update(0.0f);
            start_updated = true;
            press.play();
        } else if (!srect.contains(endscreen.mapPixelToCoords(sf::Mouse::getPosition(endscreen))) &&
                   start_updated) {
            start_updated = false;
            m_start->update(0.0f);
            up.play();
        }

        sf::Rect<float> qrect = sf::Rect<float>(sf::Vector2f(m_quit->getPosition().x - (m_quit->getWidth() / 4),
                                                             m_quit->getPosition().y - (m_quit->getHeight() / 4)),
                                                sf::Vector2f(m_quit->getWidth() * m_quit->getScale().x,
                                                             m_quit->getHeight() * m_quit->getScale().y));

        if (qrect.contains(endscreen.mapPixelToCoords(sf::Mouse::getPosition(endscreen))) && !quit_updated) {
            m_quit->update(0.0f);
            quit_updated = true;
            press.play();
        } else if (!qrect.contains(endscreen.mapPixelToCoords(sf::Mouse::getPosition(endscreen))) && quit_updated) {
            quit_updated = false;
            m_quit->update(0.0f);
            up.play();
        }

        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) &&
            srect.contains(endscreen.mapPixelToCoords(sf::Mouse::getPosition(endscreen)))) {
            background.stop();
            endscreen.close();
            startgame(highscore, mode, netManager);
        }
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) &&
            qrect.contains(endscreen.mapPixelToCoords(sf::Mouse::getPosition(endscreen)))) {
            background.stop();
            endscreen.close();
        }


            while (endscreen.pollEvent(event)) {
                if (event.type == sf::Event::Closed){
                    background.stop();
                    endscreen.close();
                }
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Y) {
                        background.stop();
                        endscreen.close();
                        startgame(highscore, mode, netManager);
                    }
                    if (event.key.code == sf::Keyboard::N) {
                        background.stop();
                        endscreen.close();
                    }
                }
            }
        }
}

enum MenuState {
    MAIN_MENU,
    HOST_WAITING,
    JOIN_INPUT,
    READY_TO_START
};

int main(int argc, char** argv)
{
  try {
    initResourcePath(argv[0]);

    int highscore = 0;

    // ── CLI parsing ───────────────────────────────────────────────────────────
    DebugConfig  config;
    unsigned     seed     = 0;
    bool         haveSeed = false;

    auto needValue = [&](const std::string& flag, int& i) -> const char* {
        if (i + 1 >= argc) throw std::runtime_error(flag + " requires a value");
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
                seed     = static_cast<unsigned>(std::stoul(needValue(arg, i)));
                haveSeed = true;
            } else if (arg == "--help") {
                std::cout
                    << "Usage: dungeon_game [options]\n"
                    << "  --frames N             Run N sim steps then exit\n"
                    << "  --replay <file>        Drive P2 (robot) from replay file\n"
                    << "  --replay-p1 <file>     Drive P1 (rocket) from replay file\n"
                    << "  --screenshot-every N   Save screenshot every N steps\n"
                    << "  --screenshot-dir <d>   Directory for screenshots (default: .)\n"
                    << "  --seed <n>             Seed the RNG\n"
                    << "\n"
                    << "Replay format: one line per step — 'up down left right attack' (0 or 1)\n"
                    << "  Everything from '#' to end-of-line is ignored; blank lines skipped.\n";
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

    // ── Normal menu flow (unchanged) ──────────────────────────────────────────
    MenuState menuState = MAIN_MENU;
    NetworkMode mode = NetworkMode::LOCAL;
    std::shared_ptr<NetworkManager> netManager = nullptr;
    std::string ipInput = "127.0.0.1";
    std::string statusMessage = "";
    std::string hostIpAddress = "";

    // Load resources
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
    loadOrThrow(up_buffer,   resource_path + "ButtonOff.wav");

    sf::Sound press;
    sf::Sound up;
    press.setBuffer(down_buffer);
    up.setBuffer(up_buffer);

    // Load font for UI text
    sf::Font font;
    loadOrThrow(font, resource_path + "oswald.ttf");

    bool start_updated = false;
    bool quit_updated = false;
    bool host_updated = false;
    bool join_updated = false;
    bool back_updated = false;
    
    sf::RenderWindow startscreen(sf::VideoMode(1024, 576), "Dungeon Game");
    // Create button objects for main menu
    auto m_start = std::make_unique<AnimatedGameObject>(1331, 300, 2,1,2,0);
    loadOrThrow(*m_start, resource_path + "start.png");
    m_start->setOrigin();
    m_start->setPosition(startscreen.getSize().x/2, 200);
    m_start->setScale(.4f);

    // Create host and join buttons (will use text labels with start button image)
    auto m_host = std::make_unique<AnimatedGameObject>(1331, 300, 2,1,2,0);
    loadOrThrow(*m_host, resource_path + "start.png");
    m_host->setOrigin();
    m_host->setPosition(startscreen.getSize().x/2, 320);
    m_host->setScale(.4f);

    auto m_join = std::make_unique<AnimatedGameObject>(1331, 300, 2,1,2,0);
    loadOrThrow(*m_join, resource_path + "start.png");
    m_join->setOrigin();
    m_join->setPosition(startscreen.getSize().x/2, 440);
    m_join->setScale(.4f);

    auto m_quit = std::make_unique<AnimatedGameObject>(1332, 300, 2,1,2,0);
    loadOrThrow(*m_quit, resource_path + "quit.png");
    m_quit->setOrigin();
    m_quit->setPosition((startscreen.getSize().x/2), 520);
    m_quit->setScale(.3f);

    // Back button (using quit button graphics)
    auto m_back = std::make_unique<AnimatedGameObject>(1332, 300, 2,1,2,0);
    loadOrThrow(*m_back, resource_path + "quit.png");
    m_back->setOrigin();
    m_back->setPosition(100, 50);
    m_back->setScale(.25f);

    // Text labels for buttons
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

    while (startscreen.isOpen()) {
        sf::Event event;
        while (startscreen.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                background.stop();
                startscreen.close();
            }
            
            // Handle text input for IP address in JOIN_INPUT state
            if (menuState == JOIN_INPUT && event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b' && !ipInput.empty()) {
                    ipInput.pop_back();
                } else if (event.text.unicode == '\r' || event.text.unicode == '\n') {
                    // Try to connect
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
                } else if (event.text.unicode < 128 && event.text.unicode != '\b' && ipInput.length() < 30) {
                    char c = static_cast<char>(event.text.unicode);
                    // Allow only valid IP characters
                    if ((c >= '0' && c <= '9') || c == '.' || c == ':') {
                        ipInput += c;
                    }
                }
            }
        }

        // Get mouse position
        sf::Vector2f mousePos = startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen));
        
        // Define button rectangles
        sf::Rect<float> startRect(m_start->getPosition().x - (m_start->getWidth()/4), 
                                   m_start->getPosition().y - (m_start->getHeight()/4),
                                   m_start->getWidth() * m_start->getScale().x, 
                                   m_start->getHeight() * m_start->getScale().y);
        
        sf::Rect<float> hostRect(m_host->getPosition().x - (m_host->getWidth()/4),
                                  m_host->getPosition().y - (m_host->getHeight()/4),
                                  m_host->getWidth() * m_host->getScale().x,
                                  m_host->getHeight() * m_host->getScale().y);
        
        sf::Rect<float> joinRect(m_join->getPosition().x - (m_join->getWidth()/4),
                                  m_join->getPosition().y - (m_join->getHeight()/4),
                                  m_join->getWidth() * m_join->getScale().x,
                                  m_join->getHeight() * m_join->getScale().y);
        
        sf::Rect<float> quitRect(m_quit->getPosition().x - (m_quit->getWidth()/4),
                                  m_quit->getPosition().y - (m_quit->getHeight()/4),
                                  m_quit->getWidth() * m_quit->getScale().x,
                                  m_quit->getHeight() * m_quit->getScale().y);
        
        sf::Rect<float> backRect(m_back->getPosition().x - (m_back->getWidth()/4),
                                  m_back->getPosition().y - (m_back->getHeight()/4),
                                  m_back->getWidth() * m_back->getScale().x,
                                  m_back->getHeight() * m_back->getScale().y);

        // Handle menu state logic
        if (menuState == MAIN_MENU) {
            // Start button hover
            if (startRect.contains(mousePos) && !start_updated) {
                m_start->update(0.0f);
                start_updated = true;
                press.play();
            } else if (!startRect.contains(mousePos) && start_updated) {
                start_updated = false;
                m_start->update(0.0f);
                up.play();
            }
            
            // Host button hover
            if (hostRect.contains(mousePos) && !host_updated) {
                m_host->update(0.0f);
                host_updated = true;
                press.play();
            } else if (!hostRect.contains(mousePos) && host_updated) {
                host_updated = false;
                m_host->update(0.0f);
                up.play();
            }
            
            // Join button hover
            if (joinRect.contains(mousePos) && !join_updated) {
                m_join->update(0.0f);
                join_updated = true;
                press.play();
            } else if (!joinRect.contains(mousePos) && join_updated) {
                join_updated = false;
                m_join->update(0.0f);
                up.play();
            }
            
            // Quit button hover
            if (quitRect.contains(mousePos) && !quit_updated) {
                m_quit->update(0.0f);
                quit_updated = true;
                press.play();
            } else if (!quitRect.contains(mousePos) && quit_updated) {
                quit_updated = false;
                m_quit->update(0.0f);
                up.play();
            }
            
            // Handle clicks
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                if (startRect.contains(mousePos)) {
                    mode = NetworkMode::LOCAL;
                    menuState = READY_TO_START;
                    sf::sleep(sf::milliseconds(200)); // Prevent double-click
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
                }
            }
        }
        else if (menuState == HOST_WAITING) {
            // Check for client connection
            if (netManager && netManager->waitForClient(sf::milliseconds(100))) {
                menuState = READY_TO_START;
                statusMessage = "Player 2 connected! Click START to begin";
            }
            
            // Back button hover
            if (backRect.contains(mousePos) && !back_updated) {
                m_back->update(0.0f);
                back_updated = true;
                press.play();
            } else if (!backRect.contains(mousePos) && back_updated) {
                back_updated = false;
                m_back->update(0.0f);
                up.play();
            }
            
            // Handle back button click
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && backRect.contains(mousePos)) {
                menuState = MAIN_MENU;
                netManager = nullptr;
                statusMessage = "";
                sf::sleep(sf::milliseconds(200));
            }
        }
        else if (menuState == JOIN_INPUT) {
            // Back button hover
            if (backRect.contains(mousePos) && !back_updated) {
                m_back->update(0.0f);
                back_updated = true;
                press.play();
            } else if (!backRect.contains(mousePos) && back_updated) {
                back_updated = false;
                m_back->update(0.0f);
                up.play();
            }
            
            // Handle back button click
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && backRect.contains(mousePos)) {
                menuState = MAIN_MENU;
                ipInput = "127.0.0.1";
                statusMessage = "";
                sf::sleep(sf::milliseconds(200));
            }
        }
        else if (menuState == READY_TO_START) {
            // Start button hover
            if (startRect.contains(mousePos) && !start_updated) {
                m_start->update(0.0f);
                start_updated = true;
                press.play();
            } else if (!startRect.contains(mousePos) && start_updated) {
                start_updated = false;
                m_start->update(0.0f);
                up.play();
            }
            
            // Handle start click
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && startRect.contains(mousePos)) {
                background.stop();
                startscreen.close();
                startgame(highscore, mode, netManager);
                return 0;
            }
        }

        // Render
        startscreen.clear();
        david.draw(startscreen);
        
        if (menuState == MAIN_MENU) {
            // Draw all buttons
            m_start->draw(startscreen);
            m_host->draw(startscreen);
            m_join->draw(startscreen);
            m_quit->draw(startscreen);
            
            // Draw labels on buttons
            localLabel.setOrigin(localLabel.getGlobalBounds().width/2, localLabel.getGlobalBounds().height/2);
            localLabel.setPosition(m_start->getPosition().x, m_start->getPosition().y);
            startscreen.draw(localLabel);
            
            hostLabel.setOrigin(hostLabel.getGlobalBounds().width/2, hostLabel.getGlobalBounds().height/2);
            hostLabel.setPosition(m_host->getPosition().x, m_host->getPosition().y);
            startscreen.draw(hostLabel);
            
            joinLabel.setOrigin(joinLabel.getGlobalBounds().width/2, joinLabel.getGlobalBounds().height/2);
            joinLabel.setPosition(m_join->getPosition().x, m_join->getPosition().y);
            startscreen.draw(joinLabel);
        }
        else if (menuState == HOST_WAITING) {
            m_back->draw(startscreen);
            backLabel.setOrigin(backLabel.getGlobalBounds().width/2, backLabel.getGlobalBounds().height/2);
            backLabel.setPosition(m_back->getPosition().x, m_back->getPosition().y);
            startscreen.draw(backLabel);
            
            sf::Text title("HOSTING GAME", font, 50);
            title.setFillColor(sf::Color::Green);
            title.setStyle(sf::Text::Bold);
            title.setOrigin(title.getGlobalBounds().width/2, 0);
            title.setPosition(startscreen.getSize().x/2, 150);
            startscreen.draw(title);
            
            sf::Text ipText("Your IP: " + hostIpAddress, font, 35);
            ipText.setFillColor(sf::Color::White);
            ipText.setOrigin(ipText.getGlobalBounds().width/2, 0);
            ipText.setPosition(startscreen.getSize().x/2, 250);
            startscreen.draw(ipText);
            
            sf::Text statusText(statusMessage, font, 30);
            statusText.setFillColor(sf::Color(200, 200, 200));
            statusText.setOrigin(statusText.getGlobalBounds().width/2, 0);
            statusText.setPosition(startscreen.getSize().x/2, 350);
            startscreen.draw(statusText);
        }
        else if (menuState == JOIN_INPUT) {
            m_back->draw(startscreen);
            backLabel.setOrigin(backLabel.getGlobalBounds().width/2, backLabel.getGlobalBounds().height/2);
            backLabel.setPosition(m_back->getPosition().x, m_back->getPosition().y);
            startscreen.draw(backLabel);
            
            sf::Text title("JOIN GAME", font, 50);
            title.setFillColor(sf::Color::Blue);
            title.setStyle(sf::Text::Bold);
            title.setOrigin(title.getGlobalBounds().width/2, 0);
            title.setPosition(startscreen.getSize().x/2, 150);
            startscreen.draw(title);
            
            sf::Text prompt("Enter Host IP Address:", font, 30);
            prompt.setFillColor(sf::Color::White);
            prompt.setOrigin(prompt.getGlobalBounds().width/2, 0);
            prompt.setPosition(startscreen.getSize().x/2, 250);
            startscreen.draw(prompt);
            
            sf::Text ipText(ipInput, font, 40);
            ipText.setFillColor(sf::Color::Green);
            ipText.setOrigin(ipText.getGlobalBounds().width/2, 0);
            ipText.setPosition(startscreen.getSize().x/2, 320);
            startscreen.draw(ipText);
            
            sf::Text instruct("Press ENTER to connect", font, 25);
            instruct.setFillColor(sf::Color(200, 200, 200));
            instruct.setOrigin(instruct.getGlobalBounds().width/2, 0);
            instruct.setPosition(startscreen.getSize().x/2, 400);
            startscreen.draw(instruct);
            
            if (!statusMessage.empty()) {
                sf::Text statusText(statusMessage, font, 22);
                if (statusMessage.find("Failed") != std::string::npos) {
                    statusText.setFillColor(sf::Color::Red);
                } else {
                    statusText.setFillColor(sf::Color::Yellow);
                }
                statusText.setOrigin(statusText.getGlobalBounds().width/2, 0);
                statusText.setPosition(startscreen.getSize().x/2, 460);
                startscreen.draw(statusText);
            }
        }
        else if (menuState == READY_TO_START) {
            m_start->draw(startscreen);
            localLabel.setString("START");
            localLabel.setOrigin(localLabel.getGlobalBounds().width/2, localLabel.getGlobalBounds().height/2);
            localLabel.setPosition(m_start->getPosition().x, m_start->getPosition().y);
            startscreen.draw(localLabel);
            
            sf::Text statusText(statusMessage, font, 30);
            statusText.setFillColor(sf::Color::Green);
            statusText.setStyle(sf::Text::Bold);
            statusText.setOrigin(statusText.getGlobalBounds().width/2, 0);
            statusText.setPosition(startscreen.getSize().x/2, 350);
            startscreen.draw(statusText);
        }
        
        startscreen.display();
    }
    
    background.stop();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}