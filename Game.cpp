//
// Created by bswenson3 on 11/9/16.
//

#include "Game.h"
#include "resource_path.h"
#include <iostream>
#include <cmath>
#include <cstdio>

Game::Game() :
		m_window(sf::VideoMode(1920, 1080), "Dungeon Game"),
		m_networkMode(NetworkMode::LOCAL)
{

	if(!background.openFromFile(resource_path + "background.wav")){
		std::cout << "your music is broken, go fix it" << std::endl;
	}

	//load the player
	//m_player.load(resource_path + "Mario.png");
	m_player->load(resource_path + "rocket.png");
	m_player->setScale(2.0f);
	//size him.  trial and error to get correct values
	//m_player->setScale(-1.0f,1.0f);
	m_player->setPosition(m_player->getWidth(),400);

	player2->load(resource_path + "robot.png");
	player2->setScale(-1.0f,1.0f);
	player2->setPosition(m_window.getSize().x-(m_player->getWidth()+150),400);
/*
	GameObject* asteroid = new AnimatedGameObject(360,280,5,4,19,0);
	stuff.push_back(asteroid);
	stuff[0]->load(resource_path + "asteroid.png");
    stuff[0]->setPosition(300,200);
*/
/*	GameObject* fire = new AnimatedGameObject(216,216,5,3,10,0);
    stuff.push_back(fire);
    stuff[1]->load(resource_path + "fire.png");
    stuff[1]->setPosition(200,900);*/

	GameObject* wall = new RegularGameObject();
	stuff.push_back(wall);
	stuff[0]->load(resource_path + "brick.png");
	//stuff[0]->setPosition(100,300);
	stuff[0]->setScale(2.0f);

	GameObject* fire = new AnimatedGameObject(216,216,5,3,10,0);
	fire->load(resource_path + "fire.png");
	fire->setScale(2.0f);
	fire->setPosition(-15,400);
	stuff.push_back(fire);

	GameObject* fire2 = new AnimatedGameObject(216,216,5,3,10,0);
	fire2->load(resource_path + "fire.png");
	fire2->setScale(2.0f);
	fire2->setPosition((1920/2)-5,400);
	stuff.push_back(fire2);

	GameObject* fire3 = new AnimatedGameObject(216,216,5,3,10,0);
	fire3->load(resource_path + "fire.png");
	fire3->setScale(2.0f);
	fire3->setPosition(1820,400);
	stuff.push_back(fire3);
/*
    GameObject* astro = new AnimatedGameObject(144,192,3,4,3,0);
    stuff.push_back(astro);
    stuff[1]->load(resource_path + "astro.png");
    stuff[1]->setPosition(150,400);

    GameObject* laser = new RegularGameObject();
    stuff.push_back(laser);
    stuff[2]->load(resource_path + "laser.png");
    stuff[2]->setPosition(100,300);
    stuff[2]->setScale(.1f);
*/
	if(!font.loadFromFile(resource_path + "oswald.ttf")){
		std::cout << "Font did not load" << std::endl;
	}
	if(!tfont.loadFromFile(resource_path + "timer.ttf")){
		std::cout << "Font did not load" << std::endl;
	}
	if(!block.loadFromFile(resource_path + "Blockt.ttf")){
		std::cout << "Font did not load" << std::endl;
	}

	pause_text = sf::Text("Cooldown",block,400);
	pause_text.setPosition(m_window.getSize().x/2 - 850,100);
	pause_text.setFillColor(sf::Color::Black);

	info = sf::Text("a short intermission",font,50);
	info.setPosition(pause_text.getPosition().x + 150,pause_text.getPosition().y + 500);
	info.setFillColor(sf::Color::Black);

	text = sf::Text("Rocket Score: 0",font,40);
	text2 = sf::Text("Robot Score: 0",font,40);
	timer = sf::Text("timer",tfont,60);
	timer.setPosition((m_window.getSize().x/2)-60,0);
	text.setPosition(10,0);
	text2.setPosition(m_window.getSize().x-text2.getGlobalBounds().width,0);
	text2.setFillColor(sf::Color::Green);
	timer.setFillColor(sf::Color::Black);
	//text.setCharacterSize(300);
	text.setFillColor(sf::Color::Blue);

	if(!sbuffer.loadFromFile(resource_path + "sword_miss.wav")) {
		std::cout << "sound did not load";
	}
	sword.setBuffer(sbuffer);
	if(!p2hitbuffer.loadFromFile(resource_path + "skeleton.wav")) {
		std::cout << "sound did not load";
	}
	p2hit.setBuffer(p2hitbuffer);
	if(!laserbuffer.loadFromFile(resource_path + "new_laser.wav")) {
		std::cout << "sound did not load";
	}
	laser.setBuffer(laserbuffer);
	if(!metalbuffer.loadFromFile(resource_path + "metal_hit.wav")) {
		std::cout << "sound did not load";
	}
	metal.setBuffer(metalbuffer);
	if(!speedbuffer.loadFromFile(resource_path + "SpeedUp.wav")) {
		std::cout << "sound did not load";
	}
	speed_up.setBuffer(speedbuffer);
	if(!slowbuffer.loadFromFile(resource_path + "SlowDown.wav")) {
		std::cout << "sound did not load";
	}
	slow_down.setBuffer(slowbuffer);
	if(!burnbuffer.loadFromFile(resource_path + "burn.wav")) {
		std::cout << "sound did not load";
	}
	burn.setBuffer(burnbuffer);
	if(!gongbuffer.loadFromFile(resource_path + "gong.wav")) {
		std::cout << "sound did not load";
	}
	gong.setBuffer(gongbuffer);
	laser.setVolume(60);

}

