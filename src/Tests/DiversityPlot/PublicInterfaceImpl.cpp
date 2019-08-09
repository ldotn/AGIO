#include "PublicInterfaceImpl.h"
#include "DiversityPlot.h"
#include <enumerate.h>
#include <assert.h>
#include "../../Serialization/SIndividual.h"

using namespace fpp;
using namespace std;
using namespace agio;

int cycle_x(int x)
{
    return ((x % WorldSizeX) + WorldSizeX) % WorldSizeX;
};

int cycle_y(int y)
{
    return ((y % WorldSizeY) + WorldSizeY) % WorldSizeY;
};

float2 random_valid_position(WorldData *world_ptr)
{
    minstd_rand RNG = minstd_rand(chrono::high_resolution_clock::now().time_since_epoch().count());
    bool valid_position = false;
    int x, y;
    while (!valid_position)
    {
        x = uniform_int_distribution<int>(0, WorldSizeX -1)(RNG);
        y = uniform_int_distribution<int>(0, WorldSizeY -1)(RNG);

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
        //float dist = (pos - state_ptr->Position).length_sqr();
		auto delta = abs >> (pos - state_ptr->Position);
		int dist = max(delta.x, delta.y);

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
                                   const vector<BaseIndividual *> &Individuals, bool SearchDead = false)
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
        if (!individual->InSimulation || tag == individual->GetMorphologyTag())
            continue;
		
		// Ignore individuals that are out of the world bounds
		if (other_state_ptr->Position.x < 0 || other_state_ptr->Position.y < 0 ||
			other_state_ptr->Position.x > WorldSizeX || other_state_ptr->Position.y > WorldSizeY)
			continue;

		// Ignore dead individuals
		if ((SearchDead && other_state_ptr->Life > 0) ||
			(!SearchDead && other_state_ptr->Life <= 0))
			continue;

		auto delta = abs >> (other_state_ptr->Position - state_ptr->Position);
		int dist = max(delta.x, delta.y);
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

	auto delta = abs >> (pos - state_ptr->Position);
	int dist = max(delta.x, delta.y);
    if (dist <= 1)
    {
        world_ptr->FoodPositions[idx] = random_valid_position(world_ptr);
        return true;
    }

    return false;
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
							state_ptr->Score -= WastedActionPenalty;

						if(any_eaten)
							state_ptr->EatenCount++;
                    }
            );

    // Eat any dead enemy that's at most one cell away
    ActionRegistry[(int) ActionsIDs::EatEnemy] = Action
            (
                    [this](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
						auto dead_enemy = find_nearest_enemy(State, Org, Individuals, true);
                        //bool any_eaten = eat_enemy(State, Individuals, Org, World);

                        auto state_ptr = (OrgState *) State;
						if (dead_enemy)
						{
							state_ptr->Life += FoodScoreGain;

							// Move the eaten enemy out of the world
							dead_enemy->GetState<OrgState>()->Position.x = -999;
							dead_enemy->GetState<OrgState>()->Position.y = -999;

							state_ptr->EatenCount++;
						}
                        else
							state_ptr->Score -= WastedActionPenalty;
                    }
            );

    // Eat any dead enemy or food (food first) that's at most one cell away
    ActionRegistry[(int) ActionsIDs::EatFoodEnemy] = Action
            (
                    [this](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        auto state_ptr = (OrgState *) State;
						auto enemy = find_nearest_enemy(State, Org, Individuals, true);

                        if (eat_food(State, World))
                            state_ptr->Life += FoodScoreGain;
						else if (enemy)
						{
							state_ptr->Life += FoodScoreGain;

							enemy->GetState<OrgState>()->Score -= DeathPenalty;

							// Move the eaten enemy out of the world
							enemy->GetState<OrgState>()->Position.x = -999;
							enemy->GetState<OrgState>()->Position.y = -999;

							state_ptr->EatenCount++;
						}
                        else
							state_ptr->Score -= WastedActionPenalty;
                    }
            );

    // Kill an individual of a different species at most once cell away
    // This is a simple test, so no fighting is simulated
    ActionRegistry[(int) ActionsIDs::Kill] = Action
            (
                    [this](void *State, const vector<BaseIndividual *> &Individuals, BaseIndividual *Org, void *World)
                    {
                        auto state_ptr = (OrgState *) State;

                        BaseIndividual *enemy = find_nearest_enemy(State, Org, Individuals);
                        if (enemy)
                        {
                            auto other_state_ptr = (OrgState *) enemy->GetState();

							auto delta = abs >> (other_state_ptr->Position - state_ptr->Position);
							int dist = max(delta.x, delta.y);
							if (dist <= 1)
							{
								other_state_ptr->Life = 0;

								enemy->GetState<OrgState>()->Score -= DeathPenalty;
							}
                            else
								state_ptr->Score -= WastedActionPenalty;
                        } else
							state_ptr->Score -= WastedActionPenalty;
                    }
            );

    // Fill sensors
    SensorRegistry.resize((int) SensorsIDs::NumberOfSensors);

    SensorRegistry[(int) SensorsIDs::NearestFoodDeltaX] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        auto world_ptr = (WorldData *) World;
                        auto state_ptr = (OrgState *) State;

                        int idx = find_nearest_food(State, World);
                        float2 pos = world_ptr->FoodPositions[idx];

                        float2 dist = pos - state_ptr->Position;
                        return dist.x;
                    }
            );

    SensorRegistry[(int) SensorsIDs::NearestFoodDeltaY] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        auto world_ptr = (WorldData *) World;
                        auto state_ptr = (OrgState *) State;

                        int idx = find_nearest_food(State, World);
                        float2 pos = world_ptr->FoodPositions[idx];

                        float2 dist = pos - state_ptr->Position;
                        return dist.y;
                    }
            );

    SensorRegistry[(int) SensorsIDs::NearestCorpseDeltaX] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        BaseIndividual* nearest_competitor = find_nearest_enemy(State, Org, Individuals, true);
						if (!nearest_competitor) return -999.0f;

                        auto state_ptr = (OrgState *) State;
                        auto other_state_ptr = (OrgState *) nearest_competitor->GetState();

                        float2 dist = other_state_ptr->Position - state_ptr->Position;
                        return dist.x;
                    }
            );


    SensorRegistry[(int) SensorsIDs::NearestCorpseDeltaY] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        BaseIndividual* nearest_competitor = find_nearest_enemy(State, Org, Individuals, true);
						if (!nearest_competitor) return -999.0f;

                        auto state_ptr = (OrgState *) State;
                        auto other_state_ptr = (OrgState *) nearest_competitor->GetState();

                        float2 dist = other_state_ptr->Position - state_ptr->Position;
                        return dist.y;
                    }
            );

	    SensorRegistry[(int) SensorsIDs::NearestCompetidorDeltaX] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        BaseIndividual* nearest_competitor = find_nearest_enemy(State, Org, Individuals);
						if (!nearest_competitor) return -999.0f;

                        auto state_ptr = (OrgState *) State;
                        auto other_state_ptr = (OrgState *) nearest_competitor->GetState();

                        float2 dist = other_state_ptr->Position - state_ptr->Position;
                        return dist.x;
                    }
            );


    SensorRegistry[(int) SensorsIDs::NearestCompetidorDeltaY] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
                        BaseIndividual* nearest_competitor = find_nearest_enemy(State, Org, Individuals);
						if (!nearest_competitor) return -999.0f;

                        auto state_ptr = (OrgState *) State;
                        auto other_state_ptr = (OrgState *) nearest_competitor->GetState();

                        float2 dist = other_state_ptr->Position - state_ptr->Position;
                        return dist.y;
                    }
            );

	SensorRegistry[(int) SensorsIDs::CurrentCell] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
						auto world_ptr = (WorldData *)World;
                        auto state_ptr = (OrgState *) State;

						int x = cycle_x(state_ptr->Position.x + 0);
						int y = cycle_y(state_ptr->Position.y + 0);
						return (float)world_ptr->CellTypes[y][x];
                    }
            );
	SensorRegistry[(int) SensorsIDs::TopCell] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
						auto world_ptr = (WorldData *)World;
                        auto state_ptr = (OrgState *) State;

						int x = cycle_x(state_ptr->Position.x + 0);
						int y = cycle_y(state_ptr->Position.y + 1);
						return (float)world_ptr->CellTypes[y][x];
                    }
            );
	SensorRegistry[(int) SensorsIDs::DownCell] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
						auto world_ptr = (WorldData *)World;
                        auto state_ptr = (OrgState *) State;

						int x = cycle_x(state_ptr->Position.x + 0);
						int y = cycle_y(state_ptr->Position.y - 1);
						return (float)world_ptr->CellTypes[y][x];
                    }
            );
	SensorRegistry[(int) SensorsIDs::RightCell] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
						auto world_ptr = (WorldData *)World;
                        auto state_ptr = (OrgState *) State;

						int x = cycle_x(state_ptr->Position.x + 1);
						int y = cycle_y(state_ptr->Position.y + 0);
						return (float)world_ptr->CellTypes[y][x];
                    }
            );
	SensorRegistry[(int) SensorsIDs::LeftCell] = Sensor
            (
                    [](void *State, const vector<BaseIndividual *> &Individuals, const BaseIndividual *Org, void *World)
                    {
						auto world_ptr = (WorldData *)World;
                        auto state_ptr = (OrgState *) State;

						int x = cycle_x(state_ptr->Position.x - 1);
						int y = cycle_y(state_ptr->Position.y + 0);
						return (float)world_ptr->CellTypes[y][x];
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

    // Fill components
	ComponentRegistry.push_back
	({
		1, 1, // Everyone can see the nearby cells
		{{
			{},
			{
				(int)SensorsIDs::CurrentCell,
				(int)SensorsIDs::TopCell,
				(int)SensorsIDs::DownCell,
				(int)SensorsIDs::RightCell,
				(int)SensorsIDs::LeftCell,
			}
		}}
	});
    ComponentRegistry.push_back
            ({
                     1, 1, // Can only have one mouth
                     {
                             // Herbivore
                             {
                                     {(int) ActionsIDs::EatFood},
                                     {
                                             (int) SensorsIDs::NearestFoodDeltaX,
                                             (int) SensorsIDs::NearestFoodDeltaY,
                                     }
                             },
                             // Carnivore
                             {
                                     {(int) ActionsIDs::EatEnemy, (int) ActionsIDs::Kill},
                                     {
											 (int) SensorsIDs::NearestCompetidorDeltaX,
											 (int) SensorsIDs::NearestCompetidorDeltaY,
                                             (int) SensorsIDs::NearestCorpseDeltaX,
                                             (int) SensorsIDs::NearestCorpseDeltaY,
                                     }
                             },
                             // Omnivore
                             {
                                     {(int) ActionsIDs::EatFoodEnemy, (int)ActionsIDs::Kill},
                                     {
										     (int)SensorsIDs::NearestCompetidorDeltaX,
											 (int)SensorsIDs::NearestCompetidorDeltaY,
                                             (int) SensorsIDs::NearestFoodDeltaX,
                                             (int) SensorsIDs::NearestFoodDeltaY,
                                             (int) SensorsIDs::NearestCorpseDeltaX,
                                             (int) SensorsIDs::NearestCorpseDeltaY,
                                     }
                             },
                     }
             });

    ComponentRegistry.push_back
            ({
                     1, 1, // Can have a only ground body or an amphibian body
                     {
                             // Can walk or swim and walk
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
                                                     (int) SensorsIDs::NearestCompetidorDeltaX,
                                                     (int) SensorsIDs::NearestCompetidorDeltaY
                                             }
                                     }
                             }
                     }
             });
}

