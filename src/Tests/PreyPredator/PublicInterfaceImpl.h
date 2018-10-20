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
    float Score = 0;
    float2 Position;

    bool IsCarnivore;
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
    JumpForward,
    JumpBackwards,
    JumpLeft,
    JumpRight,
    EatFood,
    KillAndEat,

    NumberOfActions
};

enum class SensorsIDs
{
    CurrentLife,
    NearestPartnerAngle,
    NearestCompetidorAngle, // Angle to the nearest individual of another species
    NearestFoodAngle,
    NearestPartnerDistance,
    NearestCompetidorDistance,
    NearestFoodDistance,

    NearestPartnerX,
    NearestPartnerY,
    NearestCompetidorX,
    NearestCompetidorY,
    NearestFoodX,
    NearestFoodY,

    CurrentPosX,
    CurrentPosY,

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

    void * MakeState(const Individual * org) override;

    void ResetState(void * State) override;

    void DestroyState(void * State) override;

    float Distance(class Individual *, class Individual *, void * World) override;

    void * DuplicateState(void * State) override;

    // How many steps did the last simulation took before everyone died
    int LastSimulationStepCount = 0;

    // This computes fitness for the entire population
    void ComputeFitness(Population * Pop, void * World) override;
};