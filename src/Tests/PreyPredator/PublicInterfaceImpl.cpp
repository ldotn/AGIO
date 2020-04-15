#include "PublicInterfaceImpl.h"
#include "PreyPredator.h"
#include <enumerate.h>
#include <assert.h>

using namespace fpp;
using namespace std;
using namespace agio;

void PublicInterfaceImpl::Init()
{
    ActionRegistry.resize((int)ActionsIDs::NumberOfActions);
    auto kill_and_eat = [this](void* State, const vector<BaseIndividual*>& Individuals, BaseIndividual* Org, void* World)
    {return;
        auto state_ptr = (OrgState*)State;
        const auto& tag = Org->GetMorphologyTag();

        // Find an individual herbivore at the same position
        bool any_eaten = false;
        for (const auto& individual : Individuals)
        {
            // Ignore individuals that aren't being simulated right now
            // Also, don't do all the other stuff against yourself.
            // You already know you don't want to kill yourself
            if (!individual->InSimulation || individual == Org)
                continue;

            auto other_state_ptr = (OrgState*)individual->GetState();
            if(!other_state_ptr->IsCarnivore && other_state_ptr->Position == state_ptr->Position)
            {
                state_ptr->Score += KillScoreGain;

                other_state_ptr->Score -= DeathPenalty;
                other_state_ptr->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
                other_state_ptr->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
                any_eaten = true;
                break;
            }
        }

        state_ptr->FailableActionCount++;
        if (!any_eaten)
        {
            state_ptr->Score -= WastedActionPenalty;
            state_ptr->FailedActionCountCurrent++;
        }
        else
            state_ptr->EatenCount++;

    };
    auto eat_food = [this](void* State, const vector<BaseIndividual*>& Individuals, BaseIndividual* Org, void* World)
    {return;
        auto world_ptr = (WorldData*)World;
        auto state_ptr = (OrgState*)State;

        bool any_eaten = false;
        for (auto [idx, pos] : enumerate(world_ptr->FoodPositions))
        {
            auto diff = abs >> (pos - state_ptr->Position);

            //if (diff.x <= 1 && diff.y <= 1)
            if (pos == state_ptr->Position)
            {
                state_ptr->Score += FoodScoreGain;

                // Just move the food to a different position. Achieves the same effect as removing it
                world_ptr->FoodPositions[idx].x = uniform_int_distribution<int>(0, WorldSizeX-1)(RNG);
                world_ptr->FoodPositions[idx].y = uniform_int_distribution<int>(0, WorldSizeY-1)(RNG);
                any_eaten = true;
                break;
            }
        }

        state_ptr->FailableActionCount++;
        if (!any_eaten)
        {
            state_ptr->Score -= WastedActionPenalty;
            state_ptr->FailedActionCountCurrent++;
        }
        else
            state_ptr->EatenCount++;
    };
    
    // Basic movement
    // The world is circular, doing this so that % works with negative numbers
    auto cycle_x = [](int x) { return ((x % WorldSizeX) + WorldSizeX) % WorldSizeX; };
    auto cycle_y = [](int y) { return ((y % WorldSizeY) + WorldSizeY) % WorldSizeY; };
    float DistanceNormFactor = 1.0/(WorldSizeX * WorldSizeX + WorldSizeY * WorldSizeY);

    ActionRegistry[(int)ActionsIDs::MoveForward] = Action
            (
                    [&, kill_and_eat, eat_food](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.y >= WorldSizeY - 1)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.y = cycle_y(state_ptr->Position.y + 1);
						state_ptr->VisitedCells.insert({state_ptr->Position.x,state_ptr->Position.y});
                        if (state_ptr->IsCarnivore)
                            kill_and_eat(State, Individuals, Org, World);
                        else
                            eat_food(State, Individuals, Org, World);
                    }
            );

    ActionRegistry[(int)ActionsIDs::MoveBackwards] = Action
            (
                    [&, kill_and_eat, eat_food](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.y <= 1)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.y = cycle_y(state_ptr->Position.y - 1);
						state_ptr->VisitedCells.insert({ state_ptr->Position.x,state_ptr->Position.y });
                        if (state_ptr->IsCarnivore)
                            kill_and_eat(State, Individuals, Org, World);
                        else
                            eat_food(State, Individuals, Org, World);
                    }
            );

    ActionRegistry[(int)ActionsIDs::MoveRight] = Action
            (
                    [&, kill_and_eat, eat_food](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.x >= WorldSizeX)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.x = cycle_x(state_ptr->Position.x + 1);
						state_ptr->VisitedCells.insert({ state_ptr->Position.x,state_ptr->Position.y });
                        if (state_ptr->IsCarnivore)
                            kill_and_eat(State, Individuals, Org, World);
                        else
                            eat_food(State, Individuals, Org, World);
                    }
            );

    ActionRegistry[(int)ActionsIDs::MoveLeft] = Action
            (
                    [&, kill_and_eat, eat_food](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.x <= 1)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.x = cycle_x(state_ptr->Position.x - 1);
						state_ptr->VisitedCells.insert({ state_ptr->Position.x,state_ptr->Position.y });
                        if (state_ptr->IsCarnivore)
                            kill_and_eat(State, Individuals, Org, World);
                        else
                            eat_food(State, Individuals, Org, World);
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
                                world_ptr->FoodPositions[idx].x = uniform_int_distribution<int>(0, WorldSizeX)(RNG);
                                world_ptr->FoodPositions[idx].y = uniform_int_distribution<int>(0, WorldSizeY)(RNG);
                                any_eaten = true;
                                break;
                            }
                        }

						state_ptr->FailableActionCount++;
						if (!any_eaten)
						{
							state_ptr->Score -= WastedActionPenalty;
							state_ptr->FailedActionCountCurrent++;
						}
						else
							state_ptr->EatenCount++;
                    }
            );

    // Kill and eat and individual of a different species at most once cell away
    // This is a simple test, so no fighting is simulated
    ActionRegistry[(int)ActionsIDs::KillAndEat] = Action
            (
                    [this](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;
                        //return;

                        const auto& tag = Org->GetMorphologyTag();

                        // Find an herbivore
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
                            if (diff.x <= 2 && diff.y <= 2)
                            //if (diff.x <= 1 && diff.y <= 1)
                            {
                                if (!other_state_ptr->IsCarnivore)
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

						state_ptr->FailableActionCount++;
						if (!any_eaten)
						{
							state_ptr->Score -= WastedActionPenalty;
							state_ptr->FailedActionCountCurrent++;
						}
						else
							state_ptr->EatenCount++;
                           
                    }
            );

    // Fill sensors
    SensorRegistry.resize((int)SensorsIDs::NumberOfSensors);

    SensorRegistry[(int)SensorsIDs::NearestFoodDeltaX] = Sensor
            (
                    [DistanceNormFactor](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
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

                        return (nearest_pos - state_ptr->Position).x;// > 0 ? 1 : -1;
                    }
            );

    SensorRegistry[(int)SensorsIDs::NearestFoodDeltaY] = Sensor
            (
                    [DistanceNormFactor](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
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

                        return (nearest_pos - state_ptr->Position).y;// > 0 ? 1 : -1;
                    }
            );

    SensorRegistry[(int)SensorsIDs::NearestCompetitorDeltaX] = Sensor
            (
                    [DistanceNormFactor](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        const auto& tag = Org->GetMorphologyTag();
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

                        return (nearest_pos - state_ptr->Position).x;// > 0 ? 1 : -1;
                    }
            );


    SensorRegistry[(int)SensorsIDs::NearestCompetitorDeltaY] = Sensor
            (
                    [DistanceNormFactor](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
                    {
                        const auto& tag = Org->GetMorphologyTag();
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

                        return (nearest_pos - state_ptr->Position).y;// > 0 ? 1 : -1;
                    }
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
                                             (int)SensorsIDs::NearestFoodDeltaX,
                                             (int)SensorsIDs::NearestFoodDeltaY,
                                             //(int)SensorsIDs::NearestCompetitorDeltaX,
                                             //(int)SensorsIDs::NearestCompetitorDeltaY,
                                     }
                             },
                             // Carnivore
                             {
                                     {(int)ActionsIDs::KillAndEat},
                                     {
                                             (int)SensorsIDs::NearestCompetitorDeltaX,
                                             (int)SensorsIDs::NearestCompetitorDeltaY,
                                     }
                             }
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
}

