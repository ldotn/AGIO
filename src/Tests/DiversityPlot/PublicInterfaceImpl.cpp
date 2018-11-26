#include "PublicInterfaceImpl.h"
#include "DiversityPlot.h"
#include <enumerate.h>
#include <assert.h>

using namespace fpp;
using namespace std;
using namespace agio;

int cycle_x(int x)
{
    return abs(x) % WorldSizeX;
};

int cycle_y(int y)
{
    return abs(y) % WorldSizeY;
};

float2 random_valid_position(WorldData *world_ptr)
{
    minstd_rand RNG = minstd_rand(chrono::high_resolution_clock::now().time_since_epoch().count());
    bool valid_position = false;
    float x, y;
    while (!valid_position)
    {
        x = uniform_real_distribution<int>(0, WorldSizeX)(RNG);
        y = uniform_real_distribution<int>(0, WorldSizeX)(RNG);

        valid_position = world_ptr->CellTypes[y][x] != CellType::Wall;
    }

    return float2(x, y);
}

void move(int sum_x, int sum_y, void *State, void *World, bool swim)
{
    auto state_ptr = (OrgState *) State;
    auto world_ptr = (WorldData *) World;

    int x = cycle_x(state_ptr->Position.x + sum_x);
    int y = cycle_y(state_ptr->Position.y + sum_y);
    CellType cellType = world_ptr->CellTypes[y][x];

    if (cellType != CellType::Wall)
    {
        state_ptr->Position.x = x;
        state_ptr->Position.y = y;

        if (!swim && cellType == CellType::Water)
            state_ptr->Life -= WaterPenalty;
    }

};

int find_nearest_food(void *State, void *World)
{
    auto world_ptr = (WorldData *) World;
    auto state_ptr = (OrgState *) State;

    float nearest_dist = numeric_limits<float>::max();
    int nearest_idx = 0;
    for (auto[idx, pos] : enumerate(world_ptr->FoodPositions))
    {
        float dist = (pos - state_ptr->Position).length_sqr();
        // Minimum distance is 0. Impossible to find any closer.
        if (dist == 0)
            return idx;

        if (dist < nearest_dist)
        {
            nearest_dist = dist;
            nearest_idx = idx;
        }
    }

    return nearest_idx;
}

BaseIndividual *find_nearest_enemy(const void *State, const BaseIndividual *Org,
                                   const vector<BaseIndividual *> &Individuals, bool looking_enemy_alive,
                                   bool looking_for_both = false)
{
    auto state_ptr = (OrgState *) State;
    const auto &tag = Org->GetMorphologyTag();

    float nearest_dist = numeric_limits<float>::max();
    BaseIndividual *nearest_enemy = nullptr;
    // Find an individual of a DIFFERENT species.
    // The species map is not really useful here
    for (const auto &individual : Individuals)
    {
        auto other_state_ptr = (OrgState *) individual->GetState();

        // Ignore individuals that aren't being simulated right now or those of your species
        bool equal_species = tag == individual->GetMorphologyTag();
        if (!individual->InSimulation || equal_species)
            continue;

        if (!looking_for_both)
        {
            bool other_is_alive = other_state_ptr->Life > 0;
            if ((looking_enemy_alive && !other_is_alive) || (!looking_enemy_alive && other_is_alive))
                continue;
        }

        float dist = (other_state_ptr->Position - state_ptr->Position).length_sqr();
        if (dist == 0)
            return individual;

        if (dist < nearest_dist)
        {
            nearest_dist = dist;
            nearest_enemy = individual;
        }
    }
    return nearest_enemy;
}

bool eat_food(void *State, void *World)
{
    auto world_ptr = (WorldData *) World;
    auto state_ptr = (OrgState *) State;

    int idx = find_nearest_food(State, World);
    float2 pos = world_ptr->FoodPositions[idx];

    float dist = (pos - state_ptr->Position).length_sqr();
    if (dist <= 1)
    {
        world_ptr->FoodPositions[idx] = random_valid_position(world_ptr);
        return true;
    }

    return false;
}

