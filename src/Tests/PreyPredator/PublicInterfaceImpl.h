#pragma once

#include <unordered_set>
#include <vector>

#include "../../Utils/Math/Float2.h"
#include "../../Evolution/Globals.h"
#include "../../Evolution/Individual.h"
#include "../../Evolution/Population.h"

using namespace std;
using namespace agio;

struct OrgState
{
    float Score = 0;
    float2 Position;

    bool IsCarnivore;

	int FailedActionCount;
	int EatenCount;
	int VisitedCellsCount;
	int Repetitions; // Divide the metrics by this to get the average values (the real metrics, otherwise it's the sum)
	int MetricsCurrentGenNumber;

	struct pair_hash
	{
		inline size_t operator()(const pair<int, int> & v) const
		{
			return v.first ^ v.second;
		}
	};

	unordered_set<pair<int, int>,pair_hash> VisitedCells;
};

enum class ParametersIDs
{
    JumpDistance,

    NumberOfParameters
};

enum class ActionsIDs
{
    MoveForward,
    MoveBackwards,
    MoveLeft,
    MoveRight,

    EatFood,
    KillAndEat,

    NumberOfActions
};

enum class SensorsIDs
{
    NearestCompetidorAngle, // Angle to the nearest individual of another species
    NearestCompetidorDistance,

    NearestFoodAngle,
    NearestFoodDistance,

    NumberOfSensors
};

struct WorldData
{
    vector<float2> FoodPositions;
};

class PublicInterfaceImpl : public PublicInterface
{
private:
    minstd_rand RNG = minstd_rand(chrono::high_resolution_clock::now().time_since_epoch().count());
public:
    virtual ~PublicInterfaceImpl() override {};

    void Init() override;

    void * MakeState(const BaseIndividual * org) override;

    void ResetState(void * State) override;

    void DestroyState(void * State) override;

    void * DuplicateState(void * State) override;

	// Needs to be set BEFORE calling epoch
	// Used by the evaluation metrics
	int CurrentGenNumber = 0;

    // How many steps did the last simulation took before everyone died
    int LastSimulationStepCount = 0;

    // This computes fitness for the entire population
    void ComputeFitness(const std::vector<class BaseIndividual*>& Individuals, void * World) override;
};