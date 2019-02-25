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

using namespace agio;
using namespace std;

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

	// Create and fill the world
	WorldData world = createWorld(world_path);

	// Load the evolved population
	SRegistry registry;
	registry.load(SerializationFile);

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

	for (auto& org : individuals)
		org->Reset();

	// Select a random individual for the user to control, show the number of eaten stuff and life
	//   and also show average eat count and life for the species of the individual controled by the user
	int user_idx = uniform_int_distribution<int>(0, individuals.size() - 1)(RNG);
	auto& user_org = individuals[user_idx];
	while (true)
	{
		
	}
}