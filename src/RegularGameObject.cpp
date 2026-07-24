//
// Created by bswenson3 on 11/9/16.
//

#include "RegularGameObject.h"

RegularGameObject::RegularGameObject() {}

bool RegularGameObject::load(const std::string& filename) {
    if (m_texture.loadFromFile(filename)) {
        m_filename = filename;
        m_sprite.emplace(m_texture);
        m_valid = true;
        return true;
    }
    return false;
}

void RegularGameObject::update(float /*deltaT*/) {}

void RegularGameObject::draw(sf::RenderWindow& window) {
    if (m_valid)
        window.draw(*m_sprite);
}

void RegularGameObject::move(sf::Vector2f loc) {
    if (m_valid)
        m_sprite->move(loc);
}

void RegularGameObject::setPosition(float x, float y) {
    if (m_valid)
        m_sprite->setPosition({x, y});
}

sf::Vector2f RegularGameObject::getPosition() const {
    if (m_valid)
        return m_sprite->getPosition();
    else
        return sf::Vector2f(0, 0);
}

float RegularGameObject::getHeight() const {
    return m_sprite ? m_sprite->getLocalBounds().size.y : 0.f;
}

float RegularGameObject::getWidth() const {
    return m_sprite ? m_sprite->getLocalBounds().size.x : 0.f;
}

void RegularGameObject::setScale(float scale) {
    if (m_valid)
        m_sprite->setScale({scale, scale});
}

void RegularGameObject::setScale(float x, float y) {
    if (m_valid)
        m_sprite->setScale({x, y});
}

sf::Vector2f RegularGameObject::getScale() const {
    if (m_valid)
        return m_sprite->getScale();
    else
        return sf::Vector2f(0, 0);
}

void RegularGameObject::changeValid(bool a) {
    m_valid = a;
    // SFML 3's sf::Sprite has no default ctor, so m_sprite is empty until load().
    // Marking an unloaded object valid must still give the mutators (guarded on
    // m_valid) a live sprite to act on — emplace an untextured sprite backed by the
    // default m_texture, matching SFML 2's always-present default sprite.
    if (a && !m_sprite)
        m_sprite.emplace(m_texture);
}

bool RegularGameObject::isValid() { return m_valid; }

sf::FloatRect RegularGameObject::getGlobalBounds() const {
    return m_sprite ? m_sprite->getGlobalBounds() : sf::FloatRect();
}

void RegularGameObject::setOrigin() {
    if (!m_sprite)
        return;
    m_sprite->setOrigin(
        {(m_sprite->getLocalBounds().size.x) / 2, (m_sprite->getLocalBounds().size.y) / 2});
}
