#pragma once

#include <string>
#include "PublicInterfaceImpl.h"

extern int WorldSizeX;
extern int WorldSizeY;
extern float FoodScoreGain;
extern float KillScoreGain;
extern float DeathPenalty;
extern float FoodCellProportion;
extern int MaxSimulationSteps;
extern int SimulationSize;
extern int PopSizeMultiplier;
extern int PopulationSize;
extern int GenerationsCount;
extern float LifeLostPerTurn;
extern float BorderPenalty;
extern float WastedActionPenalty;
extern float WaterPenalty;
extern std::string SerializationFile;
extern int InitialLife;

void runEvolution();
void runSimulation();

WorldData createWorld();