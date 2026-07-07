#pragma once
#include "GameObject.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <stdexcept>
#include <string>

// Load helpers that throw std::runtime_error on failure instead of silently continuing.
// sf::Music streams from disk so uses openFromFile; others use loadFromFile / .load().

inline void loadOrThrow(sf::Font& f, const std::string& path) {
    if (!f.loadFromFile(path))
        throw std::runtime_error("failed to load asset: " + path);
}

inline void loadOrThrow(sf::SoundBuffer& b, const std::string& path) {
    if (!b.loadFromFile(path))
        throw std::runtime_error("failed to load asset: " + path);
}

inline void loadOrThrow(sf::Music& m, const std::string& path) {
    if (!m.openFromFile(path))
        throw std::runtime_error("failed to load asset: " + path);
}

inline void loadOrThrow(GameObject& obj, const std::string& path) {
    if (!obj.load(path))
        throw std::runtime_error("failed to load asset: " + path);
}
