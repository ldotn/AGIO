#include "../../Evolution/Population.h"
#include "../../Serialization/SPopulation.h"
#include "../../Evolution/Globals.h"
#include <assert.h>
#include "../../Utils/Math/Float2.h"
#include <algorithm>
#include <random>
#include <matplotlibcpp.h>
#include <enumerate.h>
#include <queue>
#include "../../Core/Config.h"

namespace plt = matplotlibcpp;
using namespace std;
using namespace agio;
using namespace fpp;

// TODO : Refactor, this could be spread on a couple files. 
//   Need to see if that's actually better though
const int WorldSizeX = 50;
const int WorldSizeY = 50;
const float FoodScoreGain = 20;
const float KillScoreGain = 30;
const float DeathPenalty = 0;// 400;
const int FoodCellCount = WorldSizeX * WorldSizeY*0.05;
const int MaxSimulationSteps = 50;
const int SimulationSize = 10; // Population is simulated in batches
const int PopSizeMultiplier = 30; // Population size is a multiple of the simulation size
const int PopulationSize = PopSizeMultiplier * SimulationSize;
const int GenerationsCount = 3;
const float LifeLostPerTurn = 5;
const float BorderPenalty = 0;// 80; // penalty when trying to go out of bounds
const float WastedActionPenalty = 0;// 5; // penalty when doing an action that has no valid target (like eating and there's no food close)

minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

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

		// Test : Instead of a circular world, make them bounce on walls
		//auto cycle_x = [](int x) { return x < WorldSizeX ? (x > 0 ? x : 2) : WorldSizeX - 2; };
		//auto cycle_y = [](int y) { return y < WorldSizeY ? (y > 0 ? y : 2) : WorldSizeY - 2; };


		ActionRegistry[(int)ActionsIDs::MoveForward] = Action
		(
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
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
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
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
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
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
			[&](void * State, const Population * Pop,Individual * Org, void * World) 
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
			[](void * State, const Population * Pop, Individual * Org, void * World)
			{
				auto state_ptr = (OrgState*)State;


				const auto& tag = Org->GetMorphologyTag();
				
				// Find an individual of a DIFFERENT species.
				// The species map is not really useful here
				bool any_eaten = false;
				for (const auto& individual : Pop->GetIndividuals())
				{
					// Ignore individuals that aren't being simulated right now
					// Also, don't do all the other stuff against yourself. 
					// You already know you don't want to kill yourself
					if (!individual.InSimulation || individual.GetGlobalID() == Org->GetGlobalID())
						continue;

					auto other_state_ptr = (OrgState*)individual.GetState();
					auto diff = abs >> (other_state_ptr->Position - state_ptr->Position);
					if (diff.x <= 1 && diff.y <= 1)
					{
						// Check if they are from different species by comparing tags
						const auto& other_tag = individual.GetMorphologyTag();

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
			[](void * State, void * World, const Population* Pop, const Individual * Org)
			{
				return ((OrgState*)State)->Score;
			}
		);
		// Normalized position
		SensorRegistry[(int)SensorsIDs::CurrentPosX] = Sensor
		(
			[](void * State, void * World, const Population* Pop, const Individual * Org)
			{
				return((OrgState*)State)->Position.x / (float)WorldSizeX;
			}
		);
		SensorRegistry[(int)SensorsIDs::CurrentPosY] = Sensor
		(
			[](void * State, void * World, const Population* Pop, const Individual * Org)
			{
				return ((OrgState*)State)->Position.y / (float)WorldSizeY;
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
		SensorRegistry[(int)SensorsIDs::NearestFoodDistance] = Sensor
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

				return nearest_dist;
			}
		);
		SensorRegistry[(int)SensorsIDs::NearestPartnerAngle] = Sensor
		(
			[](void * State, void * World,const Population* Pop, const Individual * Org)
			{
				// TODO : USE THE SPECIES MAP HERE!
				const auto & tag = Org->GetMorphologyTag();

				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				float nearest_dist = numeric_limits<float>::max();
				float2 nearest_pos;
				for (const auto& other_org : Pop->GetIndividuals())
				{
					// Ignore individuals that aren't being simulated right now
					// Also, don't do all the other stuff against yourself. 
					if (!other_org.InSimulation || other_org.GetGlobalID() == Org->GetGlobalID())
						continue;

					float dist = (((OrgState*)other_org.GetState())->Position - state_ptr->Position).length_sqr();
					if (dist < nearest_dist)
					{
						const auto & other_tag = other_org.GetMorphologyTag();

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
				const auto& tag = Org->GetMorphologyTag();

				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				float nearest_dist = numeric_limits<float>::max();
				float2 nearest_pos;
				for (const auto& other_org : Pop->GetIndividuals())
				{
					// Ignore individuals that aren't being simulated right now
					// Also, don't do all the other stuff against yourself. 
					if (!other_org.InSimulation || other_org.GetGlobalID() == Org->GetGlobalID())
						continue;

					float dist = (((OrgState*)other_org.GetState())->Position - state_ptr->Position).length_sqr();
					if (dist < nearest_dist)
					{
						const auto& other_tag = other_org.GetMorphologyTag();

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
		SensorRegistry[(int)SensorsIDs::NearestPartnerDistance] = Sensor
		(
			[](void * State, void * World,const Population* Pop, const Individual * Org)
			{
				// TODO : USE THE SPECIES MAP HERE!
				const auto & tag = Org->GetMorphologyTag();

				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				float nearest_dist = numeric_limits<float>::max();
				float2 nearest_pos;
				for (const auto& other_org : Pop->GetIndividuals())
				{
					// Ignore individuals that aren't being simulated right now
					// Also, don't do all the other stuff against yourself. 
					if (!other_org.InSimulation || other_org.GetGlobalID() == Org->GetGlobalID())
						continue;

					float dist = (((OrgState*)other_org.GetState())->Position - state_ptr->Position).length_sqr();
					if (dist < nearest_dist)
					{
						const auto & other_tag = other_org.GetMorphologyTag();

						if (tag == other_tag)
						{
							nearest_dist = dist;
							nearest_pos = ((OrgState*)other_org.GetState())->Position;
						}
					}
				}

				return nearest_dist;
			}
		);
		SensorRegistry[(int)SensorsIDs::NearestCompetidorDistance] = Sensor
		(
			[](void * State, void * World,const Population* Pop, const Individual * Org)
			{
				const auto& tag = Org->GetMorphologyTag();

				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				float nearest_dist = numeric_limits<float>::max();
				float2 nearest_pos;
				for (const auto& other_org : Pop->GetIndividuals())
				{
					// Ignore individuals that aren't being simulated right now
					// Also, don't do all the other stuff against yourself. 
					if (!other_org.InSimulation || other_org.GetGlobalID() == Org->GetGlobalID())
						continue;

					float dist = (((OrgState*)other_org.GetState())->Position - state_ptr->Position).length_sqr();
					if (dist < nearest_dist)
					{
						const auto& other_tag = other_org.GetMorphologyTag();

						if (!(tag == other_tag))
						{
							nearest_dist = dist;
							nearest_pos = ((OrgState*)other_org.GetState())->Position;
						}
					}
				}

				return nearest_dist;
			}
		);



		SensorRegistry[(int)SensorsIDs::NearestFoodX] = Sensor
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

				return nearest_pos.x;
			}
		);
		SensorRegistry[(int)SensorsIDs::NearestFoodY] = Sensor
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

				return nearest_pos.y;
			}
		);
		SensorRegistry[(int)SensorsIDs::NearestPartnerX] = Sensor
		(
			[](void * State, void * World,const Population* Pop, const Individual * Org)
			{
				// TODO : USE THE SPECIES MAP HERE!
				const auto & tag = Org->GetMorphologyTag();

				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				float nearest_dist = numeric_limits<float>::max();
				float2 nearest_pos;
				for (const auto& other_org : Pop->GetIndividuals())
				{
					// Ignore individuals that aren't being simulated right now
					// Also, don't do all the other stuff against yourself. 
					if (!other_org.InSimulation || other_org.GetGlobalID() == Org->GetGlobalID())
						continue;

					float dist = (((OrgState*)other_org.GetState())->Position - state_ptr->Position).length_sqr();
					if (dist < nearest_dist)
					{
						const auto & other_tag = other_org.GetMorphologyTag();

						if (tag == other_tag)
						{
							nearest_dist = dist;
							nearest_pos = ((OrgState*)other_org.GetState())->Position;
						}
					}
				}

				return nearest_pos.x;
			}
		);
		SensorRegistry[(int)SensorsIDs::NearestPartnerY] = Sensor
		(
			[](void * State, void * World,const Population* Pop, const Individual * Org)
			{
				// TODO : USE THE SPECIES MAP HERE!
				const auto & tag = Org->GetMorphologyTag();

				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				float nearest_dist = numeric_limits<float>::max();
				float2 nearest_pos;
				for (const auto& other_org : Pop->GetIndividuals())
				{
					// Ignore individuals that aren't being simulated right now
					// Also, don't do all the other stuff against yourself. 
					if (!other_org.InSimulation || other_org.GetGlobalID() == Org->GetGlobalID())
						continue;

					float dist = (((OrgState*)other_org.GetState())->Position - state_ptr->Position).length_sqr();
					if (dist < nearest_dist)
					{
						const auto & other_tag = other_org.GetMorphologyTag();

						if (tag == other_tag)
						{
							nearest_dist = dist;
							nearest_pos = ((OrgState*)other_org.GetState())->Position;
						}
					}
				}

				return nearest_pos.y;
			}
		);
		SensorRegistry[(int)SensorsIDs::NearestCompetidorX] = Sensor
		(
			[](void * State, void * World,const Population* Pop, const Individual * Org)
			{
				const auto& tag = Org->GetMorphologyTag();

				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				float nearest_dist = numeric_limits<float>::max();
				float2 nearest_pos;
				for (const auto& other_org : Pop->GetIndividuals())
				{
					// Ignore individuals that aren't being simulated right now
					// Also, don't do all the other stuff against yourself. 
					if (!other_org.InSimulation || other_org.GetGlobalID() == Org->GetGlobalID())
						continue;

					float dist = (((OrgState*)other_org.GetState())->Position - state_ptr->Position).length_sqr();
					if (dist < nearest_dist)
					{
						const auto& other_tag = other_org.GetMorphologyTag();

						if (!(tag == other_tag))
						{
							nearest_dist = dist;
							nearest_pos = ((OrgState*)other_org.GetState())->Position;
						}
					}
				}

				return nearest_pos.x;
			}
		);
		SensorRegistry[(int)SensorsIDs::NearestCompetidorY] = Sensor
		(
			[](void * State, void * World,const Population* Pop, const Individual * Org)
			{
				const auto& tag = Org->GetMorphologyTag();

				auto world_ptr = (WorldData*)World;
				auto state_ptr = (OrgState*)State;

				float nearest_dist = numeric_limits<float>::max();
				float2 nearest_pos;
				for (const auto& other_org : Pop->GetIndividuals())
				{
					// Ignore individuals that aren't being simulated right now
					// Also, don't do all the other stuff against yourself. 
					if (!other_org.InSimulation || other_org.GetGlobalID() == Org->GetGlobalID())
						continue;

					float dist = (((OrgState*)other_org.GetState())->Position - state_ptr->Position).length_sqr();
					if (dist < nearest_dist)
					{
						const auto& other_tag = other_org.GetMorphologyTag();

						if (!(tag == other_tag))
						{
							nearest_dist = dist;
							nearest_pos = ((OrgState*)other_org.GetState())->Position;
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

	virtual void * MakeState(const Individual * org) override
	{
		auto state = new OrgState;

		state->Score = 0;
		state->Position.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
		state->Position.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);

		// TODO : If the org mutates this becomes invalid...
		for (auto[gid, cid] : org->GetComponents())
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

	virtual void ResetState(void * State) override
	{
		auto state_ptr = (OrgState*)State;

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
};

#include "neat.h"
int main()
{
	// TODO : REMOVE THIS! I put it here just in the off case that the reason why the population is not improving is because some parameter wasn't loaded
	NEAT::load_neat_params("../cfg/neat_test_config.txt");
	NEAT::mutate_morph_param_prob = Settings::ParameterMutationProb;
	NEAT::destructive_mutate_morph_param_prob = Settings::ParameterDestructiveMutationProb;
	NEAT::mutate_morph_param_spread = Settings::ParameterMutationSpread;

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
	pop.Spawn(PopSizeMultiplier,SimulationSize);
	
	// Do evolution loop
	vector<float> fitness_vec_hervibore;
	vector<float> novelty_vec_hervibore;
	vector<float> fitness_vec_carnivore;
	vector<float> novelty_vec_carnivore;

	vector<float> fitness_vec_registry;
	vector<float> novelty_vec_registry;
	vector<float> avg_fitness;
	vector<float> avg_novelty;
	vector<float> avg_fitness_carnivore;
	vector<float> avg_novelty_carnivore;

	vector<float> avg_fitness_difference;
	vector<float> avg_fitness_network;
	vector<float> avg_fitness_random;
	vector<float> avg_novelty_registry;
	vector<float> species_count;
	vector<float> min_fitness_difference;
	vector<float> max_fitness_difference;

	// TODO : Make the plot work in debug
#ifndef _DEBUG
	plt::ion();
#endif

	for (int g = 0; g < GenerationsCount; g++)
	{
		pop.Epoch(&world, [&](int gen)
		{
			cout << "Generation : " << gen << endl;
			cout << "    Species Size [" << pop.GetSpecies().size() << "] : ";
			for (const auto&[_, species] : pop.GetSpecies())
				cout << species->IndividualsIDs.size() << " | " << species->ProgressMetric <<  " , ";
			cout << endl;

#ifdef _DEBUG
			auto metrics = pop.ComputeProgressMetrics(&world);
			cout << metrics.ProgressMetric << endl;
			return;
#endif
			// Every some generations graph the fitness & novelty of the individuals of the registry
			if (true)//(gen % 10 == 0)
			{
				fitness_vec_hervibore.resize(0);
				novelty_vec_hervibore.resize(0);
				fitness_vec_carnivore.resize(0);
				novelty_vec_carnivore.resize(0);

				for (auto [idx, org] : enumerate(pop.GetIndividuals()))
				{
					if (org.GetState<OrgState>()->IsCarnivore)
					{
						fitness_vec_carnivore.push_back(org.Fitness);
						novelty_vec_carnivore.push_back(org.LastNoveltyMetric);
					}
					else
					{
						fitness_vec_hervibore.push_back(org.Fitness);
						novelty_vec_hervibore.push_back(org.LastNoveltyMetric);
					}
				}

				/*float avg_f = 0;
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
				avg_n /= (float)registry_count;*/
#ifdef _DEBUG
				//cout << "    Avg Fitness (registry) : " << avg_f << " , Avg Novelty (registry) : " << avg_n << endl;
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

				//auto metrics = pop.ComputeProgressMetrics(&world);
				/*avg_fitness_difference.push_back(metrics.AverageFitnessDifference);
				avg_fitness_network.push_back(metrics.AverageFitness);
				avg_fitness_random.push_back(metrics.AverageRandomFitness);
				avg_novelty_registry.push_back(metrics.AverageNovelty);

				min_fitness_difference.push_back(metrics.MinFitnessDifference);
				max_fitness_difference.push_back(metrics.MaxFitnessDifference);*/

				plt::clf();

				plt::subplot(3, 1, 1);
				plt::plot(fitness_vec_hervibore, novelty_vec_hervibore, "xb");
				plt::plot(fitness_vec_carnivore, novelty_vec_carnivore, "xk");
				//plt::loglog(fitness_vec, novelty_vec, "x");

				// Only average the best 5
				sort(fitness_vec_hervibore.begin(), fitness_vec_hervibore.end(), [](float a, float b) { return a > b; });
				sort(fitness_vec_carnivore.begin(), fitness_vec_carnivore.end(), [](float a, float b) { return a > b; });

				float avg_f_hervibore = accumulate(fitness_vec_hervibore.begin(), fitness_vec_hervibore.begin() + min<int>(fitness_vec_hervibore.size(), 5), 0.0f) / 5.0f;
				float avg_n_hervibore = accumulate(novelty_vec_hervibore.begin(), novelty_vec_hervibore.begin() + min<int>(novelty_vec_hervibore.size(), 5), 0.0f) / 5.0f;
				float avg_f_carnivore = accumulate(fitness_vec_carnivore.begin(), fitness_vec_carnivore.begin() + min<int>(fitness_vec_carnivore.size(), 5), 0.0f) / 5.0f;
				float avg_n_carnivore = accumulate(novelty_vec_carnivore.begin(), novelty_vec_carnivore.begin() + min<int>(novelty_vec_carnivore.size(), 5), 0.0f) / 5.0f;
				for (auto[_, s] : pop.GetSpecies())
				{
					if (pop.GetIndividuals()[s->IndividualsIDs[0]].GetState<OrgState>()->IsCarnivore)
						avg_n_carnivore = s->ProgressMetric;
					else
						avg_n_hervibore = s->ProgressMetric;
				}

				avg_fitness.push_back((avg_f_hervibore));
				avg_fitness_carnivore.push_back((avg_f_carnivore));

				avg_novelty_carnivore.push_back(avg_n_carnivore);
				avg_novelty.push_back(avg_n_hervibore);


				//cout << metrics.AverageFitnessDifference << endl;


				plt::subplot(3, 1, 2);
				plt::plot(avg_fitness, "b");
				plt::plot(avg_fitness_carnivore, "k");

				plt::subplot(3, 1, 3);
				plt::plot(avg_novelty, "b");
				plt::plot(avg_novelty_carnivore, "k");

				//plt::subplot(4, 1, 4);
				//plt::plot(avg_fitness_difference, "r");
				//plt::plot(min_fitness_difference, "k");
				//plt::plot(max_fitness_difference, "b");

				//plt::subplot(5, 1, 5);
				//plt::plot(avg_novelty_registry, "g");

				plt::pause(0.01);
#endif
			}
		});

	}

	pop.save("population.txt");
	SPopulation spop = pop.load("population.txt");

	// Generations finished, so visualize the individuals
#ifndef _DEBUG
	{
		char c;
		cout << "Show simulation (y/n) ? ";
//		cin >> c;
		if (true)//c == 'n')
			return 0;
	}
	pop.EvaluatePopulation(&world);
	// After evolution is done, replace the population with the ones of the registry
//	pop.BuildFinalPopulation();
	//another interesting option would be to select 5 random individuals from the non dominated front, maybe for each species
	// Find the best 5 prey and predator
	{
		auto comparator = [](pair<int, float> a, pair<int, float> b) { return a.second > b.second; };
		priority_queue<pair<int, float>, vector<pair<int, float>>, decltype(comparator)> prey_queue(comparator), predator_queue(comparator);

		for (auto[idx, org] : enumerate(pop.GetIndividuals()))
		{
			if (org.GetState<OrgState>()->IsCarnivore)
			{
				predator_queue.push({ idx, org.Fitness });
				if (predator_queue.size() == 6)
					predator_queue.pop();
			}
			else
			{
				prey_queue.push({ idx, org.Fitness });
				if (prey_queue.size() == 6)
					prey_queue.pop();
			}
		}

		vector<Individual> tmp;
		while (predator_queue.size() > 0)
		{
			tmp.push_back(move(pop.GetIndividuals()[predator_queue.top().first]));
			predator_queue.pop();
		}

		while (prey_queue.size() > 0)
		{
			tmp.push_back(move(pop.GetIndividuals()[prey_queue.top().first]));
			prey_queue.pop();
		}

		for (auto& org : tmp)
			org.InSimulation = true;

		pop.GetIndividuals() = move(tmp);
	}

    //pop.GetIndividuals().resize(5);

	while (false)
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
				org.DecideAndExecute(&world, &pop);

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

			// Plot herbivores on blue and carnivores on black
			vector<int> herbivores_x, herbivores_y;
			vector<int> carnivores_x, carnivores_y;
			herbivores_x.reserve(PopulationSize);
			herbivores_y.reserve(PopulationSize);
			carnivores_x.reserve(PopulationSize);
			carnivores_y.reserve(PopulationSize);

			for (const auto& org : pop.GetIndividuals())
			{
				auto state_ptr = (OrgState*)org.GetState();

				any_alive = true;
				if (state_ptr->IsCarnivore)
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