#include <iostream>
#include "ExperimentConfig.h"
#include "../../Core/Config.h"
#include "PreyPredatorEvolution.cpp"
#include "PreyPredator.cpp"

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
	ConfigLoader loader("../Tests/PreyPredator/Config.cfg");
	loader.LoadValue(WorldSizeX,"WorldSizeX");
	loader.LoadValue(WorldSizeY,"WorldSizeY");
	loader.LoadValue(FoodScoreGain,"FoodScoreGain");
	loader.LoadValue(KillScoreGain,"KillScoreGain");
	loader.LoadValue(DeathPenalty,"DeathPenalty");
	loader.LoadValue(FoodCellCount,"FoodCellCount");
	loader.LoadValue(MaxSimulationSteps,"MaxSimulationSteps");
	loader.LoadValue(SimulationSize,"SimulationSize");
	loader.LoadValue(PopSizeMultiplier,"PopSizeMultiplier");
	loader.LoadValue(PopulationSize,"PopulationSize");
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