#define _HAS_STD_BYTE 0 // needed for compatibility reasons with rlutil
#include <assert.h>
#include <algorithm>
#include <enumerate.h>
#include <matplotlibcpp.h>
#include <random>
#include <queue>
#include <thread>

#include "../../Core/Config.h"
#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include "../../Serialization/SRegistry.h"
#include "../../Utils/Math/Float2.h"

#include "DiversityPlot.h"
#include "PublicInterfaceImpl.h"
#include "zip.h"
//#include "../../Utils/ConsoleRenderer.h"
#include "../../Utils/SFMLRenderer.h"

//#include "../rlutil-master/rlutil.h"
//#include "Greedy/GreedyPrey.h"

namespace plt = matplotlibcpp;
using namespace std;
using namespace agio;
using namespace fpp;


#include "neat.h"
void runSimulation(const string& WorldPath) 
{
	minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());
	//minstd_rand RNG(42);

	Interface = new PublicInterfaceImpl();
	Interface->Init();

	// Create and fill the world
	WorldData world = createWorld(WorldPath);

	SRegistry registry;
	registry.load(SerializationFile);

	//    // Create the individuals that are gonna be used in the simulation
	//    // TODO: Select individuals with a criteria
	vector<vector<int>> species_ids;
	vector<BaseIndividual*> individuals;

	// Select randomly until the simulation size is filled
	// Also create a map of random colors for each species
	vector<sf::Color> species_colors(registry.Species.size());
	species_ids.resize(registry.Species.size());
	for (int i = 0; i < SimulationSize; i++)
	{
		int species_idx = uniform_int_distribution<int>(0, registry.Species.size() - 1)(RNG);
		int individual_idx = uniform_int_distribution<int>(0, registry.Species[species_idx].Individuals.size() - 1)(RNG);

		SIndividual * org = new SIndividual;
		*org = registry.Species[species_idx].Individuals[individual_idx];
		org->InSimulation = true;
		individuals.push_back(org);
		species_ids[species_idx].push_back(individuals.size() - 1);

		auto cdist = uniform_int_distribution<int>(0, 255);
		species_colors[species_idx] = sf::Color(cdist(RNG), cdist(RNG), cdist(RNG));
	}

	for (auto &individual : individuals)
		individual->State = Interface->MakeState(individual);

	for (auto& org : individuals)
		org->Reset();

	//rlutil::cls();

	//ConsoleRenderer renderer;
	SFMLRenderer renderer;

	// Load world sprites
	renderer.LoadSprite("ground", "../assets/ground_sprite.png");
	renderer.LoadSprite("wall", "../assets/wall_sprite.png");
	renderer.LoadSprite("water", "../assets/water_sprite.png");
	renderer.LoadSprite("food", "../assets/food_sprite.png");

	// Load bodies sprites
	// Use one body type based on what they can eat
	// The body is white and black, each species tints it to a different color
	renderer.LoadSprite("carnivore", "../assets/carnivore.png");
	renderer.LoadSprite("herbivore", "../assets/herbivore.png");
	renderer.LoadSprite("omnivore", "../assets/omnivore.png");


	sf::RenderWindow window(sf::VideoMode(1600, 1024), "AGIO");

	vector<SFMLRenderer::Item> items_to_render;

	window.setFramerateLimit(5);

	// run the program as long as the window is open
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

		items_to_render.resize(0);

		for (int y = 0; y < world.CellTypes.size(); y++)
		{
			const auto& row = world.CellTypes[y];
			for (int x = 0; x < row.size(); x++)
			{
				switch (row[x])
				{
				case CellType::Ground:
					items_to_render.push_back({ {x,y}, renderer.GetSpriteID("ground") });
					break;
				case CellType::Wall:
					items_to_render.push_back({ {x,y}, renderer.GetSpriteID("wall") });
					break;
				case CellType::Water:
					items_to_render.push_back({ {x,y}, renderer.GetSpriteID("water") });
					break;
				}
			}
		}

		for (auto& org : individuals)
			if (org->GetState<OrgState>()->Life > 0) // TODO : Create a new one of a different species
				org->DecideAndExecute(&world, individuals);

		// Plot food
		for (auto pos : world.FoodPositions)
			items_to_render.push_back({ {(int)pos.x,(int)pos.y}, renderer.GetSpriteID("food") });

		// Plot each species in a different color
		for (auto [idx, id_vec] : enumerate(species_ids))
		{
			for (int id : id_vec)
			{
				auto state_ptr = (OrgState*)individuals[id]->GetState();

				// Could add a random (per individual, same each frame) rotation so you can know if there are several one over the other
				int sprite_id = -1;//renderer.GetSpriteID("carnivore");//renderer.GetSpriteID(bodies[idx % size(bodies)]);
				if (state_ptr->Type == OrgType::Omnivore)
					sprite_id = renderer.GetSpriteID("omnivore");
				else if (state_ptr->Type == OrgType::Carnivore)
					sprite_id = renderer.GetSpriteID("carnivore");
				else if (state_ptr->Type == OrgType::Herbivore)
					sprite_id = renderer.GetSpriteID("herbivore");
			
				items_to_render.push_back({ {(int)state_ptr->Position.x,(int)state_ptr->Position.y}, sprite_id, species_colors[idx] });
			}
		}

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

#include <fstream>
#include <string>
WorldData createWorld(const string& FilePath) {
    WorldData world;

    // Load cell types
    string line;
    ifstream file(FilePath);
	if (!file.is_open())
		throw invalid_argument("World file not found");

    while(!file.eof())
    {
        file >> line;
		if (line == "")
			continue;

        vector<CellType> cellTypes;
        for (char &c : line)
        {
            CellType type;
			if (c == '0')
				type = CellType::Ground;
			else if (c == '1')
				type = CellType::Water;
			else if (c == '2')
				type = CellType::Wall;
			else
				continue;
            cellTypes.push_back(type);
        }

        world.CellTypes.push_back(cellTypes);
    }

	WorldSizeY = world.CellTypes.size();
	WorldSizeX = world.CellTypes[0].size();

    // Fill with food
    minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());
    world.FoodPositions.resize(WorldSizeX * WorldSizeY*FoodCellProportion);
    for (auto &pos : world.FoodPositions)
    {
        bool valid_position = false;
        while(!valid_position)
        {
            pos.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
            pos.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);

            valid_position = world.CellTypes[pos.y][pos.x] != CellType::Wall;
        }
    }

    return world;
}