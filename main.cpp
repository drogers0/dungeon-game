//#pragma once
#include <SFML/Graphics.hpp>
#include "Game.h"
#include <iostream>
#include "AnimatedGameObject.h"
#include "resource_path.h"
#include <tuple>
#include <cstdio>

int highscore = 0;
int p1score = 0;
int p2score = 0;
std::string clack = "";

void startgame() {
    Game game;
    std::tuple<int,int,float,int,bool> tuple = game.run();
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
            startgame();
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
                        startgame();
                    }
                    if (event.key.code == sf::Keyboard::N) {
                        background.stop();
                        endscreen.close();
                    }
                }
            }
        }
}
    int main()
    {

        RegularGameObject david = RegularGameObject();
        david.load(resource_path + "david_background.png");

        sf::Music background;
        if(!background.openFromFile(resource_path + "m_start_background.wav")){
            std::cout << "your music is broken, go fix it" << std::endl;
        }

        background.play();
        background.setVolume(60);
        //background.setLoop(true);

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

        bool start_updated = false;
        bool quit_updated = false;
        sf::RenderWindow startscreen(sf::VideoMode(1024, 576), "gamestart");
        /*
        sf::Font font;
        if (!font.loadFromFile("content\\font.ttf")) {
            std::cout << "Font screwed up" << std::endl;
        }
        sf::Text text = sf::Text("PLAY? (Y)es/(N)o", font, 50 );
        text.setFillColor(sf::Color::White);
        text.setOrigin((text.getLocalBounds().width)/2, (text.getLocalBounds().height)/2);
        text.setPosition(startscreen.getSize().x/2, startscreen.getSize().y/2);
        */



        GameObject* m_start = new AnimatedGameObject(1331, 300, 2,1,2,0);
        m_start->load(resource_path + "start.png");
        m_start->setOrigin();
        m_start->setPosition(startscreen.getSize().x/2, startscreen.getSize().y / 3);
        m_start->setScale(.5f);
        //gamevec.push_back(m_start);


        GameObject* m_quit = new AnimatedGameObject(1332, 300, 2,1,2,0);
        m_quit->load(resource_path+"quit.png");
        m_quit->setOrigin();
        m_quit->setPosition((startscreen.getSize().x/2), 2*(startscreen.getSize().y / 3));
        m_quit->setScale(.5f);
        //gamevec.push_back(m_quit);

        while (startscreen.isOpen()) {

            //mouse.getPosition(startscreen);
            startscreen.clear();
            david.draw(startscreen);
            m_start->draw(startscreen);
            //std::cout << "draw" << std::endl;
            m_quit->draw(startscreen);
            sf::Event event;
            sf::Mouse mouse;
            //if (mouse.getPosition(startscreen).x > m_start->getPosition().x + (m_start->getWidth() / 2) && mouse.getPosition(startscreen).x < m_start->getPosition().x - (m_start->getWidth() / 2)) {
            sf::Rect<float> srect;
            // m_start->setOrigin();
            srect = sf::Rect<float>(sf::Vector2f(m_start->getPosition().x-(m_start->getWidth()/4),m_start->getPosition().y-(m_start->getHeight()/4)), sf::Vector2f(m_start->getWidth()*m_start->getScale().x, m_start->getHeight()*m_start->getScale().y));
            //m_start->setOrigin();
            //std::cout << (srect.contains(startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen)))) << std::endl;
            if (srect.contains(startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen))) && !start_updated) {
                m_start->update(0.0f);
                start_updated = true;
                press.play();
            } else if (!srect.contains(startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen))) && start_updated){
                start_updated = false;
                m_start->update(0.0f);
                up.play();
            }

            sf::Rect<float> qrect = sf::Rect<float>(sf::Vector2f(m_quit->getPosition().x-(m_quit->getWidth()/4),m_quit->getPosition().y-(m_quit->getHeight()/4)), sf::Vector2f(m_quit->getWidth()*m_quit->getScale().x, m_quit->getHeight()*m_quit->getScale().y));
            //m_start->setOrigin();
            //std::cout << (qrect.contains(startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen)))) << std::endl;
            if (qrect.contains(startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen))) && !quit_updated) {
                m_quit->update(0.0f);
                quit_updated = true;
                press.play();
                //std::cout << "I am changing the button" << std::endl;
            } else if (!qrect.contains(startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen))) && quit_updated){
                quit_updated = false;
                m_quit->update(0.0f);
                up.play();
                //std::cout << "button?" << std::endl;
            }

            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && srect.contains(startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen)))) {
                background.stop();
                startscreen.close();
                startgame();
            }
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && qrect.contains(startscreen.mapPixelToCoords(sf::Mouse::getPosition(startscreen)))) {
                background.stop();
                startscreen.close();
            }

            while (startscreen.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    background.stop();
                    startscreen.close();


                }if (event.type == sf::Event::KeyPressed){
                    if (event.key.code == sf::Keyboard::Y) {
                        std::cout << "OPEN GAME" << std::endl;
                        background.stop();
                        startscreen.close();
                        startgame();

                    }
                    if (event.key.code == sf::Keyboard::N) {
                        background.stop();
                        startscreen.close();
                    }
                }
            }
            startscreen.display();
        }
        return 0;
    }