Game::Game(NetworkMode mode, std::shared_ptr<NetworkManager> netManager) :
		m_window(sf::VideoMode(1920, 1080), "Dungeon Game - " + 
			(mode == NetworkMode::HOST ? std::string("Host") : std::string("Client"))),
		m_networkMode(mode),
		m_networkManager(netManager)
{
	if(!background.openFromFile(resource_path + "background.wav")){
		std::cout << "your music is broken, go fix it" << std::endl;
	}

	//load the player
	m_player->load(resource_path + "rocket.png");
	m_player->setScale(2.0f);
	m_player->setPosition(m_player->getWidth(),400);

	player2->load(resource_path + "robot.png");
	player2->setScale(-1.0f,1.0f);
	player2->setPosition(m_window.getSize().x-(m_player->getWidth()+150),400);

	GameObject* wall = new RegularGameObject();
	stuff.push_back(wall);
	stuff[0]->load(resource_path + "brick.png");
	stuff[0]->setScale(2.0f);

	GameObject* fire = new AnimatedGameObject(216,216,5,3,10,0);
	fire->load(resource_path + "fire.png");
	fire->setScale(2.0f);
	fire->setPosition(-15,400);
	stuff.push_back(fire);

	GameObject* fire2 = new AnimatedGameObject(216,216,5,3,10,0);
	fire2->load(resource_path + "fire.png");
	fire2->setScale(2.0f);
	fire2->setPosition((1920/2)-5,400);
	stuff.push_back(fire2);

	GameObject* fire3 = new AnimatedGameObject(216,216,5,3,10,0);
	fire3->load(resource_path + "fire.png");
	fire3->setScale(2.0f);
	fire3->setPosition(1820,400);
	stuff.push_back(fire3);

	if(!font.loadFromFile(resource_path + "oswald.ttf")){
		std::cout << "Font did not load" << std::endl;
	}
	if(!tfont.loadFromFile(resource_path + "timer.ttf")){
		std::cout << "Font did not load" << std::endl;
	}
	if(!block.loadFromFile(resource_path + "Blockt.ttf")){
		std::cout << "Font did not load" << std::endl;
	}

	pause_text = sf::Text("Cooldown",block,400);
	pause_text.setPosition(m_window.getSize().x/2 - 850,100);
	pause_text.setFillColor(sf::Color::Black);

	info = sf::Text("a short intermission",font,50);
	info.setPosition(pause_text.getPosition().x + 150,pause_text.getPosition().y + 500);
	info.setFillColor(sf::Color::Black);

	text = sf::Text("Rocket Score: 0",font,40);
	text2 = sf::Text("Robot Score: 0",font,40);
	timer = sf::Text("timer",tfont,60);
	timer.setPosition((m_window.getSize().x/2)-60,0);
	text.setPosition(10,0);
	text2.setPosition(m_window.getSize().x-text2.getGlobalBounds().width,0);
	text2.setFillColor(sf::Color::Green);
	timer.setFillColor(sf::Color::Black);
	text.setFillColor(sf::Color::Blue);

	if(!sbuffer.loadFromFile(resource_path + "sword_miss.wav")) {
		std::cout << "sound did not load";
	}
	sword.setBuffer(sbuffer);
	if(!p2hitbuffer.loadFromFile(resource_path + "skeleton.wav")) {
		std::cout << "sound did not load";
	}
	p2hit.setBuffer(p2hitbuffer);
	if(!laserbuffer.loadFromFile(resource_path + "new_laser.wav")) {
		std::cout << "sound did not load";
	}
	laser.setBuffer(laserbuffer);
	if(!metalbuffer.loadFromFile(resource_path + "metal_hit.wav")) {
		std::cout << "sound did not load";
	}
	metal.setBuffer(metalbuffer);
	if(!speedbuffer.loadFromFile(resource_path + "SpeedUp.wav")) {
		std::cout << "sound did not load";
	}
	speed_up.setBuffer(speedbuffer);
	if(!slowbuffer.loadFromFile(resource_path + "SlowDown.wav")) {
		std::cout << "sound did not load";
	}
	slow_down.setBuffer(slowbuffer);
	if(!burnbuffer.loadFromFile(resource_path + "burn.wav")) {
		std::cout << "sound did not load";
	}
	burn.setBuffer(burnbuffer);
	if(!gongbuffer.loadFromFile(resource_path + "gong.wav")) {
		std::cout << "sound did not load";
	}
	gong.setBuffer(gongbuffer);
	laser.setVolume(60);
}

