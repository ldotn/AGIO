#include <iostream>
#include "PreyPredator.h"
#include "../../Core/Config.h"

using namespace agio;
using namespace std;

// Need to define them somewhere
int WorldSizeX;
int WorldSizeY;
float FoodScoreGain;
float KillScoreGain;
float DeathPenalty;
int FoodCellCount;
int MaxSimulationSteps;
int SimulationSize;
int PopSizeMultiplier;
int PopulationSize;
int GenerationsCount;
float LifeLostPerTurn;
float BorderPenalty;
float WastedActionPenalty;

int main() {
	ConfigLoader loader("../src/Tests/PreyPredator/Config.cfg");
	loader.LoadValue(WorldSizeX,"WorldSizeX");
	loader.LoadValue(WorldSizeY,"WorldSizeY");
	loader.LoadValue(FoodScoreGain,"FoodScoreGain");
	loader.LoadValue(KillScoreGain,"KillScoreGain");
	loader.LoadValue(DeathPenalty,"DeathPenalty");
	float food_proportion;
	loader.LoadValue(food_proportion,"FoodProportion");
	FoodCellCount = WorldSizeX * WorldSizeY*food_proportion;
	loader.LoadValue(MaxSimulationSteps,"MaxSimulationSteps");
	loader.LoadValue(SimulationSize,"SimulationSize");
	loader.LoadValue(PopSizeMultiplier,"PopSizeMultiplier");
	PopulationSize = PopSizeMultiplier * SimulationSize;
	loader.LoadValue(GenerationsCount,"GenerationsCount");
	loader.LoadValue(LifeLostPerTurn,"LifeLostPerTurn");
	loader.LoadValue(BorderPenalty,"BorderPenalty");
	loader.LoadValue(WastedActionPenalty,"WastedActionPenalty");

    cout << "1 - Run Evolution" << endl;
    cout << "2 - Run Simulation" << endl;
    cout << "Select an option:";

    int op;
    cin >> op;
    if (op == 1)
        runEvolution();
    if (op == 2)
        runSimulation();

}