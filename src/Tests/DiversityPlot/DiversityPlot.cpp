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
#include "../../Utils/ConsoleRenderer.h"

#include "../rlutil-master/rlutil.h"
//#include "Greedy/GreedyPrey.h"

namespace plt = matplotlibcpp;
using namespace std;
using namespace agio;
using namespace fpp;


#include "neat.h"
void runSimulation() 
{
	minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

	Interface = new PublicInterfaceImpl();
	Interface->Init();

	// Create and fill the world
	WorldData world = createWorld();

	SRegistry registry;
	registry.load(SerializationFile);

	//    // Create the individuals that are gonna be used in the simulation
	//    // TODO: Select individuals with a criteria
	vector<vector<int>> species_ids;
	vector<BaseIndividual*> individuals;

	// Select randomly until the simulation size is filled
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
	}

	for (auto &individual : individuals)
		individual->State = Interface->MakeState(individual);

	for (auto& org : individuals)
		org->Reset();

	rlutil::cls();

	ConsoleRenderer renderer;

	while (true)
	{
		set<ConsoleItem> items_to_render;

		for (auto& org : individuals)
			if (org->GetState<OrgState>()->Life > 0)
				org->DecideAndExecute(&world, individuals);

		// Plot food
		for (auto pos : world.FoodPositions)
			items_to_render.insert({(int)pos.x,(int)pos.y,'x',rlutil::GREEN });

		// Plot each species in a different color
		for (const auto& id_vec : species_ids)
		{
			for (auto [idx, id] : enumerate(id_vec))
			{
				auto state_ptr = (OrgState*)individuals[id]->GetState();

				char symbol = 'O';
				if (state_ptr->Life <= 0)
					symbol = '@';

				items_to_render.insert({ (int)state_ptr->Position.x,(int)state_ptr->Position.y,symbol,idx % 15 + 1 });
				//pos_x[idx] = state_ptr->Position.x;
				//pos_y[idx] = state_ptr->Position.y;
			}
		}

		//plt::xlim(-1, WorldSizeX);
		//plt::ylim(-1, WorldSizeY);
		//plt::pause(0.1);
		renderer.Render(items_to_render);
		this_thread::sleep_for(16ms);
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
WorldData createWorld() {
    WorldData world;

    // Load cell types
    string line;
    ifstream file("../src/Tests/DiversityPlot/world.txt");
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

    // Fill with food
    minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());
    world.FoodPositions.resize(FoodCellCount);
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