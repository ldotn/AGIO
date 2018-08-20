#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include <assert.h>
#include "../../Utils/Math/Float2.h"
#include <algorithm>
#include <random>
#include <matplotlibcpp.h>
#include <enumerate.h>

namespace plt = matplotlibcpp;
using namespace std;
using namespace agio;
using namespace fpp;

// TODO : Refactor, this could be spread on a couple files. 
//   Need to see if that's actually better though
const int WorldSizeX = 30;
const int WorldSizeY = 30;
const float FoodLifeGain = 20;
const float KillLifeGain = 30;
const float DeathPenalty = 30;
const float StartingLife = 300;
const int FoodCellCount = WorldSizeX * WorldSizeY*0.1;
const int MaxSimulationSteps = 50;
const int PopulationSize = 100;
const int GenerationsCount = 10;
const float LifeLostPerTurn = 0;

minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

struct OrgState
{
	float Life = StartingLife;
	float2 Position;
	float Score = 0;
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

	NumberOfSensors
};

struct WorldData
{
	vector<float2> FoodPositions;
};

class TestInterface : public PublicInterface
{
public:
	virtual void Init() override
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

				state_ptr->Position.y = cycle_y(state_ptr->Position.y + 1);//clamp<int>(state_ptr->Position.y + 1, 0, WorldSizeY - 1);
			}
		);
		ActionRegistry[(int)ActionsIDs::MoveBackwards] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
			{
				auto state_ptr = (OrgState*)State;

				state_ptr->Position.y = cycle_y(state_ptr->Position.y - 1) % WorldSizeY;//clamp<int>(state_ptr->Position.y - 1, 0, WorldSizeY - 1);
			}
		);
		ActionRegistry[(int)ActionsIDs::MoveRight] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
			{
				auto state_ptr = (OrgState*)State;

				state_ptr->Position.x = cycle_x(state_ptr->Position.x + 1) % WorldSizeX;//clamp<int>(state_ptr->Position.x + 1, 0, WorldSizeX - 1);
			}
		);
		ActionRegistry[(int)ActionsIDs::MoveLeft] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
			{
				auto state_ptr = (OrgState*)State;

				state_ptr->Position.x = cycle_x(state_ptr->Position.x - 1) % WorldSizeX;// clamp<int>(state_ptr->Position.x - 1, 0, WorldSizeX - 1);
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

				auto [_, jump_dist] = *param_iter;

				state_ptr->Position.y = cycle_y(round(state_ptr->Position.y + jump_dist.Value));//clamp<int>((int)round(state_ptr->Position.y + jump_dist.Value), 0, WorldSizeY - 1);
			}
		);
		ActionRegistry[(int)ActionsIDs::JumpBackwards] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
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
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
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
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
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
			[](void * State, const Population * Pop, Individual * Org, void * World)
			{
				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				for (auto [idx,pos] : enumerate(world_ptr->FoodPositions))
				{
					auto diff = abs >> (pos - state_ptr->Position);

					if (diff.x <= 1 && diff.y <= 1)
					{
						//state_ptr->Life += FoodLifeGain;
						state_ptr->Score += FoodLifeGain;

						// Remove food and create a new one
						world_ptr->FoodPositions.erase(world_ptr->FoodPositions.begin() + idx);
						float2 food_pos(uniform_real_distribution<float>(0, WorldSizeX)(RNG), uniform_real_distribution<float>(0, WorldSizeY)(RNG));
						world_ptr->FoodPositions.push_back(food_pos);
						break;
					}
				}
			}
		);

		// Kill and eat and individual of a different species at most once cell away
		// This is a simple test, so no fighting is simulated
		ActionRegistry[(int)ActionsIDs::KillAndEat] = Action
		(
			[](void * State, const Population * Pop, Individual * Org, void * World)
			{
				auto state_ptr = (OrgState*)State;

				// TODO : Find a way to avoid the copy of the tag
				auto tag = Org->GetMorphologyTag();
				tag.Parameters = {};
				
				// Find an individual of a DIFFERENT species.
				// The species map is not really useful here
				for (const auto& individual : Pop->GetIndividuals())
				{
					if (individual.GetState<OrgState>()->Life <= 0)
						continue;

					auto other_state_ptr = (OrgState*)individual.GetState();
					auto diff = abs >> (other_state_ptr->Position - state_ptr->Position);
					if (diff.x <= 1 && diff.y <= 1)
					{
						// Check if they are from different species by comparing tags without parameters
						// TODO : Find a way to avoid the copy of the tag
						auto other_tag = individual.GetMorphologyTag();
						other_tag.Parameters = {};

						if (!(tag == other_tag))
						{
							//state_ptr->Life += KillLifeGain;
							state_ptr->Score += KillLifeGain;
							//other_state_ptr->Life = 0;
							other_state_ptr->Score -= DeathPenalty;
							other_state_ptr->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
							other_state_ptr->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
							break;
						}
					}
				}
			}
		);
		
		// Fill sensors
		SensorRegistry.resize((int)SensorsIDs::NumberOfSensors);

		SensorRegistry[(int)SensorsIDs::CurrentLife] = Sensor
		(
			[](void * State, void * World, const Population* Pop, const Individual * Org)
			{
				return ((OrgState*)State)->Life;
			}
		);
		SensorRegistry[(int)SensorsIDs::NearestFoodAngle] = Sensor
		(
			[](void * State, void * World, const Population* Pop, const Individual * Org)
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
		SensorRegistry[(int)SensorsIDs::NearestPartnerAngle] = Sensor
		(
			[](void * State, void * World,const Population* Pop, const Individual * Org)
			{
				// TODO : Find a way to avoid the copy of the tag
				auto tag = Org->GetMorphologyTag();
				tag.Parameters = {};

				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				float nearest_dist = numeric_limits<float>::max();
				float2 nearest_pos;
				for (const auto& other_org : Pop->GetIndividuals())
				{
					if (other_org.GetState<OrgState>()->Life <= 0)
						continue;

					float dist = (((OrgState*)other_org.GetState())->Position - state_ptr->Position).length_sqr();
					if (dist < nearest_dist)
					{
						auto other_tag = other_org.GetMorphologyTag();
						other_tag.Parameters = {};

						if (tag == other_tag)
						{
							nearest_dist = dist;
							nearest_pos = ((OrgState*)other_org.GetState())->Position;
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
			[](void * State, void * World,const Population* Pop, const Individual * Org)
			{
				// TODO : Find a way to avoid the copy of the tag
				auto tag = Org->GetMorphologyTag();
				tag.Parameters = {};

				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				float nearest_dist = numeric_limits<float>::max();
				float2 nearest_pos;
				for (const auto& other_org : Pop->GetIndividuals())
				{
					if (other_org.GetState<OrgState>()->Life <= 0)
						continue;

					float dist = (((OrgState*)other_org.GetState())->Position - state_ptr->Position).length_sqr();
					if (dist < nearest_dist)
					{
						auto other_tag = other_org.GetMorphologyTag();
						other_tag.Parameters = {};

						if (!(tag == other_tag))
						{
							nearest_dist = dist;
							nearest_pos = ((OrgState*)other_org.GetState())->Position;
						}
					}
				}

				// Compute "angle", taking as 0 = looking forward ([0,1])
				// The idea is
				//	angle = normalize(V - Pos) . [0,1]
				return (nearest_pos - state_ptr->Position).normalize().y;
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
		/*ParameterDefRegistry.resize((int)ParametersIDs::NumberOfParameters);

		ParameterDefRegistry[(int)ParametersIDs::JumpDistance] = ParameterDefinition
		(
			true, // require the parameter, even if the organism doesn't have the component to jump
			2, // Min bound
			5, // max bound
			(int)ParametersIDs::JumpDistance
		);*/

		// Fill components
		ComponentRegistry.push_back
		({
			1,1, // Can only have one mouth 
			{
				// Herbivore
				{
					{(int)ActionsIDs::EatFood},
					{(int)SensorsIDs::NearestFoodAngle}
				},
				// Carnivore
				{
					{(int)ActionsIDs::KillAndEat},
					{(int)SensorsIDs::NearestCompetidorAngle}
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
					},
					{ (int)SensorsIDs::CurrentLife } 
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
		});
		ComponentRegistry.push_back
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

	virtual void * MakeState(const class Individual *) override
	{
		auto state = new OrgState;

		state->Life = StartingLife;
		state->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
		state->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);

		return state;
	}

	virtual void ResetState(void * State) override
	{
		auto state_ptr = (OrgState*)State;

		state_ptr->Life = StartingLife;
		state_ptr->Score = 0;
		state_ptr->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
		state_ptr->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
	}

	virtual void DestroyState(void * State) override
	{
		auto state_ptr = (OrgState*)State;
		delete state_ptr;
	}

	virtual float Distance(class Individual *, class Individual *, void * World) override
	{
		assert(false);
		return 0; // Not used
	}

	virtual void * DuplicateState(void * State) override
	{
		auto new_state = new OrgState;
		*new_state = *(OrgState*)State;
		return new_state;
	}

	// How many steps did the last simulation took before everyone died
	int LastSimulationStepCount = 0;

	// This computes fitness for the entire population
	virtual void ComputeFitness(Population * Pop, void * World) override
	{
		for (auto& org : Pop->GetIndividuals())
			org.Reset();

		// TODO : Shuffle the evaluation order
		LastSimulationStepCount = 0;
		for (int i = 0; i < MaxSimulationSteps; i++)
		{
			for (auto& org : Pop->GetIndividuals())
			{
				auto state_ptr = (OrgState*)org.GetState();

				org.DecideAndExecute(World, Pop);

				// Using as fitness the accumulated life
				org.LastFitness = state_ptr->Score;
				// Second option : moving average
				//org.LastFitness = org.LastFitness*0.9f + state_ptr->Life*0.1f;
			}
			LastSimulationStepCount++;
		}
	}
};

#include "neat.h"
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
	vector<float> avg_novelty_registry;
	vector<float> species_count;
	// TODO : Make the plot work in debug
#ifndef _DEBUG
	plt::ion();
#endif

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
					fitness_vec[idx] = org.LastFitness;
					novelty_vec[idx] = org.LastNoveltyMetric;
				}

				float avg_f = 0;
				float avg_n = 0;

				int registry_count = 0;
				fitness_vec_registry.resize(0);
				novelty_vec_registry.resize(0);
				for (const auto&[_, individuals] : pop.GetNonDominatedRegistry())
				{
					for (const auto& org : individuals)
					{
						avg_f += org.LastFitness;
						avg_n += org.LastNoveltyMetric;
						
						fitness_vec_registry.push_back(org.LastFitness);
						novelty_vec_registry.push_back(org.LastNoveltyMetric);

						registry_count++;
					}
				}

				avg_f /= (float)registry_count;
				avg_n /= (float)registry_count;
#ifdef _DEBUG
				cout << "    Avg Fitness (registry) : " << avg_f << " , Avg Novelty (registry) : " << avg_n << endl;
#else
				/*auto metrics = pop.ComputeProgressMetrics(&world, 10);
				avg_fitness_difference.push_back(metrics.AverageFitnessDifference);
				avg_novelty_registry.push_back(metrics.AverageNovelty);

				avg_f = accumulate(fitness_vec.begin(), fitness_vec.end(), 0) / fitness_vec.size();
				avg_n = accumulate(novelty_vec.begin(), novelty_vec.end(), 0) / novelty_vec.size();

				avg_fitness.push_back(avg_f);
				avg_novelty.push_back(avg_n);
				species_count.push_back(pop.GetNonDominatedRegistry().size());

				plt::clf();

				plt::subplot(7, 1, 1);
				plt::loglog(fitness_vec, novelty_vec, "x");

				plt::subplot(7, 1, 2);
				plt::loglog(fitness_vec_registry, novelty_vec_registry, string("x"));

				plt::subplot(7, 1, 3);
				plt::plot(avg_fitness, "r");

				plt::subplot(7, 1, 4);
				plt::plot(avg_novelty, "g");

				plt::subplot(7, 1, 5);
				plt::plot(species_count, "k");

				plt::subplot(7, 1, 6);
				plt::plot(avg_fitness_difference, "r");
				//plt::hist(fitness_vec_registry);

				plt::subplot(7, 1, 7);
				plt::plot(avg_novelty_registry, "g");
				//plt::hist(novelty_vec_registry);*/

				avg_f = accumulate(fitness_vec.begin(), fitness_vec.end(), 0) / fitness_vec.size();
				avg_n = accumulate(novelty_vec.begin(), novelty_vec.end(), 0) / novelty_vec.size();

				auto metrics = pop.ComputeProgressMetrics(&world, 10);
				avg_fitness_difference.push_back(metrics.AverageFitnessDifference);
				avg_novelty_registry.push_back(metrics.AverageNovelty);

				avg_fitness.push_back(avg_f);
				avg_novelty.push_back(avg_n);

				plt::clf();

				plt::subplot(5, 1, 1);
				plt::loglog(fitness_vec, novelty_vec, "x");

				plt::subplot(5, 1, 2);
				plt::plot(avg_fitness, "r");

				plt::subplot(5, 1, 3);
				plt::plot(avg_novelty, "g");

				plt::subplot(5, 1, 4);
				plt::plot(avg_fitness_difference, "r");

				plt::subplot(5, 1, 5);
				plt::plot(avg_novelty_registry, "g");

				plt::pause(0.01);
#endif
			}
		});
	}

	// Generations finished, so visualize the individuals
#ifndef _DEBUG
	// After evolution is done, replace the population with the ones of the registry
	pop.BuildFinalPopulation();

	while (true)
	{
		for (auto& org : pop.GetIndividuals())
			org.Reset();

		vector<int> food_x, food_y;
		food_x.resize(world.FoodPositions.size());
		food_y.resize(world.FoodPositions.size());

		bool any_alive = true;
		//vector<int> org_x, org_y;
		//org_x.reserve(PopulationSize);
		//org_y.reserve(PopulationSize);

		while (any_alive)
		{
			for (auto[idx, pos] : enumerate(world.FoodPositions))
			{
				food_x[idx] = pos.x;
				food_y[idx] = pos.y;
			}

			any_alive = false;
			for (auto& org : pop.GetIndividuals())
			{
				auto state_ptr = (OrgState*)org.GetState();
				if (state_ptr->Life >= 0)
					org.DecideAndExecute(&world, &pop);
				state_ptr->Life -= LifeLostPerTurn;
			}

			// Plot food
			plt::clf();
			plt::plot(food_x, food_y, "gx");

			// Plot the individuals with different colors per species
			/*for (auto [species_idx,entry] : enumerate(pop.GetSpecies()))
			{
			auto [_, species] = entry;
			org_x.resize(species.IndividualsIDs.size());
			org_y.resize(species.IndividualsIDs.size());

			for (auto [idx, org_idx] : enumerate(species.IndividualsIDs))
			{
			auto state_ptr = (OrgState*)pop.GetIndividuals()[org_idx].GetState();
			if (state_ptr->Life)
			{
			any_alive = true;
			org_x[idx] = state_ptr->Position.x;
			org_y[idx] = state_ptr->Position.y;
			}
			}

			plt::plot(org_x, org_y, string("x") + string("C") + to_string(species_idx));
			}*/

			// Plot herbivores on green and carnivores on black
			vector<int> herbivores_x, herbivores_y;
			vector<int> carnivores_x, carnivores_y;
			herbivores_x.reserve(PopulationSize);
			herbivores_y.reserve(PopulationSize);
			carnivores_x.reserve(PopulationSize);
			carnivores_y.reserve(PopulationSize);

			for (const auto& org : pop.GetIndividuals())
			{
				bool is_carnivore = false;
				for (const auto& comp : org.GetComponents())
				{
					// Group 0 is the "mouth", component 1 of that group is carnivores mouth
					if (comp.GroupID == 0 && comp.ComponentID == 1)
						is_carnivore = true;
				}

				auto state_ptr = (OrgState*)org.GetState();
				if (state_ptr->Life <= 0)
					continue;

				any_alive = true;
				if (is_carnivore)
				{
					carnivores_x.push_back(state_ptr->Position.x);
					carnivores_y.push_back(state_ptr->Position.y);
				}
				else
				{
					herbivores_x.push_back(state_ptr->Position.x);
					herbivores_y.push_back(state_ptr->Position.y);
				}
			}
			plt::plot(herbivores_x, herbivores_y, "ob");
			plt::plot(carnivores_x, carnivores_y, "ok");
			
			plt::xlim(-1, WorldSizeX);
			plt::ylim(-1, WorldSizeY);
			plt::pause(0.1);
		}
	}
#endif
}