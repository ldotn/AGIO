#include "3DExperiment.h"
#include <matplotlibcpp.h>
#include "neat.h"
#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include "../../Core/Config.h"
#include <iostream>
#include "zip.h"
#include "enumerate.h"
#include "../../Serialization/SRegistry.h"

namespace plt = matplotlibcpp;
using namespace agio;
using namespace std;
using namespace fpp;

// Does the evolution loop and saves the evolved networks to a file
int main()
{
	// Values hardcoded from the UE4 level
	// TODO : Find a better way to set the values
	GameplayParams::GameArea = { {-2840,-8000}, {3200,2900} };

	{
		ConfigLoader cfg("../src/Tests/3DExperiment/Config.cfg");
		cfg.LoadValue(GameplayParams::WalkSpeed,"WalkSpeed");
		cfg.LoadValue(GameplayParams::RunSpeed,"RunSpeed");

		int turn_degrees;
		cfg.LoadValue(turn_degrees,"TurnDegrees");
		GameplayParams::TurnRadians = turn_degrees * 0.0174533f;

		cfg.LoadValue(GameplayParams::EatDistance,"EatDistance");
		cfg.LoadValue(GameplayParams::WastedActionPenalty,"WastedActionPenalty");
		cfg.LoadValue(GameplayParams::CorpseBitesDuration,"CorpseBitesDuration");
		cfg.LoadValue(GameplayParams::EatCorpseLifeGained,"EatCorpseLifeGained");
		cfg.LoadValue(GameplayParams::EatPlantLifeGained,"EatPlantLifeGained");
		cfg.LoadValue(GameplayParams::AttackDamage,"AttackDamage");
		cfg.LoadValue(GameplayParams::LifeLostPerTurn,"LifeLostPerTurn");
		cfg.LoadValue(GameplayParams::StartingLife,"StartingLife");

		cfg.LoadValue(ExperimentParams::MaxSimulationSteps, "MaxSimulationSteps");
		cfg.LoadValue(ExperimentParams::PopSizeMultiplier, "PopSizeMultiplier");
		cfg.LoadValue(ExperimentParams::SimulationSize, "SimulationSize");
		cfg.LoadValue(ExperimentParams::GenerationsCount, "GenerationsCount");

		// The total population size must be >= Settings::MinIndividualsPerSpecies
		if (ExperimentParams::SimulationSize*ExperimentParams::PopSizeMultiplier < Settings::MinIndividualsPerSpecies)
		{
			cout << "The total population size must be >= MinIndividualsPerSpecies" << endl;
			cout << "Total population size is " << ExperimentParams::SimulationSize
				<< " * " << ExperimentParams::PopSizeMultiplier << " = "
				<< ExperimentParams::PopSizeMultiplier*ExperimentParams::SimulationSize
				<< " but MinIndividualsPerSpecies = " << Settings::MinIndividualsPerSpecies << endl;
			cout << "Press any key plus enter to exit" << endl;
			string dummy_;
			cin >> dummy_;
			return 1;
		}
	}

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
	
	pop.EvaluatePopulation(world_ptr);
	pop.CurrentSpeciesToRegistry();

	SRegistry registry(&pop);
	registry.save("evolved_3d_experiment.txt");
	registry.load("evolved_3d_experiment.txt");

	// If needed, show the simulation
	if (show_simulation)
	{
		vector<BaseIndividual*> individuals;
		for (auto &entry : registry.Species)
			for (int idx = 0;idx < entry.Individuals.size();idx++)
				individuals.push_back(&entry.Individuals[idx]);

		minstd_rand0 rng(42);
		shuffle(individuals.begin(), individuals.end(),rng);

		if (individuals.size() > ExperimentParams::SimulationSize)
		{
			individuals.resize(ExperimentParams::SimulationSize);
		}
		else
		{
			// TODO : Duplicate to find new ones
		}

		for (auto org_ptr : individuals)
		{
			org_ptr->InSimulation = true; // Just in case
			org_ptr->State = Interface->MakeState(org_ptr);
		}

		unordered_map<MorphologyTag, vector<BaseIndividual*>> species_map;
		for (auto org_ptr : individuals)
			species_map[org_ptr->GetMorphologyTag()].push_back(org_ptr);

		plt::figure();
		while (true)
		{
			plt::clf();

			plt::xlim(GameplayParams::GameArea.Min.x, GameplayParams::GameArea.Max.x);
			plt::ylim(GameplayParams::GameArea.Min.y, GameplayParams::GameArea.Max.y);

			for (auto& org : individuals)
				if(((OrgState*)org->GetState())->Life >= 0)
					org->DecideAndExecute(world_ptr, individuals);
			
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
			for (auto [species_idx,entry] : enumerate(species_map))
			{
				auto [_, species] = entry;
				org_x.resize(0);
				org_y.resize(0);

				for (auto org_ptr : entry.second)
				{
					auto state_ptr = (OrgState*)org_ptr->GetState();
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

int ExperimentParams::MaxSimulationSteps;
int ExperimentParams::PopSizeMultiplier;
int ExperimentParams::SimulationSize;
int ExperimentParams::GenerationsCount;