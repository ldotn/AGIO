#include "PublicInterfaceImpl.h"
#include "DiversityPlot.h"
#include <enumerate.h>
#include <assert.h>

using namespace fpp;
using namespace std;
using namespace agio;

int cycle_x (int x)
{
    return abs(x) % WorldSizeX;
};

int cycle_y (int y)
{
    return abs(y) % WorldSizeY;
};

void move(int sum_x, int sum_y, void *State, void *World, bool swim)
{
    auto state_ptr = (OrgState *) State;
    auto worldData = (WorldData *) World;

    int x = cycle_x(state_ptr->Position.x + sum_x);
    int y = cycle_y(state_ptr->Position.y + sum_y);
    CellType cellType = worldData->CellTypes[y][x];

    if (cellType != CellType::Wall)
    {
        state_ptr->Position.x = x;
        state_ptr->Position.y = y;

        if (!swim && cellType == CellType::Water)
            state_ptr->Life -= WaterPenalty;
    }

};

bool eatFood(void *State, void *World)
{
    minstd_rand RNG = minstd_rand(chrono::high_resolution_clock::now().time_since_epoch().count());
    auto world_ptr = (WorldData *) World;
    auto state_ptr = (OrgState *) State;

    bool any_eaten = false;
    for (auto[idx, pos] : enumerate(world_ptr->FoodPositions))
    {
        auto diff = abs >> (pos - state_ptr->Position);

        if (diff.x <= 1 && diff.y <= 1)
        {
            state_ptr->Score += FoodScoreGain;

            // Just move the food to a different position. Achieves the same effect as removing it
            // TODO: Check that the position isn't a wall
            world_ptr->FoodPositions[idx] = {uniform_real_distribution<float>(0, WorldSizeX)(RNG),
                                             uniform_real_distribution<float>(0, WorldSizeY)(RNG)};
            any_eaten = true;
            break;
        }
    }

    return any_eaten;
}