bool eat_enemy(void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
{
    bool enemy_alive = false;
    BaseIndividual *enemy = find_nearest_enemy(State, Org, Individuals, enemy_alive);
    return enemy != nullptr;
}


void PublicInterfaceImpl::Init()
{
    ActionRegistry.resize((int) ActionsIDs::NumberOfActions);
    // Basic movement
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

    ActionRegistry[(int) ActionsIDs::JumpForward] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
                        int jump_dist = round(param_iter->second.Value);

                        move(0, jump_dist, State, World, false);
                    }
            );

    ActionRegistry[(int) ActionsIDs::JumpBackwards] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
                        int jump_dist = round(param_iter->second.Value);

                        move(0, -jump_dist, State, World, false);
                    }
            );

    ActionRegistry[(int) ActionsIDs::JumpRight] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
                        int jump_dist = round(param_iter->second.Value);

                        move(jump_dist, 0, State, World, false);
                    }
            );

    ActionRegistry[(int) ActionsIDs::JumpLeft] = Action
            (
                    [&](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
                        int jump_dist = round(param_iter->second.Value);

                        move(-jump_dist, 0, State, World, false);
                    }
            );

    // Eat any food that's at most one cell away
    ActionRegistry[(int) ActionsIDs::EatFood] = Action
            (
                    [this](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        bool any_eaten = eat_food(State, World);

                        auto state_ptr = (OrgState *) State;
                        if (any_eaten)
                            state_ptr->Life += FoodScoreGain;
                        else
                            state_ptr->Life -= WastedActionPenalty;
                    }
            );

    // Eat any enemy that's at most one cell away
    ActionRegistry[(int) ActionsIDs::EatEnemy] = Action
            (
                    [this](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        bool any_eaten = eat_enemy(State, Individuals, Org, World);

                        auto state_ptr = (OrgState *) State;
                        if (any_eaten)
                            // TODO: Set different score for eat_enemy
                            state_ptr->Life += FoodScoreGain;
                        else
                            state_ptr->Life -= WastedActionPenalty;
                    }
            );

    // Eat any enemy that's at most one cell away
    ActionRegistry[(int) ActionsIDs::EatFoodEnemy] = Action
            (
                    [this](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        auto state_ptr = (OrgState *) State;

                        if (eat_food(State, World))
                            state_ptr->Life += FoodScoreGain;
                        else if (eat_enemy(State, Individuals, Org, World))
                            // TODO: Set different score for eat_enemy
                            state_ptr->Life += FoodScoreGain;
                        else
                            state_ptr->Life -= WastedActionPenalty;
                    }
            );

    // Kill and eat and individual of a different species at most once cell away
    // This is a simple test, so no fighting is simulated
    ActionRegistry[(int) ActionsIDs::Kill] = Action
            (
                    [this](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        auto state_ptr = (OrgState *) State;

                        BaseIndividual *enemy = find_nearest_enemy(State, Org, Individuals, true);
                        if (enemy)
                        {
                            auto other_state_ptr = (OrgState *) enemy->GetState();

                            float dist = (other_state_ptr->Position - state_ptr->Position).length_sqr();
                            if (dist < 1)
                                other_state_ptr->Life -= DeathPenalty;
                            else
                                state_ptr->Score -= WastedActionPenalty;
                        } else
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

                        int idx = find_nearest_food(State, World);
                        float2 pos = world_ptr->FoodPositions[idx];

                        // Compute "angle", taking as 0 = looking forward ([0,1])
                        // The idea is: angle = normalize(V - Pos) . [0,1]
                        return (pos - state_ptr->Position).normalize().y;
                    }
            );

    SensorRegistry[(int) SensorsIDs::NearestFoodDistance] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        auto world_ptr = (WorldData *) World;
                        auto state_ptr = (OrgState *) State;

                        int idx = find_nearest_food(State, World);
                        float2 pos = world_ptr->FoodPositions[idx];

                        return (pos - state_ptr->Position).length_sqr();
                    }
            );

    SensorRegistry[(int) SensorsIDs::NearestCompetitorAngle] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        BaseIndividual* nearest_competitor = find_nearest_enemy(State, Org, Individuals, true, true);

                        auto state_ptr = (OrgState *) State;
                        auto other_state_ptr = (OrgState *) nearest_competitor->GetState();

                        // Compute "angle", taking as 0 = looking forward ([0,1])
                        // The idea is
                        //	angle = normalize(V - Pos) . [0,1]
                        return (other_state_ptr->Position - state_ptr->Position).normalize().y;
                    }
            );


    SensorRegistry[(int) SensorsIDs::NearestCompetitorDistance] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        BaseIndividual* nearest_competitor = find_nearest_enemy(State, Org, Individuals, true, true);

                        auto state_ptr = (OrgState *) State;
                        auto other_state_ptr = (OrgState *) nearest_competitor->GetState();

                        return (other_state_ptr->Position - state_ptr->Position).length_sqr();
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

    ComponentRegistry.push_back
            ({
                     0, 1, // Ability of see competitors
                     {
                             {
                                     {
                                             {},
                                             {
                                                     (int) SensorsIDs::NearestCompetitorAngle,
                                                     (int) SensorsIDs::NearestCompetitorDistance
                                             }
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