void* PublicInterfaceImpl::MakeState(const BaseIndividual *org)
{
    auto state = new OrgState;

	state->EatenCount = 0;
	state->FailedActionCountCurrent = 0;
	state->Repetitions = 1;
	state->VisitedCellsCount = 0;
	state->MetricsCurrentGenNumber = -1;
	state->FailableActionCount = 0;
	state->FailedActionFractionAcc = 0;

    state->Score = 0;

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

    state->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
    state->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
    /*
    // Start carnivores on one side and herbivores on the other
    if (state->IsCarnivore)
    {
        state->Position.x = uniform_int_distribution<int>(0, WorldSizeX/2 - 1)(RNG);
        state->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
    }
    else
    {
        state->Position.x = uniform_int_distribution<int>(WorldSizeX / 2, WorldSizeX - 1)(RNG);
        state->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
    }*/

    return state;
}

void PublicInterfaceImpl::ResetState(void *State)
{
    auto state_ptr = (OrgState*)State;

    state_ptr->Score = 0;
    state_ptr->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
    state_ptr->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
    /*
    // Start carnivores on one side and herbivores on the other
    if (state_ptr->IsCarnivore)
    {
        state_ptr->Position.x = uniform_int_distribution<int>(0, WorldSizeX / 2 - 1)(RNG);
        state_ptr->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
    }
    else
    {
        state_ptr->Position.x = uniform_int_distribution<int>(WorldSizeX / 2, WorldSizeX - 1)(RNG);
        state_ptr->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
    }*/

	if (state_ptr->MetricsCurrentGenNumber != CurrentGenNumber)
	{
		state_ptr->MetricsCurrentGenNumber = CurrentGenNumber;
		state_ptr->VisitedCellsCount = 0;
		state_ptr->VisitedCells = {};
		state_ptr->EatenCount = 0;
		state_ptr->FailedActionCountCurrent = 0;
		state_ptr->Repetitions = 1;
		state_ptr->FailableActionCount = 0;
		state_ptr->FailedActionFractionAcc = 0;
	}
	else
	{
		state_ptr->Repetitions++;
		state_ptr->VisitedCellsCount += state_ptr->VisitedCells.size();
		if (state_ptr->FailableActionCount > 0)
		{
			state_ptr->FailedActionFractionAcc += state_ptr->FailedActionCountCurrent / state_ptr->FailableActionCount;
			state_ptr->FailableActionCount = 0;
			state_ptr->FailedActionCountCurrent = 0;
		}
	}
}

void PublicInterfaceImpl::DestroyState(void *State)
{
    auto state_ptr = (OrgState*)State;
    delete state_ptr;
}

void* PublicInterfaceImpl::DuplicateState(void *State)
{
    auto new_state = new OrgState;
    *new_state = *(OrgState*)State;
    return new_state;

}

void PublicInterfaceImpl::ComputeFitness(const std::vector<class BaseIndividual*>& Individuals, void *World)
{
    for (auto& org : Individuals)
        ((Individual*)org)->Reset();

    LastSimulationStepCount = 0;
    for (int i = 0; i < MaxSimulationSteps; i++)
    {
        for (auto& org : Individuals)
        {
            if (!org->InSimulation)
                continue;

            auto state_ptr = (OrgState*)org->GetState();

            org->DecideAndExecute(World, Individuals);

            ((Individual*)org)->Fitness = state_ptr->Score - (state_ptr->FailedActionCountCurrent / state_ptr->FailableActionCount);
        }
        LastSimulationStepCount++;
    }
}