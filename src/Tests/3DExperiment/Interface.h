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
			return 0; // Not used
		}
	};

	struct ExperimentParams
	{
		// TODO : Load this from a file or something. Maybe...
		inline static const int MaxSimulationSteps = 50;
		inline static const int PopSizeMultiplier = 4;
		inline static const int SimulationSize = 50;
		inline static const int GenerationsCount = 1000;
	};
}
