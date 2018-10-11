#pragma once
#include "../../Evolution/Globals.h"
#include "../../Utils/Math/Float2.h"
#include <vector>
#include <random>
#include "Public.h"

namespace agio
{
	class ExperimentInterface : public PublicInterface
	{
		std::minstd_rand0 RNG;
	public:
		ExperimentInterface();

		WorldData World;

		virtual ~ExperimentInterface() override {};

		virtual void Init() override;
		virtual void * MakeState(const Individual * org) override;
		virtual void ResetState(void * State) override;
		virtual void DestroyState(void * State) override;
		virtual void * DuplicateState(void * State) override;
		virtual void ComputeFitness(Population * Pop, void * World) override;

		// TODO : Remove this
		virtual float Distance(class Individual *, class Individual *, void * World) override
		{
			assert(false);
			return 0; // Not used
		}
	};

	struct ExperimentParams
	{
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
		const int GenerationsCount = 150;
		const float LifeLostPerTurn = 5;
		const float BorderPenalty = 0;// 80; // penalty when trying to go out of bounds
		const float WastedActionPenalty = 0;// 5; // penalty when doing an action that has no valid target (like eating and there's no food close)
	};
}