void *PublicInterfaceImpl::MakeState(const BaseIndividual *org)
{
    auto state = new OrgState;

	state->Life = InitialLife;
    state->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
    state->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
	state->EatenCount = 0;
	state->Score = 0;

	if (org->HasAction((int)ActionsIDs::EatFoodEnemy))
		state->Type = OrgType::Omnivore;
	else if (org->HasAction((int)ActionsIDs::EatEnemy))
		state->Type = OrgType::Carnivore;
	else
		state->Type = OrgType::Herbivore;

    return state;
}

void PublicInterfaceImpl::ResetState(void *State)
{
    auto state_ptr = (OrgState *) State;

	state_ptr->Life = InitialLife;
    state_ptr->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
    state_ptr->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
	state_ptr->EatenCount = 0;
	state_ptr->Score = 0;

	// The type doesnt change
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
	// IMPORTANT! This can ONLY be called with Individuals, calling it with SIndividuals will FAIL!

    for (auto &org : Individuals)
        ((Individual *) org)->Reset();

    LastSimulationStepCount = 0;
    for (int i = 0; i < MaxSimulationSteps; i++)
    {
		bool any_alive = false;

        for (auto &org : Individuals)
        {
            if (!org->InSimulation)
                continue;

            auto state_ptr = (OrgState *) org->GetState();
			if (state_ptr->Life > 0)
			{
				org->DecideAndExecute(World, Individuals);

				org->GetState<OrgState>()->Score += 0.1*state_ptr->Life;
				((Individual *)org)->Fitness = org->GetState<OrgState>()->Score;
				any_alive = true;
			}
        }
        LastSimulationStepCount++;
		if (!any_alive)
			return;
    }
}