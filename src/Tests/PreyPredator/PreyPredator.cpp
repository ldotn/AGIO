#include <assert.h>
#include <algorithm>
#include <enumerate.h>
#include <matplotlibcpp.h>
#include <random>
#include <queue>

#include "../../Core/Config.h"
#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include "../../Serialization/SRegistry.h"
#include "../../Utils/Math/Float2.h"

#include "../Greedy/Greedy.h"

#include "PreyPredator.h"
#include "PublicInterfaceImpl.h"

namespace plt = matplotlibcpp;
using namespace std;
using namespace agio;
using namespace fpp;

#include "../../Utils/SFMLRenderer.h"

#include "neat.h"
void runSimulation() {
    minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

    Interface = new PublicInterfaceImpl();
    Interface->Init();

    // Create and fill the world
    WorldData world;
    world.fill(FoodCellCount, WorldSizeX, WorldSizeY);

    SRegistry registry;
    registry.load(SerializationFile);

//    // Create the individuals that are gonna be used in the simulation
//    // TODO: Select individuals with a criteria
    vector<BaseIndividual*> individuals;
    /*for(auto &entry : registry.Species)
        for(auto &individual: entry.Individuals)
            individuals.push_back(&individual);*/
	// Select randomly until the simulation size is filled
	for (int i = 0; i < SimulationSize; i++)
	{
		int species_idx = uniform_int_distribution<int>(0, registry.Species.size() - 1)(RNG);
		int individual_idx = uniform_int_distribution<int>(0, registry.Species[species_idx].Individuals.size() - 1)(RNG);
		
		SIndividual * org = new SIndividual;
		*org = registry.Species[species_idx].Individuals[individual_idx];
		org->InSimulation = true;
		individuals.push_back(org);
	}

	for(auto &individual : individuals)
		individual->State = Interface->MakeState(individual);

	for (auto& org : individuals)
		org->Reset();

	vector<int> food_x, food_y;
	food_x.resize(world.FoodPositions.size());
	food_y.resize(world.FoodPositions.size());

	SFMLRenderer renderer;

	// Load world sprites
	renderer.LoadSprite("ground", "../assets/ground_sprite.png");
	renderer.LoadSprite("food", "../assets/food_sprite.png");
	
	// Load bodies sprites
	renderer.LoadSprite("predator", "../assets/body0.png");
	renderer.LoadSprite("prey", "../assets/body1.png");

	sf::RenderWindow window(sf::VideoMode(WorldSizeX*32, WorldSizeY*32), "AGIO");

	vector<SFMLRenderer::Item> items_to_render;

	window.setFramerateLimit(5);

	while (window.isOpen())
	{
		// check all the window's events that were triggered since the last iteration of the loop
		sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window.close();
		}

		for (auto& org : individuals)
			org->DecideAndExecute(&world, individuals);

		items_to_render.resize(0);
		for (int y = 0; y < WorldSizeY; y++)
			for (int x = 0; x < WorldSizeX; x++)
				items_to_render.push_back({ {x,y}, renderer.GetSpriteID("ground") });

		// Plot food
		//plt::clf();
		for (auto[idx, pos] : enumerate(world.FoodPositions))
		{
			food_x[idx] = pos.x;
			food_y[idx] = pos.y;
			items_to_render.push_back({ {(int)pos.x,(int)pos.y}, renderer.GetSpriteID("food") });
		}
		//plt::plot(food_x, food_y, "gx");

		// Plot herbivores on blue and carnivores on black
		vector<int> herbivores_x, herbivores_y;
		vector<int> carnivores_x, carnivores_y;
		herbivores_x.reserve(PopulationSize);
		herbivores_y.reserve(PopulationSize);
		carnivores_x.reserve(PopulationSize);
		carnivores_y.reserve(PopulationSize);

		for (const auto& org : individuals)
		{
			auto state_ptr = (OrgState*)org->GetState();
			auto pos = state_ptr->Position;

			if (state_ptr->IsCarnivore)
			{
				carnivores_x.push_back(state_ptr->Position.x);
				carnivores_y.push_back(state_ptr->Position.y);
				items_to_render.push_back({ {(int)pos.x,(int)pos.y}, renderer.GetSpriteID("predator") });

			}
			else
			{
				herbivores_x.push_back(state_ptr->Position.x);
				herbivores_y.push_back(state_ptr->Position.y);
				items_to_render.push_back({ {(int)pos.x,(int)pos.y}, renderer.GetSpriteID("prey") });

			}
		}
		
		//plt::plot(herbivores_x, herbivores_y, "ob");
		//plt::plot(carnivores_x, carnivores_y, "ok");

		//plt::xlim(-1, WorldSizeX);
		//plt::ylim(-1, WorldSizeY);
		//plt::pause(0.1);
		renderer.Render(window, items_to_render);
	}

	for (auto org : individuals)
		delete org;
	// NOTE! : Explictely leaking the interface here!
	// There's a good reason for it
	// The objects in the system need the interface to delete themselves
	//  so I can't delete it, as I don't know the order in which destructors are called
	// In any case, this is not a problem, because on program exit the SO should (will?) release the memory anyway
	// delete Interface;
}