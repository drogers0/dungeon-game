#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

// LCOV_EXCL_START — requires a live RenderWindow; not testable headless
inline void captureScreenshot(const sf::RenderWindow& w, const std::string& path) {
    sf::Texture t(w.getSize());
    t.update(w);
    if (!t.copyToImage().saveToFile(path))
        std::cerr << "captureScreenshot: failed to save " << path << "\n";
}
// LCOV_EXCL_STOP
