#include <iostream>
#include "DiversityPlot.h"
#include <SFML/Graphics.hpp>

#include "Character.h"

int main() {
    const int FPS = 12;
    const int milliseconds_between_frames = 1000 / FPS;

    // create the window
    sf::RenderWindow window(sf::VideoMode(800, 600), "My window");

    sf::Texture texture;
    if (!texture.loadFromFile("textures/image.png")) {
        std::cout << "Error al cargar textura" << std::endl;
        return -1;
    }
    Character character;
    // run the program as long as the window is open
    sf::Clock clock;
    sf::Time deltaTime;
    sf::Time elapsed = sf::seconds(0);
    bool alive = true;
    while (window.isOpen())
    {
        deltaTime = clock.restart();
        elapsed+= deltaTime;

        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // clear the window with black color
        window.clear(sf::Color::Black);

        // draw everything here...
        // window.draw(...);

        //window.draw(sprite);
        character.update(deltaTime);
        if (elapsed < sf::seconds(1))
            character.executeAction(1);
        else if (alive)
        {
            alive = false;
            character.animatedSprite.setLooped(false);
            character.executeAction(3);
        } else if (!character.animatedSprite.isPlaying())
            character.animatedSprite.setFrame(6, false);
        window.draw(character);


        // end the current frame
        window.display();
    }

    return 0;
    runSimulation();
}