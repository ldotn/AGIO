#include "Character.h"

#include <string>

Character::Character()
{
    auto texture = new sf::Texture();
    std::string file_src = "textures/image.png";
    if (!texture->loadFromFile(file_src)) {
        throw "File \"" + file_src + "\" not found.";
    }

    animations.resize((int) AnimationsId::ANIMATIONS_AMOUNT);

    Animation moveLeftAnimation;
    for (int i = 0; i < 8; i++) moveLeftAnimation.addFrame(sf::IntRect(i * 32, 6 * 32, 32, 32));
    moveLeftAnimation.setSpriteSheet(*texture);
    animations[(int) AnimationsId::MOVE_LEFT] = moveLeftAnimation;


    Animation moveRightAnimation;
    for (int i = 0; i < 8; i++) moveRightAnimation.addFrame(sf::IntRect(i * 32, 1 * 32, 32, 32));
    moveRightAnimation.setSpriteSheet(*texture);
    animations[(int) AnimationsId::MOVE_RIGHT] = moveRightAnimation;

    Animation attackAnimation;
    for (int i = 0; i < 7; i++) attackAnimation.addFrame(sf::IntRect(i * 32, 2 * 32, 32, 32));
    attackAnimation.setSpriteSheet(*texture);
    animations[(int) AnimationsId::ATTACK] = attackAnimation;

    Animation deadAnimation;
    for (int i = 0; i < 7; i++) deadAnimation.addFrame(sf::IntRect(i * 32, 4 * 32, 32, 32));
    deadAnimation.setSpriteSheet(*texture);
    animations[(int) AnimationsId::DEAD] = deadAnimation;

}

void Character::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    target.draw(animatedSprite, states);
}

void Character::update(sf::Time deltaTime)
{
    animatedSprite.update(deltaTime);
}

void Character::executeAction(int action)
{
    animatedSprite.play(animations[action]);
}