#pragma once

#include <vector>

#include "../../Utils/Math/Float2.h"
#include "../../Evolution/Globals.h"
#include "../../Evolution/Individual.h"
#include "../../Evolution/Population.h"

using namespace std;
using namespace agio;

struct OrgState
{
    float Life = 100;
    float2 Position;
};

enum class ParametersIDs
{
    JumpDistance,
    MoveGrassWaterRatio,
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
    NearestCompetitorDeltaX,
    NearestCompetitorDeltaY,
    NearestCompetitorAlive,

    NearestFoodDeltaX,
    NearestFoodDeltaY,

    NumberOfSensors
};

enum class CellType
{
    Ground,
    Water,
    Wall
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

    // How many steps did the last simulation took before everyone died
    int LastSimulationStepCount = 0;

    // This computes fitness for the entire population
    void ComputeFitness(const std::vector<class BaseIndividual*>& Individuals, void * World) override;
};