//
// Created by bswenson3 on 11/9/16.
//
#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include "RegularGameObject.h"
#include "GameObject.h"
#include "AnimatedGameObject.h"
#include <tuple>

class Game {
public:
	//use default screen size
	Game();
	//run the game
	std::tuple<int,int,float,int,bool> run();

private:
	void processEvents();
	//update the game objects
	void update(sf::Time deltaT,float time);
	//draw the scene
	void render();
	//handle input from the user
	void handlePlayerInput(sf::Keyboard::Key key, bool isDown);
	//check collison with walls or other objects
	bool collision(GameObject* a,GameObject* b);
	bool collision(sf::Rect<float> a,GameObject* b);

	sf::RenderWindow m_window;

	sf::SoundBuffer gongbuffer;
	sf::SoundBuffer sbuffer;
    sf::SoundBuffer p2hitbuffer;
    sf::SoundBuffer laserbuffer;
    sf::SoundBuffer metalbuffer;
    sf::SoundBuffer speedbuffer;
    sf::SoundBuffer slowbuffer;
    sf::SoundBuffer burnbuffer;
    sf::Sound gong;
	sf::Sound sword;
	sf::Sound p2hit;
	sf::Sound laser;
	sf::Sound metal;
	sf::Sound speed_up;
	sf::Sound slow_down;
	sf::Sound burn;

	sf::Music background;

	GameObject* m_player = new AnimatedGameObject(404,206,3,3,9,0);
	GameObject* player2 = new AnimatedGameObject(959,180,8,1,8,0);
	std::vector<GameObject*> stuff;

    sf::Font font;
    sf::Font tfont;
    sf::Font block;
    sf::Text text;
    sf::Text text2;
    sf::Text timer;
    sf::Text pause_text;
    sf::Text info;


	float m_speed = 1200.0f;
	float temp_time = 0;
	float p1_timeout = 0;
	float p2_timeout = 0;
	bool m_left = false;
	bool m_right = false;
	bool m_up = false;
	bool m_down = false;
	bool w = false;
	bool a = false;
	bool s = false;
	bool d = false;
	bool space = false;
	int points = 0;
	int p2points = 0;
	int right = false;
	bool p1left = false;
	bool p2left = true;
	bool p1move = true;
	bool p2move = true;
	bool p = false;
	bool o = false;
	bool wait = false;
	bool p1_time_passed = true;
	bool p2_time_passed = true;

};

