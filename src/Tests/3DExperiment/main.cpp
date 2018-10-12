#include "Interface.h"
#include <matplotlibcpp.h>
#include "neat.h"
#include "../../Evolution/Population.h"
#include "../../Serialization/SPopulation.h"
#include "../../Evolution/Globals.h"
#include "../../Core/Config.h"
#include <iostream>

namespace plt = matplotlibcpp;
using namespace agio;
using namespace std;

// Does the evolution loop and saves the evolved networks to a file
int main()
{
	// Values hardcoded from the UE4 level
	// TODO : Find a better way to set the values
	GameplayParams::WalkSpeed = 100.0f;
	GameplayParams::RunSpeed = 200.0f;
	GameplayParams::GameArea = { {-2840,-8000}, {3200,2900} };
	GameplayParams::TurnRadians = 10.0f * (3.14159f / 180.0f);
	GameplayParams::EatDistance = 35.0f;
	GameplayParams::WastedActionPenalty = 10;
	GameplayParams::CorpseBitesDuration = 5;
	GameplayParams::EatCorpseLifeGained = 10;
	GameplayParams::EatPlantLifeGained = 20;
	GameplayParams::AttackDamage = 40;
	GameplayParams::LifeLostPerTurn = 5;

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
			float2(2350.0,-670.0),
			500.0f
		}},
		{{
			float2(2560.0,-4810.0),
			380.0f
		}},
	};
	world_ptr->SpawnAreas = 
	{
		{
			float2(-80,400),
			820.0f
		},
		{
			float2(260.0,-4240.0),
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

	return 0;
}