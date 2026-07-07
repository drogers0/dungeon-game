#pragma once
#include "GameObject.h"
#include <SFML/Graphics.hpp>

// Build the axis-aligned bounding rect for a game object using its position,
// width, height, and scale.  Note: scale.x may be negative (P2 default -1),
// which produces a negative-width rect; SFML intersects() normalises it, and
// that normalisation is load-bearing for hit detection — do not "fix" it.
inline sf::FloatRect objectBounds(const GameObject& obj) {
    return sf::FloatRect(obj.getPosition(), sf::Vector2f(obj.getWidth() * obj.getScale().x,
                                                         obj.getHeight() * obj.getScale().y));
}

// Same rect as objectBounds() but normalised to positive width/height (a
// left-facing object has scale.x = -1, so objectBounds() yields a negative
// width).  Use this wherever edge arithmetic needs a canonical left/top and
// positive extent — e.g. the AI's nearest-edge distance math.  Do NOT use it
// for collision: intersects() relies on the raw negative-width form.
inline sf::FloatRect normalizedBounds(const GameObject& obj) {
    sf::FloatRect r = objectBounds(obj);
    if (r.width < 0.f) {
        r.left += r.width;
        r.width = -r.width;
    }
    if (r.height < 0.f) {
        r.top += r.height;
        r.height = -r.height;
    }
    return r;
}
