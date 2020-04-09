#include <iostream>
#include "ComplexSystem.h"
#include "../../Core/Config.h"
#include <string>
#include "../../Utils/Utils.h"
#include "../../Serialization/SIndividual.h"
#include "zip.h"
#include "enumerate.h"
#include <fstream>
#include "../../Serialization/SRegistry.h"
#include "neat.h"
#include "../../Utils/WorkerPool.h"

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
	minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

	string cfg_path = "../src/Tests/ComplexSystem/Config.cfg";

	ConfigLoader loader(cfg_path);
	WorldSizeX = WorldSizeY = -1;

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

	::NEAT::load_neat_params("../cfg/neat_test_config.txt");
	::NEAT::mutate_morph_param_prob = Settings::ParameterMutationProb;
	::NEAT::destructive_mutate_morph_param_prob = Settings::ParameterDestructiveMutationProb;
	::NEAT::mutate_morph_param_spread = Settings::ParameterMutationSpread;

	Interface = new PublicInterfaceImpl();
	Interface->Init();

	string world_path_base = "../assets/worlds/world";

	vector<vector<float>> total_times_vec;
	vector<vector<float>> decide_times_vec;
	vector<pair<int,float>> evolution_times_vec;

	// Measure evolution
	for (PopSizeMultiplier = 10; PopSizeMultiplier <= 10; PopSizeMultiplier += 1)
	{
		PopulationSize = PopSizeMultiplier * SimulationSize;
		cout << "Pop Size : " << PopulationSize << endl;
		float avg_t = 0;
		for (int world_id = 0; world_id < 5; world_id++)
		{
			auto world_path = world_path_base + to_string(world_id) + ".txt";
			WorldData world = createWorld(world_path);

			cout << "    World " << world_id << endl;
			PROFILE(evolution_time,
				Population pop;
			pop.Spawn(PopSizeMultiplier, SimulationSize);

			for (int g = 0; g < GenerationsCount; g++)
				pop.Epoch(&world, [](int) {}, true);
			);

			avg_t += evolution_time;
		}

		avg_t /= 5;
		cout << "    Time : " << avg_t << endl;
	}
	system("pause");

	// Measure evolution
	for (GenerationsCount = 50; GenerationsCount <= 300; GenerationsCount += 50)
	{
		cout << "Generations : " << GenerationsCount << endl;
		float avg_t = 0;
		for (int world_id = 0; world_id < 5; world_id++)
		{
			auto world_path = world_path_base + to_string(world_id) + ".txt";
			WorldData world = createWorld(world_path);

			cout << "    World " << world_id << endl;
			PROFILE(evolution_time,
				Population pop;
				pop.Spawn(PopSizeMultiplier, SimulationSize);

				for (int g = 0; g < GenerationsCount; g++)
					pop.Epoch(&world, [](int) {}, true);
			);

			avg_t += evolution_time;
		}

		avg_t /= 5;
		cout << "    Time : " << avg_t << endl;
	}
	system("pause");

	//for (int world_id = 0; world_id < 5; world_id++)
	if(false)
	{
		int world_id = -1;
		cout << "Evaluating world " << world_id << endl;

		total_times_vec.push_back({});
		decide_times_vec.push_back({});

		auto world_path = world_path_base + to_string(world_id) + ".txt";
		WorldData world = createWorld(world_path);

		// Measure evolution
		cout << "    Profiling evolution" << endl;
		cout << "        Generations : " << GenerationsCount << endl;
		PROFILE(evolution_time,
			Population pop;
			pop.Spawn(PopSizeMultiplier, SimulationSize);

			for (int g = 0; g < GenerationsCount; g++)
				pop.Epoch(&world, [](int) {}, true);

			pop.EvaluatePopulation(&world);
			pop.CurrentSpeciesToRegistry();

			SRegistry registry_save(&pop);
			registry_save.save(SerializationFile);
		);
		evolution_times_vec.push_back({ GenerationsCount,evolution_time });

		// Measure simulation
		cout << "    Profiling simulation" << endl;
		SRegistry registry;
		registry.load(SerializationFile);

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

		for (int i = 0; i < MaxSimulationSteps; i++)
		{
			for (auto& org : individuals)
			{
				if (org->GetState<OrgState>()->Life > 0) // TODO : Create a new one of a different species
				{
					auto s_org = (SIndividual *)org;

					PROFILE(total_time,
						// Load sensors
						for (auto[index, id] : enumerate(s_org->Sensors))
							s_org->UpdateSensorValueFromIndex(index, Interface->GetSensorRegistry()[id].Evaluate(org->GetState(), individuals, org, &world));

						// Decide action
						PROFILE(decision_time, 
							int action = s_org->DecideAction();
						);

						// Finally execute the action
						Interface->GetActionRegistry()[s_org->Actions[action]].Execute(org->GetState(), individuals, org, &world);
					);

					total_times_vec.back().push_back(total_time);
					decide_times_vec.back().push_back(decision_time);
				}
			}
		}
	}

	// Writes times to csv, one column per world
	cout << "Writting results to disk" << endl;
	ofstream out_total_time("total_times.csv");
	ofstream out_decision_time("decision_times.csv");

	for (int org_id = 0; org_id < MaxSimulationSteps*MaxSimulationSteps; org_id++)
	{
		for (int world_id = 0; world_id < 5; world_id++)
		{
			if (total_times_vec[world_id].size() > org_id)
				out_total_time << total_times_vec[world_id][org_id] << ",";
			if (decide_times_vec[world_id].size() > org_id)
				out_decision_time << decide_times_vec[world_id][org_id] << ",";
		}

		out_total_time << endl;
		out_decision_time << endl;
	}

	out_total_time.close();
	out_decision_time.close();

	ofstream out_evo_time("evo_time.csv");
	for (auto [gen_count, t] : evolution_times_vec)
		out_evo_time << gen_count << "," << t << endl;
	out_evo_time.close();
}