std::tuple<int,int,float,int,bool> Game::run() {
	//loop clock
	sf::Clock clock;
	//game time
	sf::Clock gTime;
	float time = 0;
	background.play();
	background.setLoop(true);
	background.setVolume(40);
	gong.setVolume(50);
	//gong.play();
	float apieceofcrap;
	int seconds;
	bool p1win = false;
	while (m_window.isOpen()) {
		char gClock [10];
		apieceofcrap = (int) (floor(gTime.getElapsedTime().asSeconds()/60));
		//apieceofcrap = apieceofcrap + 9;
		seconds = ((int) gTime.getElapsedTime().asSeconds())%60;
		std::snprintf(gClock, sizeof(gClock), "%02.0f:%02d", apieceofcrap, seconds);
		timer.setString(gClock);
		//std::cout << "time:  " << (gTime.getElapsedTime().asSeconds()) << std::endl;
		sf::Time deltaT = clock.restart();
		processEvents();
		
		// Handle network communication
		handleNetworkCommunication(gTime.getElapsedTime().asSeconds());
		
		if (!wait) {
			update(deltaT, time);
			temp_time = gTime.getElapsedTime().asSeconds() + 1.75;
		} else if (gTime.getElapsedTime().asSeconds() > temp_time){
			wait = false;
			gong.play();
		}
		if (time > .1f){
			time = 0;
		}
		time += deltaT.asSeconds();

		if (gTime.getElapsedTime().asSeconds() > p1_timeout){
			p1_time_passed = true;
		}

		if (gTime.getElapsedTime().asSeconds() > p2_timeout){
			p2_time_passed = true;
		}


        for(int x = 1;x<stuff.size();x++){
            if (collision(stuff[x],m_player) && p1_time_passed) {
				//std::cout << "i can be harmed " << collision(stuff[x],m_player) << "  " << p1_time_passed << std::endl;
				burn.play();
                p2points--;
                p1_time_passed = false;
				p1_timeout = gTime.getElapsedTime().asSeconds() + 2.2;
            } else {
                //std::cout << "i can't be harmed " << collision(stuff[x],m_player) << "  " << p1_time_passed << std::endl;
            }
            if (collision(stuff[x],player2) && p2_time_passed) {
                burn.play();
                points--;
                p2_time_passed = false;
				p2_timeout = gTime.getElapsedTime().asSeconds() + 2.2;
			}
        }

		if (points < 0)
			points = 0;
		if (p2points < 0)
			p2points = 0;

		//std::cout << points << std::endl;
		render();
	}
	background.stop();
	if (p2points > points)
		p1win = true;
	return {p2points,points,apieceofcrap,seconds,p1win};
}

