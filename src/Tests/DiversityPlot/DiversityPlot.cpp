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

#include "DiversityPlot.h"
#include "PublicInterfaceImpl.h"

//#include "Greedy/GreedyPrey.h"

namespace plt = matplotlibcpp;
using namespace std;
using namespace agio;
using namespace fpp;


#include "neat.h"
void runSimulation() {
    minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

    Interface = new PublicInterfaceImpl();
    Interface->Init();

    // Create and fill the world
    WorldData world = createWorld();

    SRegistry registry;
    registry.load(SerializationFile);

    // Create the individuals that are gonna be used in the simulation
    // TODO: Select individuals with a criteria
    vector<BaseIndividual*> individuals;
    //individuals.push_back(new GreedyPrey());
    for(auto &entry : registry.Species)
        for(auto &individual: entry.Individuals)
            individuals.push_back(&individual);

	//for(auto &individual : individuals)
		//individual->State = Interface->MakeState(individual);

	for (auto& org : individuals)
		org->Reset();

	vector<int> food_x, food_y;
	food_x.resize(world.FoodPositions.size());
	food_y.resize(world.FoodPositions.size());

	while (true)
	{
		for (auto& org : individuals)
			org->DecideAndExecute(&world, individuals);

		// Plot food
		plt::clf();
		for (auto[idx, pos] : enumerate(world.FoodPositions))
		{
			food_x[idx] = pos.x;
			food_y[idx] = pos.y;
		}
		plt::plot(food_x, food_y, "gx");

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

			if (true) //state_ptr->IsCarnivore)
			{
				carnivores_x.push_back(state_ptr->Position.x);
				carnivores_y.push_back(state_ptr->Position.y);
			}
			else
			{
				herbivores_x.push_back(state_ptr->Position.x);
				herbivores_y.push_back(state_ptr->Position.y);
			}
		}
		plt::plot(herbivores_x, herbivores_y, "ob");
		plt::plot(carnivores_x, carnivores_y, "ok");

		plt::xlim(-1, WorldSizeX);
		plt::ylim(-1, WorldSizeY);
		plt::pause(0.1);
	}

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
    ifstream file("world.txt");
    int i = 0;
    while(!file.eof())
    {
        file >> line;
        if (line == "")
            throw "Empty line in world.txt";
        vector<CellType> cellTypes;
        for (char &c : line)
        {
            CellType type;
            if (c == '0')
                type = CellType::Grass;
            else if (c == '1')
                type = CellType::Grass;//Water;
            else if (c == '2')
                type = CellType::Grass;//Wall;
            else
                throw "Undefined cell type";
            cellTypes.push_back(type);
        }
        world.CellTypes.push_back(cellTypes);
        i++;
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