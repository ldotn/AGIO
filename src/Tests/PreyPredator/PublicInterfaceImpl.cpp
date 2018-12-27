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

    // Basic movement
    // The world is circular, doing this so that % works with negative numbers
    auto cycle_x = [](int x) { return ((x % WorldSizeX) + WorldSizeX) % WorldSizeX; };
    auto cycle_y = [](int y) { return ((y % WorldSizeY) + WorldSizeY) % WorldSizeY; };



    ActionRegistry[(int)ActionsIDs::MoveForward] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.y >= WorldSizeY - 1)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.y = cycle_y(state_ptr->Position.y + 1);
						state_ptr->VisitedCells.insert({state_ptr->Position.x,state_ptr->Position.y});
                    }
            );

    ActionRegistry[(int)ActionsIDs::MoveBackwards] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.y <= 1)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.y = cycle_y(state_ptr->Position.y - 1);
						state_ptr->VisitedCells.insert({ state_ptr->Position.x,state_ptr->Position.y });
                    }
            );

    ActionRegistry[(int)ActionsIDs::MoveRight] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.x >= WorldSizeX)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.x = cycle_x(state_ptr->Position.x + 1);
						state_ptr->VisitedCells.insert({ state_ptr->Position.x,state_ptr->Position.y });
                    }
            );

    ActionRegistry[(int)ActionsIDs::MoveLeft] = Action
            (
                    [&](void * State, const vector<BaseIndividual*> &Individuals, BaseIndividual * Org, void * World)
                    {
                        auto state_ptr = (OrgState*)State;

                        if (state_ptr->Position.x <= 1)
                            state_ptr->Score -= BorderPenalty;

                        state_ptr->Position.x = cycle_x(state_ptr->Position.x - 1);
						state_ptr->VisitedCells.insert({ state_ptr->Position.x,state_ptr->Position.y });
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

                        return (nearest_pos - state_ptr->Position).x;
                    }
            );

    SensorRegistry[(int)SensorsIDs::NearestFoodDeltaY] = Sensor
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

                        return (nearest_pos - state_ptr->Position).y;
                    }
            );

    SensorRegistry[(int)SensorsIDs::NearestCompetitorDeltaX] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
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

                        return (nearest_pos - state_ptr->Position).x;
                    }
            );


    SensorRegistry[(int)SensorsIDs::NearestCompetitorDeltaY] = Sensor
            (
                    [](void * State, const vector<BaseIndividual*> &Individuals, const BaseIndividual * Org, void * World)
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

                        return (nearest_pos - state_ptr->Position).x;
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
	state->Repetitions = 0;
	state->VisitedCellsCount = 0;
	state->MetricsCurrentGenNumber = 0;
	state->FailableActionCount = 0;
	state->FailedActionFractionAcc = 0;

    state->Score = 0;
    state->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
    state->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);

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

	if (state_ptr->MetricsCurrentGenNumber != CurrentGenNumber)
	{
		state_ptr->MetricsCurrentGenNumber = CurrentGenNumber;
		state_ptr->VisitedCellsCount = 0;
		state_ptr->VisitedCells = {};
		state_ptr->EatenCount = 0;
		state_ptr->FailedActionCountCurrent = 0;
		state_ptr->Repetitions = 0;
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

            ((Individual*)org)->Fitness = state_ptr->Score;
        }
        LastSimulationStepCount++;
    }
}