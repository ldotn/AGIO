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

#include <memory>
void LoadConfig();

namespace agio { class Population; }
std::unique_ptr<agio::Population> runEvolution();
void runSimulation();