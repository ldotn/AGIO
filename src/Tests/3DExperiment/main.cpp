#include "3DExperiment.h"
#include "neat.h"
#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include "../../Core/Config.h"
#include <iostream>
#include "zip.h"
#include "enumerate.h"
#include "../../Serialization/SRegistry.h"

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
	for (int g = 0; g < ExperimentParams::GenerationsCount; g++)
	{
		((ExperimentInterface*)Interface)->CurrentGenNumber = g;

		pop.Epoch(world_ptr, [&](int gen)
		{
			cout << "Generation : " << gen << endl;
			cout << "    " << pop.GetSpecies().size() << " Species " << endl;

			for (const auto&[tag, species] : pop.GetSpecies())
			{
				cout << "--------------------" << endl;
				cout << "        Size : " << species.IndividualsIDs.size() << endl;
				cout << "        Progress Metric : " << species.ProgressMetric << endl;
				cout << "        Fitness " << species.DevMetrics.RealFitness << " vs " << species.DevMetrics.RandomFitness << endl;
			}
		});
	}
	
	pop.EvaluatePopulation(world_ptr);
	pop.CurrentSpeciesToRegistry();

	SRegistry registry(&pop);
	registry.save("evolved_3d_experiment.txt");

	// NOTE! : Explicitly leaking the interface here!
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