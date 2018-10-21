#include "PublicInterfaceImpl.h"
#include "Config.h"
#include <enumerate.h>

using namespace fpp;
using namespace std;
using namespace agio;

void PublicInterfaceImpl::Init()
{
    ActionRegistry.resize((int)ActionsIDs::NumberOfActions);

    // Basic movement
    // The world is circular, doing this so that % works with negative numbers
    auto cycle_x = [](int x) { return ((x % WorldSizeX) + WorldSizeX) % WorldSizeX; };
    auto cycle_y = [](int y) { return ((y % WorldSizeY) + WorldSizeY) % WorldSizeY; };

    // Test : Instead of a circular world, make them bounce on walls
    //auto cycle_x = [](int x) { return x < WorldSizeX ? (x > 0 ? x : 2) : WorldSizeX - 2; };
    //auto cycle_y = [](int y) { return y < WorldSizeY ? (y > 0 ? y : 2) : WorldSizeY - 2; };


    ActionRegistry[(int)ActionsIDs::MoveForward] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.y >= WorldSizeY - 1)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.y = cycle_y(state_ptr->Position.y + 1);//clamp<int>(state_ptr->Position.y + 1, 0, WorldSizeY - 1);

                        /*if (state_ptr->IsCarnivore)
                            ActionRegistry[(int)ActionsIDs::KillAndEat].Execute(State, Pop, Org, World);
                        else
                            ActionRegistry[(int)ActionsIDs::EatFood].Execute(State, Pop, Org, World);*/
                    }
            );
    ActionRegistry[(int)ActionsIDs::MoveBackwards] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.y <= 1)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.y = cycle_y(state_ptr->Position.y - 1);//clamp<int>(state_ptr->Position.y - 1, 0, WorldSizeY - 1);

                        /*if (state_ptr->IsCarnivore)
                            ActionRegistry[(int)ActionsIDs::KillAndEat].Execute(State, Pop, Org, World);
                        else
                            ActionRegistry[(int)ActionsIDs::EatFood].Execute(State, Pop, Org, World);*/
                    }
            );
    ActionRegistry[(int)ActionsIDs::MoveRight] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.x >= WorldSizeX)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.x = cycle_x(state_ptr->Position.x + 1);//clamp<int>(state_ptr->Position.x + 1, 0, WorldSizeX - 1);

                        /*if (state_ptr->IsCarnivore)
                            ActionRegistry[(int)ActionsIDs::KillAndEat].Execute(State, Pop, Org, World);
                        else
                            ActionRegistry[(int)ActionsIDs::EatFood].Execute(State, Pop, Org, World);*/
                    }
            );
    ActionRegistry[(int)ActionsIDs::MoveLeft] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.x <= 1)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.x = cycle_x(state_ptr->Position.x - 1);// clamp<int>(state_ptr->Position.x - 1, 0, WorldSizeX - 1);

                        /*if (state_ptr->IsCarnivore)
                            ActionRegistry[(int)ActionsIDs::KillAndEat].Execute(State, Pop, Org, World);
                        else
                            ActionRegistry[(int)ActionsIDs::EatFood].Execute(State, Pop, Org, World);*/
                    }
            );

    // "Jump". Move more than one cell at a time. Jump distance is a parameter
    ActionRegistry[(int)ActionsIDs::JumpForward] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
                        assert(param_iter != Org->GetParameters().end());

                        auto state_ptr = (OrgState*)State;

                        auto [_, jump_dist] = *param_iter;

                        state_ptr->Position.y = cycle_y(round(state_ptr->Position.y + jump_dist.Value));//clamp<int>((int)round(state_ptr->Position.y + jump_dist.Value), 0, WorldSizeY - 1);
                    }
            );
    ActionRegistry[(int)ActionsIDs::JumpBackwards] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
                        assert(param_iter != Org->GetParameters().end());

                        auto state_ptr = (OrgState*)State;

                        auto [_, jump_dist] = *param_iter;
                        state_ptr->Position.y = cycle_y(round(state_ptr->Position.y - jump_dist.Value));// clamp<int>((int)round(state_ptr->Position.y - jump_dist.Value), 0, WorldSizeY - 1);
                    }
            );
    ActionRegistry[(int)ActionsIDs::JumpLeft] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
                        assert(param_iter != Org->GetParameters().end());

                        auto state_ptr = (OrgState*)State;

                        auto [_, jump_dist] = *param_iter;
                        state_ptr->Position.x = cycle_x(round(state_ptr->Position.x - jump_dist.Value));// clamp<int>((int)round(state_ptr->Position.x - jump_dist.Value), 0, WorldSizeX - 1);
                    }
            );
    ActionRegistry[(int)ActionsIDs::JumpRight] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
                        assert(param_iter != Org->GetParameters().end());

                        auto state_ptr = (OrgState*)State;

                        auto [_, jump_dist] = *param_iter;
                        state_ptr->Position.x = cycle_x(round(state_ptr->Position.x + jump_dist.Value));// clamp<int>((int)round(state_ptr->Position.x + jump_dist.Value), 0, WorldSizeX - 1);
                    }
            );

    // Eat any food that's at most one cell away
    ActionRegistry[(int)ActionsIDs::EatFood] = Action
            (
                    [this](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

                        bool any_eaten = false;
                        for (auto [idx,pos] : enumerate(world_ptr->FoodPositions))
                        {
                            auto diff = abs >> (pos - state_ptr->Position);

                            if (diff.x <= 1 && diff.y <= 1)
                            {
                                state_ptr->Score += FoodScoreGain;

                                // Just move the food to a different position. Achieves the same effect as removing it
                                world_ptr->FoodPositions[idx] = { uniform_real_distribution<float>(0, WorldSizeX)(RNG), uniform_real_distribution<float>(0, WorldSizeY)(RNG) };
                                any_eaten = true;
                                break;
                            }
                        }

                        if (!any_eaten)
                            state_ptr->Score -= WastedActionPenalty;
                    }
            );

    // Kill and eat and individual of a different species at most once cell away
    // This is a simple test, so no fighting is simulated
    ActionRegistry[(int)ActionsIDs::KillAndEat] = Action
            (
                    [this](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;


                        const auto& tag = Org->GetMorphologyTag();

                        // Find an individual of a DIFFERENT species.
                        // The species map is not really useful here
                        bool any_eaten = false;
                        for (const auto& individual : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            // You already know you don't want to kill yourself
                            if (!individual->InSimulation || individual == Org)
                                continue;

                            auto other_state_ptr = (OrgState*)individual->GetState();
                            auto diff = abs >> (other_state_ptr->Position - state_ptr->Position);
                            if (diff.x <= 1 && diff.y <= 1)
                            {
                                // Check if they are from different species by comparing tags
                                const auto& other_tag = individual->GetMorphologyTag();

                                if (!(tag == other_tag))
                                {
                                    state_ptr->Score += KillScoreGain;

                                    other_state_ptr->Score -= DeathPenalty;
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
    SensorRegistry.resize((int)SensorsIDs::NumberOfSensors);

    SensorRegistry[(int)SensorsIDs::CurrentLife] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        return ((OrgState*)State)->Score;
                    }
            );
    // Normalized position
    SensorRegistry[(int)SensorsIDs::CurrentPosX] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        return((OrgState*)State)->Position.x / (float)WorldSizeX;
                    }
            );
    SensorRegistry[(int)SensorsIDs::CurrentPosY] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        return ((OrgState*)State)->Position.y / (float)WorldSizeY;
                    }
            );
    SensorRegistry[(int)SensorsIDs::NearestFoodAngle] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

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
    SensorRegistry[(int)SensorsIDs::NearestFoodDistance] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

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

                        return nearest_dist;
                    }
            );
    SensorRegistry[(int)SensorsIDs::NearestPartnerAngle] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        // TODO : USE THE SPECIES MAP HERE!
                        const auto & tag = Org->GetMorphologyTag();

                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

                        float nearest_dist = numeric_limits<float>::max();
                        float2 nearest_pos;
                        for (const auto& other_org : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            if (!other_org->InSimulation || other_org == Org)
                                continue;

                            float dist = (((OrgState*)other_org->GetState())->Position - state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                            {
                                const auto & other_tag = other_org->GetMorphologyTag();

                                if (tag == other_tag)
                                {
                                    nearest_dist = dist;
                                    nearest_pos = ((OrgState*)other_org->GetState())->Position;
                                }
                            }
                        }

                        // Compute "angle", taking as 0 = looking forward ([0,1])
                        // The idea is
                        //	angle = normalize(V - Pos) . [0,1]
                        return (nearest_pos - state_ptr->Position).normalize().y;
                    }
            );
    SensorRegistry[(int)SensorsIDs::NearestCompetidorAngle] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        const auto& tag = Org->GetMorphologyTag();

                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

                        float nearest_dist = numeric_limits<float>::max();
                        float2 nearest_pos;
                        for (const auto& other_org : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            if (!other_org->InSimulation || other_org == Org)
                                continue;

                            float dist = (((OrgState*)other_org->GetState())->Position - state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                            {
                                const auto& other_tag = other_org->GetMorphologyTag();

                                if (!(tag == other_tag))
                                {
                                    nearest_dist = dist;
                                    nearest_pos = ((OrgState*)other_org->GetState())->Position;
                                }
                            }
                        }

                        // Compute "angle", taking as 0 = looking forward ([0,1])
                        // The idea is
                        //	angle = normalize(V - Pos) . [0,1]
                        return (nearest_pos - state_ptr->Position).normalize().y;
                    }
            );
    SensorRegistry[(int)SensorsIDs::NearestPartnerDistance] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        // TODO : USE THE SPECIES MAP HERE!
                        const auto & tag = Org->GetMorphologyTag();

                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

                        float nearest_dist = numeric_limits<float>::max();
                        float2 nearest_pos;
                        for (const auto& other_org : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            if (!other_org->InSimulation || other_org == Org)
                                continue;

                            float dist = (((OrgState*)other_org->GetState())->Position - state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                            {
                                const auto & other_tag = other_org->GetMorphologyTag();

                                if (tag == other_tag)
                                {
                                    nearest_dist = dist;
                                    nearest_pos = ((OrgState*)other_org->GetState())->Position;
                                }
                            }
                        }

                        return nearest_dist;
                    }
            );
    SensorRegistry[(int)SensorsIDs::NearestCompetidorDistance] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        const auto& tag = Org->GetMorphologyTag();

                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

                        float nearest_dist = numeric_limits<float>::max();
                        float2 nearest_pos;
                        for (const auto& other_org : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            if (!other_org->InSimulation || other_org == Org)
                                continue;

                            float dist = (((OrgState*)other_org->GetState())->Position - state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                            {
                                const auto& other_tag = other_org->GetMorphologyTag();

                                if (!(tag == other_tag))
                                {
                                    nearest_dist = dist;
                                    nearest_pos = ((OrgState*)other_org->GetState())->Position;
                                }
                            }
                        }

                        return nearest_dist;
                    }
            );



    SensorRegistry[(int)SensorsIDs::NearestFoodX] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

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

                        return nearest_pos.x;
                    }
            );
    SensorRegistry[(int)SensorsIDs::NearestFoodY] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

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

                        return nearest_pos.y;
                    }
            );
    SensorRegistry[(int)SensorsIDs::NearestPartnerX] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        // TODO : USE THE SPECIES MAP HERE!
                        const auto & tag = Org->GetMorphologyTag();

                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

                        float nearest_dist = numeric_limits<float>::max();
                        float2 nearest_pos;
                        for (const auto& other_org : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            if (!other_org->InSimulation || other_org == Org)
                                continue;

                            float dist = (((OrgState*)other_org->GetState())->Position - state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                            {
                                const auto & other_tag = other_org->GetMorphologyTag();

                                if (tag == other_tag)
                                {
                                    nearest_dist = dist;
                                    nearest_pos = ((OrgState*)other_org->GetState())->Position;
                                }
                            }
                        }

                        return nearest_pos.x;
                    }
            );
    SensorRegistry[(int)SensorsIDs::NearestPartnerY] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        // TODO : USE THE SPECIES MAP HERE!
                        const auto & tag = Org->GetMorphologyTag();

                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

                        float nearest_dist = numeric_limits<float>::max();
                        float2 nearest_pos;
                        for (const auto& other_org : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            if (!other_org->InSimulation || other_org == Org)
                                continue;

                            float dist = (((OrgState*)other_org->GetState())->Position - state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                            {
                                const auto & other_tag = other_org->GetMorphologyTag();

                                if (tag == other_tag)
                                {
                                    nearest_dist = dist;
                                    nearest_pos = ((OrgState*)other_org->GetState())->Position;
                                }
                            }
                        }

                        return nearest_pos.y;
                    }
            );
    SensorRegistry[(int)SensorsIDs::NearestCompetidorX] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        const auto& tag = Org->GetMorphologyTag();

                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

                        float nearest_dist = numeric_limits<float>::max();
                        float2 nearest_pos;
                        for (const auto& other_org : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            if (!other_org->InSimulation || other_org== Org)
                                continue;

                            float dist = (((OrgState*)other_org->GetState())->Position - state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                            {
                                const auto& other_tag = other_org->GetMorphologyTag();

                                if (!(tag == other_tag))
                                {
                                    nearest_dist = dist;
                                    nearest_pos = ((OrgState*)other_org->GetState())->Position;
                                }
                            }
                        }

                        return nearest_pos.x;
                    }
            );
    SensorRegistry[(int)SensorsIDs::NearestCompetidorY] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        const auto& tag = Org->GetMorphologyTag();

                        auto world_ptr = (WorldData*)World;
                        auto state_ptr = (OrgState*)State;

                        float nearest_dist = numeric_limits<float>::max();
                        float2 nearest_pos;
                        for (const auto& other_org : Individuals)
                        {
                            // Ignore individuals that aren't being simulated right now
                            // Also, don't do all the other stuff against yourself.
                            if (!other_org->InSimulation || other_org == Org)
                                continue;

                            float dist = (((OrgState*)other_org->GetState())->Position - state_ptr->Position).length_sqr();
                            if (dist < nearest_dist)
                            {
                                const auto& other_tag = other_org->GetMorphologyTag();

                                if (!(tag == other_tag))
                                {
                                    nearest_dist = dist;
                                    nearest_pos = ((OrgState*)other_org->GetState())->Position;
                                }
                            }
                        }

                        return nearest_pos.y;
                    }
            );
    // DEBUG!
    /*ComponentRegistry.push_back
    ({
        1,1, // Can only have one body
        {
            // There's only one body to choose
            {
                {
                    (int)ActionsIDs::MoveForward,
                    (int)ActionsIDs::MoveBackwards,
                    (int)ActionsIDs::MoveLeft,
                    (int)ActionsIDs::MoveRight,
                    (int)ActionsIDs::EatFood
                },
                {
                    (int)SensorsIDs::CurrentLife,
                    (int)SensorsIDs::NearestFoodAngle
                }
            }
        }
    });
    return;*/





    // Fill parameters
    ParameterDefRegistry.resize((int)ParametersIDs::NumberOfParameters);

    ParameterDefRegistry[(int)ParametersIDs::JumpDistance] = ParameterDefinition
            (
                    true, // require the parameter, even if the organism doesn't have the component to jump
                    2, // Min bound
                    5, // max bound
                    (int)ParametersIDs::JumpDistance
            );

    // Fill components
    ComponentRegistry.push_back
            ({
                     1,1, // Can only have one mouth
                     {
                             // Herbivore
                             {
                                     {(int)ActionsIDs::EatFood},
                                     {
                                             (int)SensorsIDs::NearestFoodAngle,
                                             (int)SensorsIDs::NearestFoodDistance,

                                             //(int)SensorsIDs::NearestCompetidorAngle,
                                             //(int)SensorsIDs::NearestCompetidorDistance,

                                             //(int)SensorsIDs::NearestPartnerAngle,

                                             //(int)SensorsIDs::NearestCompetidorDistance,
                                             //(int)SensorsIDs::NearestFoodDistance,
                                             //(int)SensorsIDs::NearestPartnerDistance,

                                             /*(int)SensorsIDs::NearestFoodAngle,
                                             (int)SensorsIDs::NearestCompetidorAngle,
                                             (int)SensorsIDs::NearestCompetidorDistance,
                                             (int)SensorsIDs::NearestPartnerAngle,
                                             (int)SensorsIDs::NearestPartnerDistance,*/

                                             /*(int)SensorsIDs::NearestFoodX,
                                             (int)SensorsIDs::NearestFoodY,
                                             (int)SensorsIDs::NearestCompetidorX,
                                             (int)SensorsIDs::NearestCompetidorY,
                                             (int)SensorsIDs::NearestPartnerX,
                                             (int)SensorsIDs::NearestPartnerY,*/

                                             //(int)SensorsIDs::CurrentPosX,
                                             //(int)SensorsIDs::CurrentPosY,
                                     }
                             },
                             // Carnivore
#if 1
                             {
                                     {(int)ActionsIDs::KillAndEat},
                                     {
                                             (int)SensorsIDs::NearestCompetidorAngle,
                                             (int)SensorsIDs::NearestCompetidorDistance,

                                             //(int)SensorsIDs::NearestPartnerAngle,
                                             //(int)SensorsIDs::NearestPartnerDistance,
                                             //(int)SensorsIDs::NearestFoodAngle,
                                             //(int)SensorsIDs::NearestPartnerAngle,

                                             //(int)SensorsIDs::NearestCompetidorDistance,
                                             //(int)SensorsIDs::NearestFoodDistance,
                                             //(int)SensorsIDs::NearestPartnerDistance,

                                             /*(int)SensorsIDs::NearestCompetidorAngle,
                                             (int)SensorsIDs::NearestCompetidorDistance,
                                             (int)SensorsIDs::NearestPartnerAngle,
                                             (int)SensorsIDs::NearestPartnerDistance,*/

                                             /*(int)SensorsIDs::NearestFoodX,
                                             (int)SensorsIDs::NearestFoodY,
                                             (int)SensorsIDs::NearestCompetidorX,
                                             (int)SensorsIDs::NearestCompetidorY,
                                             (int)SensorsIDs::NearestPartnerX,
                                             (int)SensorsIDs::NearestPartnerY,*/

                                             //(int)SensorsIDs::CurrentPosX,
                                             //(int)SensorsIDs::CurrentPosY,
                                     }
                             }
#endif
                     }
             });
    ComponentRegistry.push_back
            ({
                     1,1, // Can only have one body
                     {
                             // There's only one body to choose
                             {
                                     {
                                             (int)ActionsIDs::MoveForward,
                                             (int)ActionsIDs::MoveBackwards,
                                             (int)ActionsIDs::MoveLeft,
                                             (int)ActionsIDs::MoveRight,
                                     }/*,
					{ (int)SensorsIDs::CurrentLife }*/
                             }
                     }
             });
    /*ComponentRegistry.push_back
    ({
        0,2, // Jumping is optional
        {
            {
                {
                    (int)ActionsIDs::JumpForward,
                    (int)ActionsIDs::JumpBackwards,
                },
                {}
            },
            {
                {
                    (int)ActionsIDs::JumpLeft,
                    (int)ActionsIDs::JumpRight,
                },
                {}
            }
        }
    });*/
    /*ComponentRegistry.push_back
    ({
        0,2, // Optional sensors groups
        {
            {
                {},
                {
                    (int)SensorsIDs::NearestPartnerAngle,
                    (int)SensorsIDs::NearestCompetidorAngle
                }
            },
            {
                {},
                {
                    (int)SensorsIDs::NearestFoodAngle
                }
            }
        }
    });*/

}

