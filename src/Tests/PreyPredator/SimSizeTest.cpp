#include <iostream>
#include "../PreyPredator/PreyPredator.h"
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
#include "../PreyPredator/PublicInterfaceImpl.h"

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
int FoodCellCount;

int main(int argc, char *argv[])
{
	minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

	string cfg_path = "../src/Tests/PreyPredator/Config.cfg";

	ConfigLoader loader(cfg_path);
	WorldSizeX = WorldSizeY = -1;

	loader.LoadValue(WorldSizeX, "WorldSizeX");
	loader.LoadValue(WorldSizeY, "WorldSizeY");
	loader.LoadValue(FoodScoreGain, "FoodScoreGain");
	loader.LoadValue(KillScoreGain, "KillScoreGain");
	loader.LoadValue(DeathPenalty, "DeathPenalty");
	float food_proportion;
	loader.LoadValue(food_proportion, "FoodProportion");
	FoodCellCount = WorldSizeX * WorldSizeY*food_proportion;
	loader.LoadValue(MaxSimulationSteps, "MaxSimulationSteps");
	loader.LoadValue(SimulationSize, "SimulationSize");
	loader.LoadValue(PopSizeMultiplier, "PopSizeMultiplier");
	PopulationSize = PopSizeMultiplier * SimulationSize;
	loader.LoadValue(GenerationsCount, "GenerationsCount");
	loader.LoadValue(LifeLostPerTurn, "LifeLostPerTurn");
	loader.LoadValue(BorderPenalty, "BorderPenalty");
	loader.LoadValue(WastedActionPenalty, "WastedActionPenalty");
	loader.LoadValue(SerializationFile, "SerializationFile");

	Settings::LoadFromFile(loader);

	::NEAT::load_neat_params("../cfg/neat_test_config.txt");
	::NEAT::mutate_morph_param_prob = Settings::ParameterMutationProb;
	::NEAT::destructive_mutate_morph_param_prob = Settings::ParameterDestructiveMutationProb;
	::NEAT::mutate_morph_param_spread = Settings::ParameterMutationSpread;

	Interface = new PublicInterfaceImpl();
	Interface->Init();

	GenerationsCount = 100;
	Settings::SimulationReplications = 100;
	
	vector<int> pop_sizes;
	vector<float> eaten_vec_carnivore;
	vector<float> failed_vec_carnivore;
	vector<float> coverage_vec_carnivore;
	vector<float> eaten_vec_herbivore;
	vector<float> failed_vec_herbivore;
	vector<float> coverage_vec_herbivore;

	ofstream outf("sim_size.csv");
	int pop_size = 200;
	for (SimulationSize = 10; SimulationSize <= 100; SimulationSize += 5)
	{
		PopSizeMultiplier = pop_size / SimulationSize;
		PopulationSize = PopSizeMultiplier * SimulationSize;
		cout << "Simulation Size : " << SimulationSize << endl;
		cout << "Pop Size : " << PopulationSize << endl;

		Population pop;
		pop.Spawn(PopSizeMultiplier, SimulationSize);

		WorldData world;
		world.fill(FoodCellCount, WorldSizeX, WorldSizeY);

		Metrics metrics;
		for (int g = 0; g < GenerationsCount; g++)
		{
			((PublicInterfaceImpl*)Interface)->CurrentGenNumber = g;

			pop.Epoch(&world, [&](int gen)
			{
				cout << ".";
				if(gen == GenerationsCount-1)
					metrics.update(pop);
			}, true);
		}
		cout << endl;

		outf << SimulationSize << ",";
		outf << PopulationSize << ",";

		// We only update the metrics once
		outf << metrics.avg_eaten_carnivore[0] << ",";
		outf << metrics.avg_failed_carnivore[0] << ",";
		outf << metrics.avg_coverage_carnivore[0] << ",";
		outf << metrics.avg_eaten_herbivore[0] << ",";
		outf << metrics.avg_failed_herbivore[0] << ",";
		outf << metrics.avg_coverage_herbivore[0] << ",";
		outf << endl;
	}

	outf.close();
}