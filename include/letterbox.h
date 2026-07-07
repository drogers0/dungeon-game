#pragma once
#include <SFML/Graphics.hpp>

// Compute an aspect-preserving centered sf::View that maps logicalSize onto
// windowPx screen pixels.  The view's rectangle covers [0,0]×logicalSize;
// the viewport is the pillar- or letter-boxed sub-rect of the window.
// Pass this to RenderWindow::setView() after window creation and on
// sf::Event::Resized.
inline sf::View makeLetterboxView(sf::Vector2f logicalSize, sf::Vector2u windowPx) {
    float logicalAspect = logicalSize.x / logicalSize.y;
    float windowAspect = static_cast<float>(windowPx.x) / static_cast<float>(windowPx.y);

    sf::FloatRect viewport;
    if (windowAspect >= logicalAspect) {
        // Window is wider than logical canvas → pillarbox (black bars on sides).
        float vpW = logicalAspect / windowAspect;
        viewport = sf::FloatRect((1.f - vpW) / 2.f, 0.f, vpW, 1.f);
    } else {
        // Window is taller than logical canvas → letterbox (black bars top/bottom).
        float vpH = windowAspect / logicalAspect;
        viewport = sf::FloatRect(0.f, (1.f - vpH) / 2.f, 1.f, vpH);
    }

    sf::View view(sf::FloatRect(0.f, 0.f, logicalSize.x, logicalSize.y));
    view.setViewport(viewport);
    return view;
}
