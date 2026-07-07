//
// Created by Rohan Malik on 12/4/18.
//

#pragma once
#include "GameObject.h"

class AnimatedGameObject : public GameObject {
public:

    AnimatedGameObject();

    AnimatedGameObject(double x,double y,int nx,int ny,int howmany,int angle);

    bool load(const std::string& filename) override;

    void draw(sf::RenderWindow& window) override;

    void update(float deltaT) override;

    void setPosition(float x, float y) override;

    void move(sf::Vector2f) override;

    sf::Vector2f getPosition() const override;

    float getHeight() const override;

    float getWidth() const override;

    void setScale(float scale) override;

    void setScale(float x, float y) override;

    sf::Vector2f getScale() const override;

    void changeValid(bool a) override;

    bool isValid() override;

    void setOrigin() override;

private:
    sf::Sprite m_sprite;
    sf::Texture m_texture;
    std::string m_filename;
    bool m_valid = false;
    double xsize;
    double ysize;
    int howmanyx;
    int howmanyy;
    int howmany;
    int curr;
    sf::IntRect rect;
};

