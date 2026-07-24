

#include "AnimatedGameObject.h"
#include "sprite_anim.h"
#include <cmath>
#include <iostream>

AnimatedGameObject::AnimatedGameObject() {
    xsize = 10;
    ysize = 10;
    howmanyx = 1;
    howmanyy = 1;
    howmany = 1;
    curr = 1;
}

AnimatedGameObject::AnimatedGameObject(double x, double y, int nx, int ny, int n, int angle) {
    m_angle = angle;
    xsize = x;
    ysize = y;
    howmanyx = nx;
    howmanyy = ny;
    howmany = n;
    curr = 1;
}

bool AnimatedGameObject::load(const std::string& filename) {
    if (m_texture.loadFromFile(filename)) {
        m_filename = filename;
        m_sprite.emplace(m_texture);
        m_sprite->setRotation(sf::degrees(static_cast<float>(m_angle)));
        rect = sf::IntRect({0, 0}, {static_cast<int>(floor(xsize / (double)howmanyx)),
                                    static_cast<int>(floor(ysize / (double)howmanyy))});
        m_sprite->setTextureRect(rect);
        m_valid = true;
        return true;
    }
    return false;
}

void AnimatedGameObject::update(float Tsec) {

    bool work = false;
    if (Tsec > .1f || Tsec == 0) {
        work = true;
    }
    if (work) {
        auto res = advanceFrameRect(rect, curr, howmanyx, howmanyy, howmany, xsize, ysize);
        rect = res.rect;
        curr = res.curr;
        m_sprite->setTextureRect(rect);
    }
}

void AnimatedGameObject::draw(sf::RenderWindow& window) {
    if (m_valid) {
        window.draw(*m_sprite);
    }
}

void AnimatedGameObject::move(sf::Vector2f loc) {
    if (m_valid)
        m_sprite->move(loc);
}

void AnimatedGameObject::setPosition(float x, float y) {
    if (m_valid)
        m_sprite->setPosition({x, y});
}

sf::Vector2f AnimatedGameObject::getPosition() const {
    if (m_valid)
        return m_sprite->getPosition();
    else
        return sf::Vector2f(0, 0);
}

float AnimatedGameObject::getHeight() const {
    return m_sprite ? m_sprite->getLocalBounds().size.y : 0.f;
}

float AnimatedGameObject::getWidth() const {
    return m_sprite ? m_sprite->getLocalBounds().size.x : 0.f;
}

void AnimatedGameObject::setScale(float scale) {
    if (m_valid)
        m_sprite->setScale({scale, scale});
}

void AnimatedGameObject::setScale(float x, float y) {
    if (m_valid)
        m_sprite->setScale({x, y});
}

sf::Vector2f AnimatedGameObject::getScale() const {
    if (m_valid)
        return m_sprite->getScale();
    else
        return sf::Vector2f(0, 0);
}

void AnimatedGameObject::changeValid(bool a) { m_valid = a; }

bool AnimatedGameObject::isValid() { return m_valid; }

sf::FloatRect AnimatedGameObject::getGlobalBounds() const {
    return m_sprite ? m_sprite->getGlobalBounds() : sf::FloatRect();
}

void AnimatedGameObject::setOrigin() {
    if (!m_sprite)
        return;
    if (m_sprite->getOrigin().x == 0) {
        m_sprite->setOrigin(
            {(m_sprite->getLocalBounds().size.x) / 2, (m_sprite->getLocalBounds().size.y) / 2});
    } else {
        m_sprite->setOrigin({0, 0});
    }
}
