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
#include "network.h"
#include "genome.h"

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
	GenerationsCount = 10;
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

	// Evolve
	WorldData world = createWorld("../assets/worlds/world0.txt");
	Population pop(&world);
	pop.Spawn(PopSizeMultiplier, SimulationSize);

	for (int g = 0; g < GenerationsCount; g++)
	{
		cout << g << endl;
		pop.Epoch([](int) {}, true);
	}
	
	pop.CurrentSpeciesToRegistry();

	// Dump networks of the historical best organism of each species
	for (auto[idx, entry] : enumerate(pop.GetSpeciesRegistry()))
	{
		string fname = "network_species" + to_string(idx);
		SIndividual tmp_org(entry.HistoricalBestGenome, entry.Morphology);

		if (tmp_org.HasAction((int)ActionsIDs::EatFoodEnemy))
			fname += "_omnivore";
		else if (tmp_org.HasAction((int)ActionsIDs::EatEnemy))
			fname += "_herbivore";
		else
			fname += "_carnivore";

		DumpNetworkToDot(fname + ".dot", entry.HistoricalBestGenome->genesis(0));
	}
}