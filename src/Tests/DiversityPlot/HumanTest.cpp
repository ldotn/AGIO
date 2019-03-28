#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Shlwapi.h"
#pragma comment( lib, "shlwapi.lib")

#include <iostream>
#include "DiversityPlot.h"
#include "../../Core/Config.h"
#include <string>
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
#include "enumerate.h"

using namespace agio;
using namespace std;
using namespace fpp;

// Need to define them somewhere
int WorldSizeX;
int WorldSizeY;
float FoodScoreGain;
float KillScoreGain;
float DeathPenalty;
float FoodCellProportion;
int MaxSimulationSteps;
int SimulationSize;
int PopSizeMultiplier;
int PopulationSize;
int GenerationsCount;
float LifeLostPerTurn;
float BorderPenalty;
float WastedActionPenalty;
float WaterPenalty;
int InitialLife;
string SerializationFile;

int main(int argc, char *argv[])
{
	string cfg_path = "../src/Tests/DiversityPlot/Config.cfg";
	string world_path = "../assets/worlds/world0.txt";
	if (argc == 4)
	{
		cfg_path = argv[1];
		world_path = argv[3];
	}

	ConfigLoader loader(cfg_path);
	WorldSizeX = WorldSizeY = -1;

	NEAT::load_neat_params("../cfg/neat_test_config.txt");
	NEAT::mutate_morph_param_prob = Settings::ParameterMutationProb;
	NEAT::destructive_mutate_morph_param_prob = Settings::ParameterDestructiveMutationProb;
	NEAT::mutate_morph_param_spread = Settings::ParameterMutationSpread;

	loader.LoadValue(FoodScoreGain, "FoodScoreGain");
	loader.LoadValue(KillScoreGain, "KillScoreGain");
	loader.LoadValue(DeathPenalty, "DeathPenalty");
	loader.LoadValue(FoodCellProportion, "FoodProportion");
	loader.LoadValue(MaxSimulationSteps, "MaxSimulationSteps");
	loader.LoadValue(SimulationSize, "SimulationSize");
	loader.LoadValue(PopSizeMultiplier, "PopSizeMultiplier");
	PopulationSize = PopSizeMultiplier * SimulationSize;
	loader.LoadValue(GenerationsCount, "GenerationsCount");
	loader.LoadValue(LifeLostPerTurn, "LifeLostPerTurn");
	loader.LoadValue(BorderPenalty, "BorderPenalty");
	loader.LoadValue(WastedActionPenalty, "WastedActionPenalty");
	loader.LoadValue(WaterPenalty, "WaterPenalty");
	loader.LoadValue(InitialLife, "InitialLife");
	loader.LoadValue(SerializationFile, "SerializationFile");

	Settings::LoadFromFile(loader);

	minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

	Interface = new PublicInterfaceImpl();
	Interface->Init();



	// DEBUG!
	Settings::SimulationReplications = 1;
	Settings::SpeciesStagnancyChances = 0;
	//Settings::ProgressThreshold = 0.0001;
	Settings::MinSpeciesAge = 2;
	PopSizeMultiplier = 5;
	PopulationSize = PopSizeMultiplier * SimulationSize;



	// Create and fill the world
	WorldData world = createWorld(world_path);

	// Make evolution if the registry files weren't found
	if( !PathFileExists("human_test_evolved_g25.txt") ||
		!PathFileExists("human_test_evolved_g200.txt") || 
		!PathFileExists("human_test_evolved_g400.txt"))
	{
		cout << "Evolution files not found, running evolution again. Will take some time" << endl;

		// Spawn population
		Population pop;
		pop.Spawn(PopSizeMultiplier, SimulationSize);

		// Evolve
		GenerationsCount = 400;

		for (int g = 0; g < GenerationsCount; g++)
		{
			pop.Epoch(&world);

			// Store at 3 different points, making 3 "dificulties"
			if (g == 24)
			{
				pop.CurrentSpeciesToRegistry();

				SRegistry registry(&pop);
				registry.save("human_test_evolved_g25.txt");
			}
			else if (g == 199)
			{
				pop.CurrentSpeciesToRegistry();

				SRegistry registry(&pop);
				registry.save("human_test_evolved_g200.txt");
			}
			else if (g == 399)
			{
				pop.CurrentSpeciesToRegistry();

				SRegistry registry(&pop);
				registry.save("human_test_evolved_g400.txt");
			}
		}
	}

	// Load the evolved population
	SRegistry registry;

	system("cls");
	cout << "Enter difficulty (0 = easy, 1 = medium, 2 = hard) : ";
	int difficulty;
	cin >> difficulty;
	if (difficulty == 0)
		registry.load("human_test_evolved_g25.txt");
	else if (difficulty == 1)
		registry.load("human_test_evolved_g200.txt");
	else
		registry.load("human_test_evolved_g400.txt");

	// Create the individuals that are gonna be used in the simulation
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


	// Select an omnivore for the user to control
	int user_idx = uniform_int_distribution<int>(0, individuals.size() - 1)(RNG);
	do
	{
		if (individuals[user_idx]->HasAction((int)ActionsIDs::EatFoodEnemy))
			break;
		user_idx = uniform_int_distribution<int>(0, individuals.size() - 1)(RNG);
	} while (true);

	vector<float> fitness_vec(individuals.size());
	fill(fitness_vec.begin(), fitness_vec.end(), 0);

	for (auto& org : individuals)
		org->Reset();

	ofstream history("history_difficulty_"
		+ to_string(difficulty)
		+ "_stamp_"
		+ to_string(chrono::steady_clock::now().time_since_epoch().count()));

	while (individuals[user_idx]->GetState<OrgState>()->Life > 0)
	{
		system("cls");
		for (int idx = 0; idx < individuals.size(); idx++)
		{
			auto state_ptr = individuals[idx]->GetState<OrgState>();

			if (idx == user_idx)
			{
				// Print score info
				cout << "Score : " << fitness_vec[idx];
				history << fitness_vec[idx] << ",";

				auto tmp_vec = fitness_vec;
				sort(tmp_vec.rbegin(), tmp_vec.rend());
				for (int fidx = 0; fidx < tmp_vec.size(); fidx++)
				{
					if (tmp_vec[fidx] == fitness_vec[idx])
					{
						cout << " [position " << fidx << " of " << fitness_vec.size() << " ]" << endl;
						history << fidx << endl;
						break;
					}
				}		

				// No description, just show raw sensors
				auto org_ptr = (SIndividual*)individuals[idx];
				cout << "Sensors" << endl;
				for (auto [sensor_index, sensor_id] : enumerate(org_ptr->Actions))
					cout << "    " 
					     << sensor_index 
					     << " : " << Interface->GetSensorRegistry()[sensor_index].Evaluate(state_ptr, individuals, org_ptr, &world)
					     << endl;

				// Ask the user to decide
				cout << "Action ? [0 to " << org_ptr->Actions.size() - 1 << " ]" << endl;
				int action;
				cin >> action;
				while (action < 0 || action >= org_ptr->Actions.size())
					cin >> action;

				// Execute
				Interface->GetActionRegistry()[org_ptr->Actions[action]].Execute(state_ptr, individuals, org_ptr, &world);
			}
			else individuals[idx]->DecideAndExecute(&world, individuals);

			// Update fitness
			state_ptr->Score += 0.1*state_ptr->Life;
			fitness_vec[idx] = state_ptr->Score;
		}
	}

	history.close();

	cout << "Your individual has died!" << endl;

	for (auto& org : individuals)
		org->Reset();
}