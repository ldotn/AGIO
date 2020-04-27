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

#include <memory>
std::unique_ptr<class agio::Population> runEvolution(const std::string& WorldPath, bool NoOutput = false);
void runSimulation(const std::string& WorldPath);

WorldData createWorld(const std::string& WorldPath);