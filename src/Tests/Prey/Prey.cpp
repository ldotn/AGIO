#include <assert.h>
#include <algorithm>
#include <random>
#include <queue>
#include <matplotlibcpp.h>
#include <enumerate.h>

#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include "../../Utils/Math/Float2.h"

#include "neat.h"

namespace plt = matplotlibcpp;
using namespace std;
using namespace agio;
using namespace fpp;

// TODO : Refactor, this could be spread on a couple files.
//   Need to see if that's actually better though
const int WorldSizeX = 100;
const int WorldSizeY = 100;
const float FoodScoreGain = 20;
const int FoodCellCount = WorldSizeX * WorldSizeY * 0.001;
const int MaxSimulationSteps = 200;
const int PopulationSize = 100;
const int GenerationsCount = 250;

minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

struct OrgState
{
	float Score = 0;
	float2 Position;
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

	NumberOfActions
};

enum class SensorsIDs
{
	NearestFoodAngle,
	NearestFoodDistance,
	NearestFoodDistanceX,
	NearestFoodDistanceY,

	NumberOfSensors
};

struct WorldData
{
	vector<float2> FoodPositions;
};

class TestInterface : public PublicInterface
{
public:
	void Init() override
	{
		ActionRegistry.resize((int)ActionsIDs::NumberOfActions);

		// Basic movement
		// The world is circular, doing this so that % works with negative numbers
		auto cycle_x = [](int x) { return ((x % WorldSizeX) + WorldSizeX) % WorldSizeX; };
		auto cycle_y = [](int y) { return ((y % WorldSizeY) + WorldSizeY) % WorldSizeY; };

		ActionRegistry[(int)ActionsIDs::MoveForward] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
			{
				auto state_ptr = (OrgState*)State;

				state_ptr->Position.y = cycle_y(state_ptr->Position.y + 1);
			}
		);
		ActionRegistry[(int)ActionsIDs::MoveBackwards] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
			{
				auto state_ptr = (OrgState*)State;

				state_ptr->Position.y = cycle_y(state_ptr->Position.y - 1) % WorldSizeY;
			}
		);
		ActionRegistry[(int)ActionsIDs::MoveRight] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
			{
				auto state_ptr = (OrgState*)State;

				state_ptr->Position.x = cycle_x(state_ptr->Position.x + 1) % WorldSizeX;
			}
		);
		ActionRegistry[(int)ActionsIDs::MoveLeft] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
			{
				auto state_ptr = (OrgState*)State;

				state_ptr->Position.x = cycle_x(state_ptr->Position.x - 1) % WorldSizeX;
			}
		);

		// "Jump". Move more than one cell at a time. Jump distance is a parameter
		ActionRegistry[(int)ActionsIDs::JumpForward] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
			{
				auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
				assert(param_iter != Org->GetParameters().end());

				auto state_ptr = (OrgState*)State;

				Parameter jump_dist = param_iter->second;

				state_ptr->Position.y = cycle_y(round(state_ptr->Position.y + jump_dist.Value));
			}
		);
		ActionRegistry[(int)ActionsIDs::JumpBackwards] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
			{
				auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
				assert(param_iter != Org->GetParameters().end());

				auto state_ptr = (OrgState*)State;

				Parameter jump_dist = param_iter->second;
				state_ptr->Position.y = cycle_y(round(state_ptr->Position.y - jump_dist.Value));
			}
		);
		ActionRegistry[(int)ActionsIDs::JumpLeft] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
			{
				auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
				assert(param_iter != Org->GetParameters().end());

				auto state_ptr = (OrgState*)State;

				Parameter jump_dist = param_iter->second;
				state_ptr->Position.x = cycle_x(round(state_ptr->Position.x - jump_dist.Value));
			}
		);
		ActionRegistry[(int)ActionsIDs::JumpRight] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
			{
				auto param_iter = Org->GetParameters().find((int)ParametersIDs::JumpDistance);
				assert(param_iter != Org->GetParameters().end());

				auto state_ptr = (OrgState*)State;

				auto [_, jump_dist] = *param_iter;
				state_ptr->Position.x = cycle_x(round(state_ptr->Position.x + jump_dist.Value));
			}
		);

		// Eat any food that's at most one cell away 
		ActionRegistry[(int)ActionsIDs::EatFood] = Action
		(
			[](void * State, const Population * Pop, Individual * Org, void * World)
			{
				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				for (auto [idx,pos] : enumerate(world_ptr->FoodPositions))
				{
					auto diff = abs >> (pos - state_ptr->Position);

					if (diff.x <= 1 && diff.y <= 1)
					{
						state_ptr->Score += FoodScoreGain;

						// Remove food and create a new one
						world_ptr->FoodPositions.erase(world_ptr->FoodPositions.begin() + idx);
						float2 food_pos(uniform_real_distribution<float>(0, WorldSizeX)(RNG),
						        uniform_real_distribution<float>(0, WorldSizeY)(RNG));
						world_ptr->FoodPositions.push_back(food_pos);
						break;
					}
				}
			}
		);

		// Fill sensors
		SensorRegistry.resize((int)SensorsIDs::NumberOfSensors);

		SensorRegistry[(int)SensorsIDs::NearestFoodAngle] = Sensor
		(
			[this](void *State, void *World, const Population *Pop, const Individual *Org)
			{
				auto state_ptr = (OrgState*)State;

				float2 nearest_food = findNearestFood(State, World);

				// Compute "angle", taking as 0 = looking forward ([0,1])
				// The idea is
				//	angle = normalize(V - Pos) . [0,1]
				return (nearest_food - state_ptr->Position).normalize().y;
			}
		);

        SensorRegistry[(int)SensorsIDs::NearestFoodDistance] = Sensor
        (
            [this](void * State, void * World, const Population* Pop, const Individual * Org)
            {
                auto state_ptr = (OrgState*)State;
                float2 nearest_food = findNearestFood(State, World);
                return (nearest_food - state_ptr->Position).length_sqr();
            }
        );

        SensorRegistry[(int)SensorsIDs::NearestFoodDistanceX] = Sensor
        (
            [this](void * State, void * World, const Population* Pop, const Individual * Org)
            {
                auto state_ptr = (OrgState*)State;
                float2 nearest_food = findNearestFood(State, World);
                return (nearest_food - state_ptr->Position).x;
            }
        );

        SensorRegistry[(int)SensorsIDs::NearestFoodDistanceY] = Sensor
        (
            [this](void * State, void * World, const Population* Pop, const Individual * Org)
            {
                auto state_ptr = (OrgState*)State;
                float2 nearest_food = findNearestFood(State, World);
                return (nearest_food - state_ptr->Position).y;
            }
        );

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
        ComponentGroup mouth_group;
        mouth_group.MinCardinality = 1;
        mouth_group.MaxCardinality = 1;

        mouth_group.Components.push_back
        ({
            {(int)ActionsIDs::EatFood},
            {(int)SensorsIDs::NearestFoodAngle}
        });

        mouth_group.Components.push_back
        ({
             {(int)ActionsIDs::EatFood},
             {
                 (int)SensorsIDs::NearestFoodAngle,
                 (int)SensorsIDs::NearestFoodDistance
             }
         });

        mouth_group.Components.push_back
        ({
             {(int)ActionsIDs::EatFood},
             {
                 (int)SensorsIDs::NearestFoodAngle,
                 (int)SensorsIDs::NearestFoodDistanceX,
                 (int)SensorsIDs::NearestFoodDistanceY
             }
         });

        ComponentRegistry.push_back(mouth_group);

		ComponentRegistry.push_back
		({
			1,1, // Can only have one body 
			{
				// There's only one body to choose
				{
					{ // Actions
						(int)ActionsIDs::MoveForward,
						(int)ActionsIDs::MoveBackwards,
						(int)ActionsIDs::MoveLeft,
						(int)ActionsIDs::MoveRight,
					},
					{ // Sensors

					}
				}
			}
		});

		ComponentRegistry.push_back
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
		});
	}

	void * MakeState(const Individual * org) override
	{
		auto state = new OrgState;

		state->Score = 0;
		state->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
		state->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);

		return state;
	}

	void ResetState(void * State) override
	{
		auto state = (OrgState*)State;

		state->Score = 0;
		state->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
		state->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
	}

	void DestroyState(void * State) override
	{
		auto state = (OrgState*)State;
		delete state;
	}

	float Distance(class Individual *, class Individual *, void * World) override
	{
		assert(false);
		return 0; // Not used
	}

	void * DuplicateState(void * State) override
	{
		auto new_state = new OrgState;
		*new_state = *(OrgState*)State;
		return new_state;
	}

	// This computes fitness for the entire population
	void ComputeFitness(Population * Pop, void * World) override
	{
		std::vector<Individual> &individuals = Pop->GetIndividuals();

		for (auto &org : individuals)
			org.Reset();

		// Shuffle the evaluation order
		vector<int> order(individuals.size());
		for (int i = 0; i < individuals.size(); i++)
			order[i] = i;
		shuffle(order.begin(), order.end(), RNG);

		for (int i = 0; i < MaxSimulationSteps; i++)
		{
			for (auto &org_idx: order)
			{
				Individual &org = individuals[org_idx];
				org.DecideAndExecute(World, Pop);

				// Using as fitness the score
				auto state = (OrgState*)org.GetState();
				org.Fitness = state->Score;
			}
		}
	}

