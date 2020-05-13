#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Shlwapi.h"
#pragma comment( lib, "shlwapi.lib")

#include <iostream>
#include "ComplexSystem.h"
#include "../../Core/Config.h"
#include <string>
#include <assert.h>
#include <algorithm>
#include <enumerate.h>
#include <random>
#include <queue>
#include <thread>
#include <chrono>

#include "../../Core/Config.h"
#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include "../../Serialization/SRegistry.h"
#include "../../Utils/Math/Float2.h"

#include "ComplexSystem.h"
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
	auto start_t = chrono::high_resolution_clock::now();

	string cfg_path = "../src/Tests/ComplexSystem/Config.cfg";
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

	// Fix the RNG to get deterministic results during species evolution and selection
	//minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());
	minstd_rand RNG(2);

	Interface = new PublicInterfaceImpl();
	Interface->Init();

	// Create and fill the world
	WorldData world = createWorld(world_path);

	// Have some more individuals to have more comparison points
	SimulationSize = 30;

	// Make evolution if the registry files weren't found
	if(!PathFileExists("human_test_evolved_g400.txt"))
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
		}

		pop.CurrentSpeciesToRegistry();
		SRegistry registry(&pop);
		registry.save("human_test_evolved_g400.txt");
	}

	// Load the evolved population
	SRegistry registry;
	system("cls");
	registry.load("human_test_evolved_g400.txt");

	cout << "Show action and sensors descriptions (y or n)? ";
	bool show_descs;
	{
		char c;
		cin >> c;
		if (c == 'y')
			show_descs = true;
		else
			show_descs = false;

	}

	// Create the individuals that are gonna be used in the simulation
	vector<vector<int>> species_ids;
	vector<BaseIndividual*> individuals;

	// Select randomly until the simulation size is filled
	species_ids.resize(registry.Species.size());
	for (int i = 0; i < SimulationSize; i++)
	{
		static bool has_herbivore = false;
		static bool has_carnivore = false;
		static bool has_omnivore = false;

		int species_idx;
		bool found_new = false;
		do
		{
			species_idx = uniform_int_distribution<int>(0, registry.Species.size() - 1)(RNG);

			if (registry.Species[species_idx].Individuals[0].HasAction((int)ActionsIDs::EatFood) && !has_herbivore)
			{
				has_herbivore = true;
				found_new = true;
			}
			else if (registry.Species[species_idx].Individuals[0].HasAction((int)ActionsIDs::EatEnemy) && !has_carnivore)
			{
				has_carnivore = true;
				found_new = true;
			}
			else if (registry.Species[species_idx].Individuals[0].HasAction((int)ActionsIDs::EatFoodEnemy) && !has_omnivore)
			{
				has_omnivore = true;
				found_new = true;
			}
		} while (!found_new);

		// Have at least 5 of each species
		for (int i = 0; i < 5; i++)
		{
			int individual_idx = uniform_int_distribution<int>(0, registry.Species[species_idx].Individuals.size() - 1)(RNG);

			SIndividual * org = new SIndividual;
			*org = registry.Species[species_idx].Individuals[individual_idx];
			org->InSimulation = true;
			individuals.push_back(org);
			species_ids[species_idx].push_back(individuals.size() - 1);
		}

		// If we found one of each, start over
		if (has_herbivore && has_carnivore && has_omnivore)
			has_herbivore = has_carnivore = has_omnivore = false;
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

	vector<float> score_vec(individuals.size());
	vector<float> tmp_vec(individuals.size());
	fill(score_vec.begin(), score_vec.end(), 0);
	fill(tmp_vec.begin(), tmp_vec.end(), 0);

	for (auto& org : individuals)
		org->Reset();

	ofstream history("history_"
		+ string(show_descs ? "_showing_descs" : "")
		+ "_stamp_"
		+ to_string(chrono::steady_clock::now().time_since_epoch().count())
		+ ".csv");

	string action_names[(int)ActionsIDs::NumberOfActions];
	action_names[(int)ActionsIDs::MoveForward] = "Move Forward";
	action_names[(int)ActionsIDs::MoveBackwards] = "Move Backwards";
	action_names[(int)ActionsIDs::MoveLeft] = "Move Left";
	action_names[(int)ActionsIDs::MoveRight] = "Move Right";
	action_names[(int)ActionsIDs::SwimMoveForward] = "Move Forward";
	action_names[(int)ActionsIDs::SwimMoveBackwards] = "Move Backwards";
	action_names[(int)ActionsIDs::SwimMoveLeft] = "Move Left";
	action_names[(int)ActionsIDs::SwimMoveRight] = "Move Right";
	action_names[(int)ActionsIDs::JumpForward] = "Jump Forward";
	action_names[(int)ActionsIDs::JumpBackwards] = "Jump Backwards";
	action_names[(int)ActionsIDs::JumpLeft] = "Jump Left";
	action_names[(int)ActionsIDs::JumpRight] = "Jump Right";
	action_names[(int)ActionsIDs::Kill] = "Kill";
	action_names[(int)ActionsIDs::EatFoodEnemy] = "Eat (Food or Corpse)";

	string sensor_names[(int)SensorsIDs::NumberOfSensors];
	sensor_names[(int)SensorsIDs::NearestCompetidorDeltaX] = "Delta X Nearest Competitor";
	sensor_names[(int)SensorsIDs::NearestCompetidorDeltaY] = "Delta Y Nearest Competitor";
	sensor_names[(int)SensorsIDs::NearestCorpseDeltaX] = "Delta X Nearest Corpse";
	sensor_names[(int)SensorsIDs::NearestCorpseDeltaY] = "Delta Y Nearest Corpse";
	sensor_names[(int)SensorsIDs::NearestFoodDeltaX] = "Delta X Food";
	sensor_names[(int)SensorsIDs::NearestFoodDeltaY] = "Delta Y Food";
	sensor_names[(int)SensorsIDs::CurrentCell] = "Current Cell";
	sensor_names[(int)SensorsIDs::TopCell] = "Forward Cell";
	sensor_names[(int)SensorsIDs::DownCell] = "Backwards Cell";
	sensor_names[(int)SensorsIDs::RightCell] = "Right Cell";
	sensor_names[(int)SensorsIDs::LeftCell] = "Left Cell";

	string cell_names[(int)CellType::NumberOfTypes];
	cell_names[(int)CellType::Ground] = "Ground";
	cell_names[(int)CellType::Water] = "Water";
	cell_names[(int)CellType::Wall] = "Wall";

	int num_in_user_species = 0;
	for (int idx = 0; idx < individuals.size(); idx++)
		if (individuals[idx]->GetMorphologyTag() == individuals[user_idx]->GetMorphologyTag())
			num_in_user_species++;

	// Go back to non determinism
	RNG = minstd_rand(chrono::high_resolution_clock::now().time_since_epoch().count());

	auto end_t = chrono::high_resolution_clock::now();
	cout << "Time (s) : " << chrono::duration_cast<chrono::seconds>(end_t - start_t).count() << endl;

	bool exit_sim = false;
	int step = 0;
	constexpr int MaxSteps = 75;
	for(; step < MaxSteps; step++)
	{
		if (individuals[user_idx]->GetState<OrgState>()->Life <= 0)
			break;

		system("cls");
		for (int idx = 0; idx < individuals.size(); idx++)
		{
			auto state_ptr = individuals[idx]->GetState<OrgState>();

			if (idx == user_idx)
			{
				// Print score info
				cout << "Score : " << score_vec[idx];
				history << score_vec[idx] << ",";
				history << state_ptr->FailedCount << ","; 
				history << state_ptr->FailableCount << ",";

				// TODO : Refactor this code, is awful
				for (auto [fa, fb] : zip(tmp_vec, score_vec))
					fa = fb;
				sort(tmp_vec.rbegin(), tmp_vec.rend());
				int current_user_position = 0;
				for (int fidx = 0; fidx < tmp_vec.size(); fidx++)
				{
					if (tmp_vec[fidx] == score_vec[idx])
					{
						cout << " [Position " << current_user_position + 1 << " of " << num_in_user_species << "]" << endl;
						history << current_user_position + 1 << "," << num_in_user_species;
						break;
					}
					else if (individuals[fidx]->GetMorphologyTag() == individuals[user_idx]->GetMorphologyTag())
						current_user_position++;
				}

				// Store the scores of the individuals on the species
				history << ",#";
				for (int fidx = 0; fidx < tmp_vec.size(); fidx++)
				{
					if (individuals[fidx]->GetMorphologyTag() == individuals[user_idx]->GetMorphologyTag())
						history << "," << score_vec[fidx];
				}

				// Store the failed count of the individuals on the species
				history << ",#";
				for (int fidx = 0; fidx < tmp_vec.size(); fidx++)
				{
					if (individuals[fidx]->GetMorphologyTag() == individuals[user_idx]->GetMorphologyTag())
						history << "," << individuals[fidx]->GetState<OrgState>()->FailedCount;
				}

				// Store the failable count of the individuals on the species
				history << ",#";
				for (int fidx = 0; fidx < tmp_vec.size(); fidx++)
				{
					if (individuals[fidx]->GetMorphologyTag() == individuals[user_idx]->GetMorphologyTag())
						history << "," << individuals[fidx]->GetState<OrgState>()->FailableCount;
				}

				history << endl;
				cout << endl;

				// No description, just show raw sensors
				auto org_ptr = (SIndividual*)individuals[idx];
				cout << "Sensors" << endl;
				for (int sensor_id : org_ptr->Sensors)
				{
					cout << "    "
						<< (show_descs ? " " + sensor_names[sensor_id] + "" : "***")
						<< " : ";
					float val = Interface->GetSensorRegistry()[sensor_id].Evaluate(state_ptr, individuals, org_ptr, &world);

					if (show_descs &&
						((SensorsIDs)sensor_id == SensorsIDs::CurrentCell ||
						(SensorsIDs)sensor_id == SensorsIDs::TopCell ||
							(SensorsIDs)sensor_id == SensorsIDs::DownCell ||
							(SensorsIDs)sensor_id == SensorsIDs::RightCell ||
							(SensorsIDs)sensor_id == SensorsIDs::LeftCell))
					{
						cout << cell_names[(int)val];
					}
					else
					{
						cout << val;
					}

					cout << endl;
				}

				// Ask the user to decide
				if (show_descs)
				{
					for (auto[action_index, action_id] : enumerate(org_ptr->Actions))
						cout << action_index << " : " << action_names[action_id] << endl;
				}
				cout << endl;
				cout << "Action ? [0 to " << org_ptr->Actions.size() - 1 << ", -1 to exit]" << endl;
				int action;
				cin >> action;
				while ((action < 0 && action != -1) || action >= (int)org_ptr->Actions.size())
					cin >> action;

				if (action == -1)
				{
					exit_sim = true;
					break;
				}

				// Execute
				Interface->GetActionRegistry()[org_ptr->Actions[action]].Execute(state_ptr, individuals, org_ptr, &world);
			}
			else if (state_ptr->Life > 0)
				individuals[idx]->DecideAndExecute(&world, individuals);

			// Update fitness
			if (state_ptr->Life > 0)
			{
				state_ptr->Score += 0.1*state_ptr->Life;
				score_vec[idx] = state_ptr->Score;
			}
		}
	}

	history.close();

	if (exit_sim)
		cout << "Exited test early" << endl;
	else if (step < MaxSteps)
		cout << "Your individual has died!" << endl;
	else
		cout << "Test finished" << endl;
}