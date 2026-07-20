#pragma once
#include <SFML/Graphics.hpp>
#include <string>

// LCOV_EXCL_START — requires a live RenderWindow; not testable headless
inline void captureScreenshot(const sf::RenderWindow& w, const std::string& path) {
    sf::Texture t;
    t.create(w.getSize().x, w.getSize().y);
    t.update(w);
    t.copyToImage().saveToFile(path);
}
// LCOV_EXCL_STOP
