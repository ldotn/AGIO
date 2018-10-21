#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include <assert.h>
#include "../../Utils/Math/Float2.h"
#include <algorithm>
#include <random>
#include <matplotlibcpp.h>
#include <enumerate.h>
#include <queue>
#include "../../Core/Config.h"
#include "Config.h"
#include "PublicInterfaceImpl.h"
#include "../../Serialization/SRegistry.h"

namespace plt = matplotlibcpp;
using namespace std;
using namespace agio;
using namespace fpp;


#include "neat.h"
int runEvolution() {
    minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

    // Create and fill the world
    WorldData world;
    world.FoodPositions.resize(FoodCellCount);
    for (auto &pos : world.FoodPositions)
    {
        pos.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
        pos.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
    }

    SRegistry registry;
    registry.load("serialization.txt");

    // Create the individuals that are gonna be used in the simulation
    vector<SIndividual> individuals;
    for(auto &entry : registry.Species)
        for(auto &individual: entry.Individuals)
            individuals.push_back(individual);

	for(auto &individual : individuals)
		individual.state = Interface->MakeState(individual);

    while (true)
	{
		for (auto& org : individuals)
			org.Reset();

		vector<int> food_x, food_y;
		food_x.resize(world.FoodPositions.size());
		food_y.resize(world.FoodPositions.size());

		bool any_alive = true;
		//vector<int> org_x, org_y;
		//org_x.reserve(PopulationSize);
		//org_y.reserve(PopulationSize);

		while (any_alive)
		{
			for (auto[idx, pos] : enumerate(world.FoodPositions))
			{
				food_x[idx] = pos.x;
				food_y[idx] = pos.y;
			}

			any_alive = false;
			for (auto& org : individuals)
				org.DecideAndExecute(&world, &pop);

			// Plot food
			plt::clf();
			plt::plot(food_x, food_y, "gx");

			// Plot the individuals with different colors per species
			/*for (auto [species_idx,entry] : enumerate(pop.GetSpecies()))
			{
			auto [_, species] = entry;
			org_x.resize(species.IndividualsIDs.size());
			org_y.resize(species.IndividualsIDs.size());

			for (auto [idx, org_idx] : enumerate(species.IndividualsIDs))
			{
			auto state_ptr = (OrgState*)pop.GetIndividuals()[org_idx].GetState();
			if (state_ptr->Life)
			{
			any_alive = true;
			org_x[idx] = state_ptr->Position.x;
			org_y[idx] = state_ptr->Position.y;
			}
			}

			plt::plot(org_x, org_y, string("x") + string("C") + to_string(species_idx));
			}*/

			// Plot herbivores on blue and carnivores on black
			vector<int> herbivores_x, herbivores_y;
			vector<int> carnivores_x, carnivores_y;
			herbivores_x.reserve(PopulationSize);
			herbivores_y.reserve(PopulationSize);
			carnivores_x.reserve(PopulationSize);
			carnivores_y.reserve(PopulationSize);

			for (const auto& org : individuals)
			{
				auto state_ptr = (OrgState*)org.GetState();

				any_alive = true;
				if (state_ptr->IsCarnivore)
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
	}
	// NOTE! : Explictely leaking the interface here!
	// There's a good reason for it
	// The objects in the system need the interface to delete themselves
	//  so I can't delete it, as I don't know the order in which destructors are called
	// In any case, this is not a problem, because on program exit the SO should (will?) release the memory anyway
	//delete Interface;
}