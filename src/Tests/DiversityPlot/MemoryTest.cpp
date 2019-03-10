#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "psapi.h"

#include <iostream>
#include "DiversityPlot.h"
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

	string cfg_path = "../src/Tests/DiversityPlot/Config.cfg";

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

	vector<float> memory_vec(GenerationsCount);
	fill(memory_vec.begin(), memory_vec.end(), 0);

	vector<size_t> org_mem_vec;
	//for (int world_id = 0; world_id < 5; world_id++)
	{
		int world_id = 0;
		auto world_path = world_path_base + to_string(world_id) + ".txt";
		WorldData world = createWorld(world_path);

		cout << "    World " << world_id << endl;
		
		Population pop;
		pop.Spawn(PopSizeMultiplier, SimulationSize);

		for (int g = 0; g < GenerationsCount; g++)
		{
			if (g % 10 == 0) cout << "    Generation " << g << endl;
			pop.Epoch(&world, [](int) {}, true);

			PROCESS_MEMORY_COUNTERS pmc;
			GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

			memory_vec[g] += pmc.WorkingSetSize;
		}

		pop.CurrentSpeciesToRegistry();
		SRegistry registry(&pop);
		
		// Find average memory usage of the individuals on the registry
		for (const auto& entry : registry.Species)
		{
			size_t avg_mem = 0;
			for (const auto& org : entry.Individuals)
				org_mem_vec.push_back(org.TotalSize());
		}
	}

	ofstream out_mem("mem_evo.csv");
	for (float& m : memory_vec)
	{
		m /= 5;
		out_mem << m << endl;
	}
	out_mem.close();

	ofstream out_mem_org("mem_org.csv");
	for (size_t m : org_mem_vec)
		out_mem_org << m << endl;
	out_mem_org.close();
}