void* PublicInterfaceImpl::MakeState(const Individual *org)
{
    auto state = new OrgState;

    state->Score = 0;
    state->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
    state->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);

    // TODO : If the org mutates this becomes invalid...
    for (auto [gid, cid] : org->GetMorphologyTag())
    {
        if (gid == 0) // mouth group
        {
            if (cid == 0) // herbivore
                state->IsCarnivore = false;
            else if (cid == 1)
                state->IsCarnivore = true;
        }
    }

    return state;
}

void PublicInterfaceImpl::ResetState(void *State)
{
    auto state_ptr = (OrgState*)State;

    state_ptr->Score = 0;
    state_ptr->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
    state_ptr->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
}

void PublicInterfaceImpl::DestroyState(void *State)
{
    auto state_ptr = (OrgState*)State;
    delete state_ptr;
}

float PublicInterfaceImpl::Distance(struct Individual *, struct Individual *, void *World)
{
    assert(false);
    return 0; // Not used
}

void* PublicInterfaceImpl::DuplicateState(void *State)
{
    auto new_state = new OrgState;
    *new_state = *(OrgState*)State;
    return new_state;

}

void PublicInterfaceImpl::ComputeFitness(Population *Pop, void *World)
{
    for (auto& org : Pop->GetIndividuals())
        org.Reset();

    LastSimulationStepCount = 0;
    for (int i = 0; i < MaxSimulationSteps; i++)
    {
        for (auto& org : Pop->GetIndividuals())
        {
            if (!org.InSimulation)
                continue;

            auto state_ptr = (OrgState*)org.GetState();

            org.DecideAndExecute(World, Pop);

            org.Fitness = state_ptr->Score;
        }
        LastSimulationStepCount++;
    }
}