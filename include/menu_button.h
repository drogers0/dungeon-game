#pragma once
#include <SFML/Graphics.hpp>
#include <string>

struct MenuButton {
    sf::RectangleShape shape;
    sf::Text label;

    // Constructs button sized to `size`; call setPosition() before drawing.
    MenuButton(sf::Vector2f size, const sf::Font& font, const std::string& text,
               unsigned charSize = 34);

    void setPosition(sf::Vector2f centre); // centres shape and label
    void setHovered(bool hovered);         // switches fill colour
    sf::FloatRect bounds() const;          // == shape.getGlobalBounds()
    void draw(sf::RenderWindow& w) const;
};