private:
    float2 findNearestFood (void *State, void *World) {
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
        return nearest_pos;
    };
};


int main()
{
	// TODO : REMOVE THIS! I put it here just in the off case that the reason why the population is not improving is because some parameter wasn't loaded
	NEAT::load_neat_params("../cfg/neat_test_config.txt");

	// Create base interface
	Interface = unique_ptr<PublicInterface>(new TestInterface);
	Interface->Init();

	// Create and fill the world
	WorldData world;
	world.FoodPositions.resize(FoodCellCount);
	for (auto& pos : world.FoodPositions)
	{
		pos.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
		pos.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
	}

	// Spawn population
	Population pop;
	pop.Spawn(PopulationSize);
	
	// Do evolution loop
	vector<float> fitness_vec(PopulationSize);
	vector<float> novelty_vec(PopulationSize);
	vector<float> fitness_vec_registry;
	vector<float> novelty_vec_registry;
	vector<float> avg_fitness;
	vector<float> avg_novelty;
	vector<float> avg_fitness_difference;
	vector<float> avg_fitness_network;
	vector<float> avg_fitness_random;
	vector<float> avg_novelty_registry;
	vector<float> species_count;

	for (int g = 0; g < GenerationsCount; g++)
	{
		pop.Epoch(&world, [&](int gen)
		{
			cout << "Generation : " << gen << endl;

			// Every some generations graph the fitness & novelty of the individuals of the registry
			if (gen % 10 == 0)
			{
				for (auto [idx, org] : enumerate(pop.GetIndividuals()))
				{
					fitness_vec[idx] = org.LocalScore;//org.LastFitness;
					novelty_vec[idx] = org.LastNoveltyMetric;
				}

                float avg_f = accumulate(fitness_vec.begin(), fitness_vec.end(), 0.0f) / fitness_vec.size();
                float avg_n = accumulate(novelty_vec.begin(), novelty_vec.end(), 0.0f) / novelty_vec.size();

                auto metrics = pop.ComputeProgressMetrics(&world, 10);
                avg_fitness_difference.push_back(metrics.AverageFitnessDifference);
                avg_fitness_network.push_back(metrics.AverageFitness);
                avg_fitness_random.push_back(metrics.AverageRandomFitness);
                avg_novelty_registry.push_back(metrics.AverageNovelty);

                avg_fitness.push_back(avg_f);
                avg_novelty.push_back(avg_n);

                plt::clf();

                plt::subplot(5, 1, 1);
                plt::plot(fitness_vec, novelty_vec, "x");

                plt::subplot(5, 1, 2);
                plt::plot(avg_fitness, "r");

                plt::subplot(5, 1, 3);
                plt::plot(avg_novelty, "g");

                plt::subplot(5, 1, 4);
                plt::plot(avg_fitness_network, "r");
                plt::plot(avg_fitness_random, "b");

                plt::subplot(5, 1, 5);
                plt::plot(avg_novelty_registry, "g");

                plt::pause(0.01);
            }
        });
	}

	// Generations finished, so visualize the individuals

	// Find the best 5
	{
		auto comparator = [](pair<int, float> a, pair<int, float> b) { return a.second > b.second; };
		priority_queue<pair<int, float>, vector<pair<int, float>>, decltype(comparator)> prey_queue(comparator);

		for (auto[idx, org] : enumerate(pop.GetIndividuals()))
		{
            prey_queue.push({ idx, org.Fitness });
            if (prey_queue.size() == 6)
                prey_queue.pop();
		}

		vector<Individual> tmp;
		while (prey_queue.size() > 0)
		{
			tmp.push_back(move(pop.GetIndividuals()[prey_queue.top().first]));
			prey_queue.pop();
		}

		pop.GetIndividuals() = move(tmp);
	}

    for (auto& org : pop.GetIndividuals())
        org.Reset();

    vector<int> food_x, food_y;
    food_x.resize(world.FoodPositions.size());
    food_y.resize(world.FoodPositions.size());

    while (true)
    {
        for (auto[idx, pos] : enumerate(world.FoodPositions))
        {
            food_x[idx] = pos.x;
            food_y[idx] = pos.y;
        }

        for (auto &org : pop.GetIndividuals())
            org.DecideAndExecute(&world, &pop);

        // Plot food
        plt::clf();
        plt::plot(food_x, food_y, "gx");

        // Plot herbivores on blue and carnivores on black
        vector<int> herbivores_x, herbivores_y;
        herbivores_x.reserve(PopulationSize);
        herbivores_y.reserve(PopulationSize);

        for (const auto &org : pop.GetIndividuals())
        {
            auto state_ptr = (OrgState *) org.GetState();

            herbivores_x.push_back(state_ptr->Position.x);
            herbivores_y.push_back(state_ptr->Position.y);
        }
        plt::plot(herbivores_x, herbivores_y, "ob");

        plt::xlim(-1, WorldSizeX);
        plt::ylim(-1, WorldSizeY);
        plt::pause(0.1);
    }
}