//#pragma once
#include <SFML/Graphics.hpp>
#include "Game.h"
#include "NetworkManager.h"
#include <iostream>
#include "AnimatedGameObject.h"
#include "resource_path.h"
#include <tuple>
#include <cstdio>
#include <memory>

int highscore = 0;
int p1score = 0;
int p2score = 0;
std::string clack = "";

void endscreen(NetworkMode mode, std::shared_ptr<NetworkManager> netManager);

void startgame(NetworkMode mode = NetworkMode::LOCAL, std::shared_ptr<NetworkManager> netManager = nullptr) {
    std::tuple<int,int,float,int,bool> tuple;
    if (mode != NetworkMode::LOCAL && netManager) {
        Game game(mode, netManager);
        tuple = game.run();
    } else {
        Game game;
        tuple = game.run();
    }
    p1score = std::get<0>(tuple);
    p2score = std::get<1>(tuple);
    float tempM = std::get<2>(tuple);
    int tempS = std::get<3>(tuple);
    //bool p1win = std::get<4>(tuple);
    char gTime [10];
    std::snprintf(gTime, sizeof(gTime), "%02.0f:%02d", tempM, tempS);
    clack = gTime;

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
    wall.load(resource_path + "david_background.png");
    wall.setScale(.7f);

    if(!pfont.loadFromFile(resource_path + "Joyful_Theatre.otf")){
        std::cout << "Font did not load" << std::endl;
    }

    if(!tfont.loadFromFile(resource_path + "timer.ttf")){
        std::cout << "Font did not load" << std::endl;
    }

    sf::RenderWindow endscreen(sf::VideoMode(1024, 576), "end game");
    sf::Event event;

    sf::Music background;
    if (!background.openFromFile(resource_path + "m_start_background.wav")) {
        std::cout << "your music is broken, go fix it" << std::endl;
    }

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
    //background.setLoop(true);

    sf::SoundBuffer down_buffer;
    sf::SoundBuffer up_buffer;
    if (!down_buffer.loadFromFile(resource_path + "ButtonOn.wav")) {
        std::cout << "sound did not load";
    }
    if (!up_buffer.loadFromFile(resource_path + "ButtonOff.wav")) {
        std::cout << "sound did not load";
    }

    sf::Sound press;
    sf::Sound up;
    press.setBuffer(down_buffer);
    up.setBuffer(up_buffer);

    bool start_updated = false;
    bool quit_updated = false;

    GameObject *m_start = new AnimatedGameObject(1331, 300, 2, 1, 2, 0);
    m_start->load(resource_path + "start.png");
    m_start->setOrigin();
    m_start->setPosition((endscreen.getSize().x / 2),(endscreen.getSize().y / 3));
    m_start->setScale(.5f);


    GameObject *m_quit = new AnimatedGameObject(1332, 300, 2, 1, 2, 0);
    m_quit->load(resource_path + "quit.png");
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
        endscreen.display();

        sf::Event event;
        sf::Mouse mouse;
        sf::Rect<float> srect;
        // m_start->setOrigin();
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

        //std::cout << (qrect.contains(endscreen.mapPixelToCoords(sf::Mouse::getPosition(endscreen)))) << std::endl;
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
            startgame(mode, netManager);
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
                        startgame(mode, netManager);
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

int main()
{
    MenuState menuState = MAIN_MENU;
    NetworkMode mode = NetworkMode::LOCAL;
    std::shared_ptr<NetworkManager> netManager = nullptr;
    std::string ipInput = "127.0.0.1";
    std::string statusMessage = "";
    std::string hostIpAddress = "";

    // Load resources
    RegularGameObject david = RegularGameObject();
    david.load(resource_path + "david_background.png");

    sf::Music background;
    if(!background.openFromFile(resource_path + "m_start_background.wav")){
        std::cout << "your music is broken, go fix it" << std::endl;
    }

    background.play();
    background.setVolume(60);
    background.setLoop(true);

    sf::SoundBuffer down_buffer;
    sf::SoundBuffer up_buffer;
    if(!down_buffer.loadFromFile(resource_path + "ButtonOn.wav")) {
        std::cout << "sound did not load";
    }
    if(!up_buffer.loadFromFile(resource_path + "ButtonOff.wav")) {
        std::cout << "sound did not load";
    }

    sf::Sound press;
    sf::Sound up;
    press.setBuffer(down_buffer);
    up.setBuffer(up_buffer);

    // Load font for UI text
    sf::Font font;
    if(!font.loadFromFile(resource_path + "oswald.ttf")){
        std::cout << "Font did not load" << std::endl;
    }

    bool start_updated = false;
    bool quit_updated = false;
    bool host_updated = false;
    bool join_updated = false;
    bool back_updated = false;
    
    sf::RenderWindow startscreen(sf::VideoMode(1024, 576), "Dungeon Game");
    // Create button objects for main menu
    GameObject* m_start = new AnimatedGameObject(1331, 300, 2,1,2,0);
    m_start->load(resource_path + "start.png");
    m_start->setOrigin();
    m_start->setPosition(startscreen.getSize().x/2, 200);
    m_start->setScale(.4f);

    // Create host and join buttons (will use text labels with start button image)
    GameObject* m_host = new AnimatedGameObject(1331, 300, 2,1,2,0);
    m_host->load(resource_path + "start.png");
    m_host->setOrigin();
    m_host->setPosition(startscreen.getSize().x/2, 320);
    m_host->setScale(.4f);

    GameObject* m_join = new AnimatedGameObject(1331, 300, 2,1,2,0);
    m_join->load(resource_path + "start.png");
    m_join->setOrigin();
    m_join->setPosition(startscreen.getSize().x/2, 440);
    m_join->setScale(.4f);

    GameObject* m_quit = new AnimatedGameObject(1332, 300, 2,1,2,0);
    m_quit->load(resource_path+"quit.png");
    m_quit->setOrigin();
    m_quit->setPosition((startscreen.getSize().x/2), 520);
    m_quit->setScale(.3f);
    
    // Back button (using quit button graphics)
    GameObject* m_back = new AnimatedGameObject(1332, 300, 2,1,2,0);
    m_back->load(resource_path+"quit.png");
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
                        statusMessage = "Failed to connect. Check IP and try again (Enter to retry)";
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
                startgame(mode, netManager);
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
}