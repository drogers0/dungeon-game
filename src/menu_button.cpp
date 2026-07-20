#include "menu_button.h"

static const sf::Color kNormalFill(50, 50, 60, 210);
static const sf::Color kHoverFill(90, 90, 110, 230);
static const sf::Color kOutlineColor(180, 180, 200, 180);

MenuButton::MenuButton(sf::Vector2f size, const sf::Font& font, const std::string& text,
                       unsigned charSize)
    : label(text, font, charSize) {
    shape.setSize(size);
    shape.setOrigin(size.x / 2.f, size.y / 2.f);
    shape.setFillColor(kNormalFill);
    shape.setOutlineColor(kOutlineColor);
    shape.setOutlineThickness(1.5f);
}

void MenuButton::setPosition(sf::Vector2f centre) {
    shape.setPosition(centre);
    auto lb = label.getLocalBounds();
    label.setOrigin(lb.left + lb.width / 2.f, lb.top + lb.height / 2.f);
    label.setPosition(centre);
}

void MenuButton::setHovered(bool hovered) {
    shape.setFillColor(hovered ? kHoverFill : kNormalFill);
}

sf::FloatRect MenuButton::bounds() const { return shape.getGlobalBounds(); }

void MenuButton::draw(sf::RenderWindow& w) const {
    w.draw(shape);
    w.draw(label);
}
