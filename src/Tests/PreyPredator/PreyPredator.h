#pragma once

#include <string>

extern int WorldSizeX;
extern int WorldSizeY;
extern float FoodScoreGain;
extern float KillScoreGain;
extern float DeathPenalty;
extern int FoodCellCount;
extern int MaxSimulationSteps;
extern int SimulationSize;
extern int PopSizeMultiplier;
extern int PopulationSize;
extern int GenerationsCount;
extern float LifeLostPerTurn;
extern float BorderPenalty;
extern float WastedActionPenalty;
extern std::string SerializationFile;

#include "../../Evolution/Population.h"

void LoadConfig();
agio::Population runEvolution();
void runSimulation();