bool eatEnemy(void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
{
    minstd_rand RNG = minstd_rand(chrono::high_resolution_clock::now().time_since_epoch().count());
    auto state_ptr = (OrgState *) State;
    const auto &tag = Org->GetMorphologyTag();

    // Find an individual of a DIFFERENT species.
    // The species map is not really useful here
    bool any_eaten = false;
    for (const auto &individual : Individuals)
    {
        // Ignore individuals that aren't being simulated right now
        // Also, don't do all the other stuff against yourself.
        // You already know you don't want to kill yourself
        if (!individual->InSimulation || individual == Org)
            continue;

        auto other_state_ptr = (OrgState *) individual->GetState();
        // Can't eat an enemy alive
        if (other_state_ptr->Life > 0)
            continue;

        auto diff = abs >> (other_state_ptr->Position - state_ptr->Position);
        if (diff.x <= 1 && diff.y <= 1)
        {
            // Check if they are from different species by comparing tags
            const auto &other_tag = individual->GetMorphologyTag();

            if (!(tag == other_tag))
            {
                any_eaten = true;
                break;
            }
        }
    }
    return any_eaten;
}



void PublicInterfaceImpl::Init()
{
    ActionRegistry.resize((int) ActionsIDs::NumberOfActions);

    // Basic movement
    // The world is circular, doing this so that % works with negative numbers



    ActionRegistry[(int) ActionsIDs::MoveForward] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        move(0, 1, State, World, false);
                    }
            );

    ActionRegistry[(int) ActionsIDs::MoveBackwards] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        move(0, -1, State, World, false);
                    }
            );

    ActionRegistry[(int) ActionsIDs::MoveRight] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        move(1, 0, State, World, false);
                    }
            );

    ActionRegistry[(int) ActionsIDs::MoveLeft] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        move(-1, 0, State, World, false);
                    }
            );

    ActionRegistry[(int) ActionsIDs::SwimMoveForward] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        move(0, 1, State, World, true);
                    }
            );

    ActionRegistry[(int) ActionsIDs::SwimMoveBackwards] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        move(0, -1, State, World, true);
                    }
            );

    ActionRegistry[(int) ActionsIDs::SwimMoveRight] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        move(1, 0, State, World, true);
                    }
            );

    ActionRegistry[(int) ActionsIDs::SwimMoveLeft] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        move(-1, 0, State, World, true);
                    }
            );

    // Eat any food that's at most one cell away
    ActionRegistry[(int) ActionsIDs::EatFood] = Action
            (
                    [this](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        bool any_eaten = eatFood(State, World);

                        if (!any_eaten)
                            ((OrgState *) State)->Score -= WastedActionPenalty;
                    }
            );

    // Eat any enemy that's at most one cell away
    ActionRegistry[(int) ActionsIDs::EatEnemy] = Action
            (
                    [this](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        bool any_eaten = eatEnemy(State, Individuals, Org, World);

                        if (!any_eaten)
                            ((OrgState *) State)->Score -= WastedActionPenalty;
                    }
            );

    // Eat any enemy that's at most one cell away
    ActionRegistry[(int) ActionsIDs::EatFoodEnemy] = Action
            (
                    [this](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        bool any_eaten = eatEnemy(State, Individuals, Org, World);

                        if (!any_eaten)
                            ((OrgState *) State)->Score -= WastedActionPenalty;
                    }
            );

    // Kill and eat and individual of a different species at most once cell away
    // This is a simple test, so no fighting is simulated
    ActionRegistry[(int) ActionsIDs::Kill] = Action
            (
                    [this](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        auto state_ptr = (OrgState *) State;


                        const auto &tag = Org->GetMorphologyTag();

                        // Find an individual of a DIFFERENT species.
                        // The species map is not really useful here
                        bool any_eaten = false;
                        for (const auto &individual : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            // You already know you don't want to kill yourself
                            if (!individual->InSimulation || individual == Org)
                                continue;

                            auto other_state_ptr = (OrgState *) individual->GetState();
                            auto diff = abs >> (other_state_ptr->Position - state_ptr->Position);
                            if (diff.x <= 1 && diff.y <= 1)
                            {
                                // Check if they are from different species by comparing tags
                                const auto &other_tag = individual->GetMorphologyTag();

                                if (!(tag == other_tag))
                                {
                                    other_state_ptr->Life -= DeathPenalty;
                                    other_state_ptr->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
                                    other_state_ptr->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
                                    any_eaten = true;
                                    break;
                                }
                            }
                        }

                        if (!any_eaten)
                            state_ptr->Score -= WastedActionPenalty;
                    }
            );

    // Fill sensors
    SensorRegistry.resize((int) SensorsIDs::NumberOfSensors);

    SensorRegistry[(int) SensorsIDs::NearestFoodAngle] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        auto world_ptr = (WorldData *) World;
                        auto state_ptr = (OrgState *) State;

                        float nearest_dist = numeric_limits<float>::max();
                        float2 nearest_pos;
                        for (auto pos : world_ptr->FoodPositions)
                        {
                            float dist = (pos - state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                            {
                                nearest_dist = dist;
                                nearest_pos = pos;
                            }
                        }

                        // Compute "angle", taking as 0 = looking forward ([0,1])
                        // The idea is
                        //	angle = normalize(V - Pos) . [0,1]
                        return (nearest_pos - state_ptr->Position).normalize().y;
                    }
            );

    SensorRegistry[(int) SensorsIDs::NearestFoodDistance] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        auto world_ptr = (WorldData *) World;
                        auto state_ptr = (OrgState *) State;

                        float nearest_dist = numeric_limits<float>::max();
                        for (auto pos : world_ptr->FoodPositions)
                        {
                            float dist = (pos - state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                                nearest_dist = dist;
                        }

                        return nearest_dist;
                    }
            );

    SensorRegistry[(int) SensorsIDs::NearestCompetitorAngle] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        const auto &tag = Org->GetMorphologyTag();
                        auto state_ptr = (OrgState *) State;
                        float nearest_dist = numeric_limits<float>::max();
                        float2 nearest_pos;

                        for (const auto &other_org : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            if (!other_org->InSimulation || other_org == Org)
                                continue;

                            float dist = (((OrgState *) other_org->GetState())->Position -
                                          state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                            {
                                const auto &other_tag = other_org->GetMorphologyTag();

                                if (!(tag == other_tag))
                                {
                                    nearest_dist = dist;
                                    nearest_pos = ((OrgState *) other_org->GetState())->Position;
                                }
                            }
                        }

                        // Compute "angle", taking as 0 = looking forward ([0,1])
                        // The idea is
                        //	angle = normalize(V - Pos) . [0,1]
                        return (nearest_pos - state_ptr->Position).normalize().y;
                    }
            );


    SensorRegistry[(int) SensorsIDs::NearestCompetitorDistance] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        const auto &tag = Org->GetMorphologyTag();
                        auto state_ptr = (OrgState *) State;
                        float nearest_dist = numeric_limits<float>::max();

                        for (const auto &other_org : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            if (!other_org->InSimulation || other_org == Org)
                                continue;

                            float dist = (((OrgState *) other_org->GetState())->Position -
                                          state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                            {
                                const auto &other_tag = other_org->GetMorphologyTag();

                                if (!(tag == other_tag))
                                    nearest_dist = dist;
                            }
                        }

                        return nearest_dist;
                    }
            );

    // Fill parameters
    ParameterDefRegistry.resize((int) ParametersIDs::NumberOfParameters);

    ParameterDefRegistry[(int) ParametersIDs::JumpDistance] = ParameterDefinition
            (
                    true, // require the parameter, even if the organism doesn't have the component to jump
                    2, // Min bound
                    5, // max bound
                    (int) ParametersIDs::JumpDistance
            );
    ParameterDefRegistry[(int) ParametersIDs::MoveGrassWaterRatio] = ParameterDefinition
            (
                    true, // require the parameter, even if the organism doesn't have the component to jump
                    0, // Min bound
                    1, // max bound
                    (int) ParametersIDs::MoveGrassWaterRatio
            );

    // Fill components
    ComponentRegistry.push_back
            ({
                     1, 1, // Can only have one mouth
                     {
                             // Herbivore
                             {
                                     {(int) ActionsIDs::EatFood},
                                     {
                                             (int) SensorsIDs::NearestFoodAngle,
                                             (int) SensorsIDs::NearestFoodDistance,
                                     }
                             },
                             // Carnivore
                             {
                                     {(int) ActionsIDs::EatEnemy},
                                     {
                                             (int) SensorsIDs::NearestCompetitorAngle,
                                             (int) SensorsIDs::NearestCompetitorDistance,
                                             (int) SensorsIDs::NearestCompetitorAlive
                                     }
                             },
                             // Herbivore and Carnivore
                             {
                                     {(int) ActionsIDs::EatFoodEnemy},
                                     {
                                             (int) SensorsIDs::NearestFoodAngle,
                                             (int) SensorsIDs::NearestFoodDistance,
                                             (int) SensorsIDs::NearestCompetitorAngle,
                                             (int) SensorsIDs::NearestCompetitorDistance,
                                             (int) SensorsIDs::NearestCompetitorAlive
                                     }
                             },
                     }
             });

    ComponentRegistry.push_back
            ({
                     1, 1, // Can have a only ground body or an amphibian body
                     {
                             // There's only one body to choose
                             {
                                     {
                                             (int) ActionsIDs::MoveForward,
                                             (int) ActionsIDs::MoveBackwards,
                                             (int) ActionsIDs::MoveLeft,
                                             (int) ActionsIDs::MoveRight,
                                     }
                             },
                             {
                                     {
                                             (int) ActionsIDs::SwimMoveForward,
                                             (int) ActionsIDs::SwimMoveBackwards,
                                             (int) ActionsIDs::SwimMoveLeft,
                                             (int) ActionsIDs::SwimMoveRight,
                                     }
                             }
                     }
             });

    ComponentRegistry.push_back
            ({
                     0, 1, // Ability of jump
                     {
                             {
                                     {
                                             (int) ActionsIDs::JumpForward,
                                             (int) ActionsIDs::JumpBackwards,
                                             (int) ActionsIDs::JumpLeft,
                                             (int) ActionsIDs::JumpRight,
                                     }
                             }
                     }
             });
}

