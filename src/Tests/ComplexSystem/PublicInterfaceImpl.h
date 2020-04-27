#pragma once

#include <vector>

#include "../../Utils/Math/Float2.h"
#include "../../Evolution/Globals.h"
#include "../../Evolution/Individual.h"
#include "../../Evolution/Population.h"

using namespace std;
using namespace agio;

enum class OrgType { Carnivore, Herbivore, Omnivore };

struct OrgState
{
    float Life = -1;
    float2 Position;

	// Score = fitness
	float Score = 0;

	// Metrics
    // This metrics are only used during simulation, so we don't care about resetting the value across replications
	int EatenCount = 0;
    int FailedCount = 0;
    int FailableCount = 0;

    bool Enabled = true; // Used for selectively ignoring organisms on the interrelations test

	OrgType Type;
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

    SwimMoveForward,
    SwimMoveBackwards,
    SwimMoveLeft,
    SwimMoveRight,

    JumpForward,
    JumpBackwards,
    JumpLeft,
    JumpRight,

    Kill,
    EatFood,
    EatEnemy,
    EatFoodEnemy,

    NumberOfActions
};

enum class SensorsIDs
{
	NearestCompetidorDeltaX,
	NearestCompetidorDeltaY,

    NearestCorpseDeltaX,
    NearestCorpseDeltaY,

    NearestFoodDeltaX,
    NearestFoodDeltaY,

	CurrentCell,
	TopCell,
	DownCell,
	RightCell,
	LeftCell,

    NumberOfSensors
};

enum class CellType
{
    Ground,
    Water,
    Wall,

	NumberOfTypes
};

struct WorldData
{
    vector<float2> FoodPositions;
    vector<vector<CellType>> CellTypes;
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

    void* MakeWorld(void* BaseWorld) override;

    void DestroyWorld(void* World) override { delete (WorldData*)World; }

    // How many steps did the last simulation took before everyone died
    int LastSimulationStepCount = 0;

    // This computes fitness for the entire population
    void ComputeFitness(const std::vector<class BaseIndividual*>& Individuals, void * World) override;
};