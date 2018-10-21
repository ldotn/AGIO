#pragma once

// TODO : Refactor, this could be spread on a couple files.
//   Need to see if that's actually better though
const int WorldSizeX = 50;
const int WorldSizeY = 50;
const float FoodScoreGain = 20;
const float KillScoreGain = 30;
const float DeathPenalty = 0;// 400;
const int FoodCellCount = WorldSizeX * WorldSizeY*0.05;
const int MaxSimulationSteps = 50;
const int SimulationSize = 10; // Population is simulated in batches
const int PopSizeMultiplier = 30; // Population size is a multiple of the simulation size
const int PopulationSize = PopSizeMultiplier * SimulationSize;
const int GenerationsCount = 5;
const float LifeLostPerTurn = 5;
const float BorderPenalty = 0;// 80; // penalty when trying to go out of bounds
const float WastedActionPenalty = 0;// 5; // penalty when doing an action that has no valid target (like eating and there's no food close)