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