void Game::processEvents() {
	sf::Event event;
	while (m_window.pollEvent(event)) {
		switch (event.type) {

			case sf::Event::KeyPressed:
				//handle key down here
				handlePlayerInput(event.key.code, true);
				break;
			case sf::Event::KeyReleased:
				handlePlayerInput(event.key.code, false);
				break;
			case sf::Event::Closed:
				m_window.close();
				break;
			default:
				break;
		}

	}
}

void Game::handlePlayerInput(sf::Keyboard::Key key, bool isDown) {
	// In network mode, host controls player 1, client controls player 2
	if (m_networkMode == NetworkMode::LOCAL || m_networkMode == NetworkMode::HOST) {
		// Player 1 controls (rocket - arrow keys)
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
	
	if (m_networkMode == NetworkMode::LOCAL || m_networkMode == NetworkMode::CLIENT) {
		// Player 2 controls (robot - WASD)
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
	
	// Common controls
	if (key == sf::Keyboard::O)
		o = !isDown;
	if (key == sf::Keyboard::P)
		p = !isDown;
	if (key == sf::Keyboard::Escape)
		m_window.close();
	if (key == sf::Keyboard::K) {
		if(wait) {
			wait = false;
			gong.play();
		}
	}
}

//use time since last update to get smooth movement
void Game::update(sf::Time deltaT,float time) {
	bool reset = false;
	//Look a vector class!
	sf::Vector2f p1movement(0.0f, 0.0f);
	sf::Vector2f p2movement(0.0f, 0.0f);

	if (m_up) {
		//std::cout << m_player->getPosition().y << "     " << -(m_player->getHeight()) << std::endl;
		if (m_player->getPosition().y < -(m_player->getHeight())) {
			m_player->setPosition(m_player->getPosition().x, (m_window.getSize().y) - m_player->getHeight());
		}



		if (collision(m_player,player2)){
			m_player->setPosition(m_player->getPosition().x,m_player->getPosition().y + 40);
			p2move = false;
		}
		else{
			p2move = true;
			p1movement.y -= m_speed;
		}
	}

	if (m_down) {
		if (m_player->getPosition().y > (m_window.getSize().y) - (m_player->getHeight())) {
			m_player->setPosition(m_player->getPosition().x, -(m_player->getHeight()));
		}



		if (collision(m_player,player2)){
			m_player->setPosition(m_player->getPosition().x,m_player->getPosition().y - 40);
			p2move = false;
		} else {
			p2move = true;
			p1movement.y += m_speed;
		}
	}if (m_left) {
		if (m_player->getPosition().x < -(m_player->getWidth())) {
			m_player->setPosition(m_window.getSize().x, m_player->getPosition().y);
		}
		if (!p1left){
			m_player->setScale(-2.0f,2);
			p1left = true;
			m_player->setPosition(m_player->getPosition().x-(m_player->getWidth()*m_player->getScale().x),m_player->getPosition().y);
		}

		if (collision(m_player,player2)){
			m_player->setPosition(m_player->getPosition().x + 40,m_player->getPosition().y);
			p2move = false;
		}else{
			p2move = true;
			p1movement.x -= m_speed;
		}
	}if (m_right) {
		m_player->setScale(2.0f, 2);

		if (m_player->getPosition().x > m_window.getSize().x) {
			m_player->setPosition(-(m_player->getWidth()), m_player->getPosition().y);
		}

		if (p1left) {
			p1left = false;
			m_player->setPosition(m_player->getPosition().x-(m_player->getWidth()*m_player->getScale().x),m_player->getPosition().y);
		}

		if (collision(m_player,player2)){
			m_player->setPosition(m_player->getPosition().x - 40,m_player->getPosition().y);
			p2move = false;
		} else {
			p2move = true;
			p1movement.x += m_speed;
		}
	}if (w) {
		if (player2->getPosition().y < -(player2->getHeight())) {
			player2->setPosition(player2->getPosition().x, (m_window.getSize().y) - player2->getHeight());
		}


		if (collision(m_player,player2)){
			player2->setPosition(player2->getPosition().x,player2->getPosition().y + 40);
		} else{
			p2movement.y -= m_speed;
		}

	}
	if (s) {
		if (player2->getPosition().y > (m_window.getSize().y) - (player2->getHeight())) {
			player2->setPosition(player2->getPosition().x, -(player2->getHeight()));
		}


		if (collision(m_player,player2)){
			player2->setPosition(player2->getPosition().x,player2->getPosition().y - 40);
		} else {
			p2movement.y += m_speed;
		}

	}
	if (a) {
		if (player2->getPosition().x < -(player2->getWidth())) {
			player2->setPosition(m_window.getSize().x, player2->getPosition().y);
		}
		player2->setScale(-1.0f,1.0f);
		if (!p2left) {
			p2left = true;
			player2->setPosition(player2->getPosition().x-(player2->getWidth()*player2->getScale().x),player2->getPosition().y);
		}
		//if (p2move)


		if (collision(m_player,player2)){
			player2->setPosition(player2->getPosition().x + 40,player2->getPosition().y);
		} else {
			p2movement.x -= m_speed;
		}

	}
	if (d) {
		player2->setScale(1.0f,1.0f);
		if (player2->getPosition().x > m_window.getSize().x) {
			player2->setPosition(-(player2->getWidth()), player2->getPosition().y);
		}
		if (p2left) {
			p2left = false;
			player2->setPosition(player2->getPosition().x-(player2->getWidth()*player2->getScale().x),player2->getPosition().y);
		}
		p2left = false;
		//if (p2move)


		if (collision(m_player,player2)){
			player2->setPosition(player2->getPosition().x - 40,player2->getPosition().y);
		} else {
			p2movement.x += m_speed;
		}

	}
	if (space) {
		sf::Rect<float> hit;
		if (!p2left) {
			hit = sf::Rect<float>(player2->getPosition(), sf::Vector2f(200 + ((player2->getWidth()) * (player2->getScale().x)),(player2->getHeight()) * (player2->getScale().y)));
		}else {
			hit = sf::Rect<float>(player2->getPosition(), sf::Vector2f(((player2->getWidth()) * (player2->getScale().x))-200, (player2->getHeight()) * (player2->getScale().y)));
		}

		if (collision(hit, m_player)) {
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
			hit = sf::Rect<float>(m_player->getPosition(), sf::Vector2f(200 + ((m_player->getWidth()) * (m_player->getScale().x)),(m_player->getHeight()) * (m_player->getScale().y)));
		}else {
			hit = sf::Rect<float>(m_player->getPosition(), sf::Vector2f(((m_player->getWidth()) * (m_player->getScale().x))-200, (m_player->getHeight()) * (m_player->getScale().y)));
		}

		if (collision(hit, player2)) {
			p2points++;
			reset = true;
			p2hit.play();
		} else {
			laser.play();
		}

		right = false;
	}
	if (o){
		m_speed = m_speed - 150.0f;
		if (m_speed < 0)
			m_speed = 1.0f;
		o = false;
		slow_down.play();
	}
	if (p) {
		m_speed = m_speed + 150.0f;
		p = false;
		speed_up.play();
	}

	if ((m_up || m_down || m_left || m_right) & !collision(m_player,player2)) {
		m_player->move(p1movement * deltaT.asSeconds());
		m_player->update(time);
	}

	if ((w || a || s || d) & !collision(m_player,player2)) {
		player2->move(p2movement * deltaT.asSeconds());
		player2->update(time);
	}
	//m_player->move(p1movement * deltaT.asSeconds());
	//m_player->update(time);
	for(int x = 0;x<stuff.size();x++){
		stuff[x]->update(time);
	}

	if(reset){
		//gong.play();
		//float temp_time = time;
		m_player->setScale(2.0f, 2);
		player2->setScale(-1.0f,1.0f);
		player2->setPosition(m_window.getSize().x-(m_player->getWidth()+150),400);
		m_player->setPosition(m_player->getWidth() + 20,400);
		p1left = false;
		p2left = true;
		wait = true;
		//points--;
		//p2points--;
	}

	//std::cout << deltaT.asSeconds() << "  " << p1movement.y << std::endl;
	text.setString("Rocket Score: " + std::to_string(p2points));
	text2.setString("Robot Score: " + std::to_string(points));
	text2.setPosition(m_window.getSize().x-text2.getGlobalBounds().width-20,0);
}

void Game::render() {
	m_window.clear();
	for (int x = 0; x < stuff.size(); x++) {
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
	m_window.display();
}

bool Game::collision(GameObject* a,GameObject* b) {
	sf::Rect<float> arect = sf::Rect<float>(a->getPosition(),sf::Vector2f(a->getWidth()*(a->getScale().x),a->getHeight()*(a->getScale().y)));
	sf::Rect<float> brect = sf::Rect<float>(b->getPosition(),sf::Vector2f(b->getWidth()*(b->getScale().x),b->getHeight()*(b->getScale().y)));
    //std::cout << (arect.intersects(brect)) << std::endl;
	return arect.intersects(brect);
}

bool Game::collision(sf::Rect<float> a,GameObject* b) {
	sf::Rect<float> brect = sf::Rect<float>(b->getPosition(),sf::Vector2f(b->getWidth()*(b->getScale().x),b->getHeight()*(b->getScale().y)));
	return a.intersects(brect);
}

void Game::handleNetworkCommunication(float gameTime) {
	if (!m_networkManager || !m_networkManager->isConnected()) {
		return;
	}

	if (m_networkMode == NetworkMode::HOST) {
		// Host: receive client input for player 2
		PlayerInput clientInput;
		if (m_networkManager->receiveInput(clientInput)) {
			// Apply client input to player 2 controls
			w = clientInput.up;
			s = clientInput.down;
			a = clientInput.left;
			d = clientInput.right;
			space = clientInput.attack;
		}

		// Send game state to client
		GameState state = captureGameState();
		m_networkManager->sendGameState(state);
	}
	else if (m_networkMode == NetworkMode::CLIENT) {
		// Client: send player 2 input to host
		PlayerInput myInput;
		myInput.up = w;
		myInput.down = s;
		myInput.left = a;
		myInput.right = d;
		myInput.attack = space;
		m_networkManager->sendInput(myInput);

		// Receive and apply game state from host
		GameState state;
		if (m_networkManager->receiveGameState(state)) {
			applyNetworkState(state);
		}
	}
}

GameState Game::captureGameState() const {
	GameState state;
	state.p1_x = m_player->getPosition().x;
	state.p1_y = m_player->getPosition().y;
	state.p2_x = player2->getPosition().x;
	state.p2_y = player2->getPosition().y;
	state.p1_score = p2points;
	state.p2_score = points;
	state.p1_left = p1left;
	state.p2_left = p2left;
	state.p1_scale_x = m_player->getScale().x;
	state.p1_scale_y = m_player->getScale().y;
	state.p2_scale_x = player2->getScale().x;
	state.p2_scale_y = player2->getScale().y;
	state.wait = wait;
	return state;
}

void Game::applyNetworkState(const GameState& state) {
	m_player->setPosition(state.p1_x, state.p1_y);
	player2->setPosition(state.p2_x, state.p2_y);
	p2points = state.p1_score;
	points = state.p2_score;
	p1left = state.p1_left;
	p2left = state.p2_left;
	m_player->setScale(state.p1_scale_x, state.p1_scale_y);
	player2->setScale(state.p2_scale_x, state.p2_scale_y);
	wait = state.wait;
}