void *PublicInterfaceImpl::MakeState(const BaseIndividual *org)
{
    auto state = new OrgState;

    state->Score = 0;
    state->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
    state->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);

    // check if can swim


    return state;
}

void PublicInterfaceImpl::ResetState(void *State)
{
    auto state_ptr = (OrgState *) State;

    state_ptr->Score = 0;
    state_ptr->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
    state_ptr->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
}

void PublicInterfaceImpl::DestroyState(void *State)
{
    auto state_ptr = (OrgState *) State;
    delete state_ptr;
}

void *PublicInterfaceImpl::DuplicateState(void *State)
{
    auto new_state = new OrgState;
    *new_state = *(OrgState *) State;
    return new_state;

}

void PublicInterfaceImpl::ComputeFitness(const std::vector<class BaseIndividual *> &Individuals, void *World)
{
    for (auto &org : Individuals)
        ((Individual *) org)->Reset();

    LastSimulationStepCount = 0;
    for (int i = 0; i < MaxSimulationSteps; i++)
    {
        for (auto &org : Individuals)
        {
            if (!org->InSimulation)
                continue;

            auto state_ptr = (OrgState *) org->GetState();

            org->DecideAndExecute(World, Individuals);

            ((Individual *) org)->Fitness = state_ptr->Score;
        }
        LastSimulationStepCount++;
    }
}