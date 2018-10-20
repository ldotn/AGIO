#include "Interface.h"
#include <matplotlibcpp.h>
#include "neat.h"
#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include "../../Core/Config.h"
#include <iostream>
#include "zip.h"
#include "enumerate.h"

namespace plt = matplotlibcpp;
using namespace agio;
using namespace std;
using namespace fpp;

// Does the evolution loop and saves the evolved networks to a file
int main()
{
	// Values hardcoded from the UE4 level
	// TODO : Find a better way to set the values
	GameplayParams::WalkSpeed = 75.0f;
	GameplayParams::RunSpeed = 175.0f;
	GameplayParams::GameArea = { {-2840,-8000}, {3200,2900} };
	GameplayParams::TurnRadians = 22.5f * (3.14159f / 180.0f);
	GameplayParams::EatDistance = 35.0f;
	GameplayParams::WastedActionPenalty = 10;
	GameplayParams::CorpseBitesDuration = 5;
	GameplayParams::EatCorpseLifeGained = 10;
	GameplayParams::EatPlantLifeGained = 20;
	GameplayParams::AttackDamage = 40;
	GameplayParams::LifeLostPerTurn = 5;
	GameplayParams::StartingLife = 100;

	NEAT::load_neat_params("../cfg/neat_test_config.txt");
	NEAT::mutate_morph_param_prob = Settings::ParameterMutationProb;
	NEAT::destructive_mutate_morph_param_prob = Settings::ParameterDestructiveMutationProb;
	NEAT::mutate_morph_param_spread = Settings::ParameterMutationSpread;

	// Create base interface
	Interface = new ExperimentInterface();
	Interface->Init();

	// Fill the world
	auto world_ptr = &((ExperimentInterface*)Interface)->World;
	world_ptr->PlantsAreas = 
	{
		{{
			float2(2350.0,-640.0),
			360.0f
		}},
		{{
			float2(2560.0,-4810.0),
			380.0f
		}},
	};
	world_ptr->SpawnAreas = 
	{
		{
			float2(750,-1040),
			1900.0f
		},
		{
			float2(620.0,-4240.0),
			2300.0f
		}
	};

	// Spawn population
	Population pop;
	pop.Spawn(ExperimentParams::PopSizeMultiplier, ExperimentParams::SimulationSize);

	// Do evolution loop
	vector<float> avg_fitness_carnivore;
	vector<float> avg_fitness_herbivore;
	vector<float> avg_fitness_carnivore_random;
	vector<float> avg_fitness_herbivore_random;
	vector<float> avg_progress_herbivore;
	vector<float> avg_progress_carnivore;

	plt::ion();
	
	bool show_simulation = false;
	{
		char c;
		cout << "Show simulation (y/n) ? ";
		cin >> c;
		if (c == 'y')
			show_simulation = true;
	}

	for (int g = 0; g < ExperimentParams::GenerationsCount; g++)
	{
		pop.Epoch(world_ptr, [&](int gen)
		{
			cout << "Generation : " << gen << endl;
			cout << "    " << pop.GetSpecies().size() << " Species " << endl;
			pop.ComputeDevMetrics(world_ptr);

			for (const auto&[tag, species] : pop.GetSpecies())
			{
				cout << "--------------------" << endl;
				cout << "        Size : " << species.IndividualsIDs.size() << endl;
				cout << "        Progress Metric : " << species.ProgressMetric << endl;
				cout << "        Fitness " << species.DevMetrics.RealFitness << " vs " << species.DevMetrics.RandomFitness << endl;
		
				// Check if the organism is carnivore or herbivore by checking the components
				ComponentRef herbivore_mouth = { 0,0 };
				if (find(tag.begin(), tag.end(), herbivore_mouth) != tag.end())
				{
					avg_fitness_herbivore.push_back(species.DevMetrics.RealFitness);
					avg_fitness_herbivore_random.push_back(species.DevMetrics.RandomFitness);
					avg_progress_herbivore.push_back(species.ProgressMetric);
				}
				else
				{
					avg_fitness_carnivore.push_back(species.DevMetrics.RealFitness);
					avg_fitness_carnivore_random.push_back(species.DevMetrics.RandomFitness);
					avg_progress_carnivore.push_back(species.ProgressMetric);
				}

				// Plot the graphs
				plt::clf();
				plt::subplot(2, 2, 1);
				plt::plot(avg_fitness_herbivore, "b");
				plt::plot(avg_fitness_herbivore_random, "r");

				plt::subplot(2, 2, 2);
				plt::plot(avg_fitness_carnivore, "k");
				plt::plot(avg_fitness_carnivore_random, "r");

				plt::subplot(2, 2, 3);
				plt::plot(avg_progress_herbivore, "b");
				plt::plot(avg_progress_carnivore, "k");

				plt::pause(0.01);
			}
		});
	}
	
	// TODO : Save the registry to file

	// If needed, show the simulation
	if (show_simulation)
	{
		// TODO :  Use the registry, this is just keeping the first N
		for (auto& org : pop.GetIndividuals())
		{
			org.InSimulation = false;
			Interface->ResetState(org.GetState());
		}

		for (int i = 0; i < ExperimentParams::SimulationSize;i++)
			pop.GetIndividuals()[i].InSimulation = true;

		plt::figure();
		while (true)
		{
			plt::clf();

			plt::xlim(GameplayParams::GameArea.Min.x, GameplayParams::GameArea.Max.x);
			plt::ylim(GameplayParams::GameArea.Min.y, GameplayParams::GameArea.Max.y);

			for (auto& org : pop.GetIndividuals())
				if(org.InSimulation && ((OrgState*)org.GetState())->Life >= 0)
					org.DecideAndExecute(world_ptr, &pop);
			
			for (auto [area,_] : world_ptr->PlantsAreas)
			{
				// Generate points to plot the circle
				float count = 1000;
				vector<float> x_vec(count);
				vector<float> y_vec(count);

				float th = 0;
				for (auto [x, y] : zip(x_vec, y_vec))
				{
					x = cosf(th * 2 * 3.14159f)*area.Radius + area.Center.x;
					y = sinf(th * 2 * 3.14159f)*area.Radius + area.Center.y;
					th += 1.0f / count;
				}
				plt::plot(x_vec, y_vec, "g");
			}

			// Plot the individuals with different colors per species
			vector<float> org_x, org_y;
			for (auto [species_idx,entry] : enumerate(pop.GetSpecies()))
			{
				auto [_, species] = entry;
				org_x.resize(0);
				org_y.resize(0);

				for (int org_idx : species.IndividualsIDs)
				{
					auto state_ptr = (OrgState*)pop.GetIndividuals()[org_idx].GetState();
					if (state_ptr->Life >= 0)
					{
						org_x.push_back(state_ptr->Position.x);
						org_y.push_back(state_ptr->Position.y);
					}
				}

				plt::plot(org_x, org_y, string("x") + string("C") + to_string(species_idx));
			}

			plt::pause(0.1);
		}
	}

	// NOTE! : Explictely leaking the interface here!
	// There's a good reason for it
	// The objects in the system need the interface to delete themselves
	//  so I can't delete it, as I don't know the order in which destructors are called
	// In any case, this is not a problem, because on program exit the SO should (will?) release the memory anyway
	//delete Interface;

	return 0;
}