#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System/Time.hpp>
#include "AnimatedSprite.h"
#include <vector>
#include "Animation.h"

enum class AnimationsId {
    MOVE_LEFT,
    MOVE_RIGHT,
    ATTACK,
    DEAD,
    ANIMATIONS_AMOUNT
};

class Character : public sf::Drawable
{
public:
    std::vector<Animation> animations;
    AnimatedSprite animatedSprite = AnimatedSprite(sf::seconds(0.1f), false, true);

    Character();

    void executeAction(int action);
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
    void update(sf::Time deltaTime);


};