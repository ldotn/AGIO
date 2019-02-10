#include <iostream>
#include "DiversityPlot.h"
#include "../../Core/Config.h"
#include <string>

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
	//loader.LoadValue(WorldSizeX,"WorldSizeX");
	//loader.LoadValue(WorldSizeY,"WorldSizeY");
	WorldSizeX = WorldSizeY = -1;

	loader.LoadValue(FoodScoreGain,"FoodScoreGain");
	loader.LoadValue(KillScoreGain,"KillScoreGain");
	loader.LoadValue(DeathPenalty,"DeathPenalty");
	//float food_proportion;
	loader.LoadValue(FoodCellProportion,"FoodProportion");
	//FoodCellCount = WorldSizeX * WorldSizeY*food_proportion;
	loader.LoadValue(MaxSimulationSteps,"MaxSimulationSteps");
	loader.LoadValue(SimulationSize,"SimulationSize");
	loader.LoadValue(PopSizeMultiplier,"PopSizeMultiplier");
	PopulationSize = PopSizeMultiplier * SimulationSize;
	loader.LoadValue(GenerationsCount,"GenerationsCount");
	loader.LoadValue(LifeLostPerTurn,"LifeLostPerTurn");
	loader.LoadValue(BorderPenalty,"BorderPenalty");
	loader.LoadValue(WastedActionPenalty,"WastedActionPenalty");
    loader.LoadValue(WaterPenalty, "WaterPenalty");
    loader.LoadValue(InitialLife, "InitialLife");
	loader.LoadValue(SerializationFile, "SerializationFile");

	Settings::LoadFromFile(loader);

	if (argc == 4)
	{
		auto final_pop = runEvolution(world_path, true);
		final_pop.SaveRegistryReport(argv[2]);
	}
	else
	{
		cout << "1 - Run Evolution" << endl;
		cout << "2 - Run Simulation" << endl;
		cout << "Select an option:";

		int op;
		cin >> op;
		if (op == 1)
			runEvolution(world_path);
		else if (op == 2)
			runSimulation(world_path);
	}
}