#pragma once

#include <string>
#include "PublicInterfaceImpl.h"

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

void runEvolution();
void runSimulation();

WorldData createWorld();