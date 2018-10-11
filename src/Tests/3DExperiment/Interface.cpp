#include "Interface.h"
#include "../../Evolution/Population.h"
#include <math>
#include <chrono>

using namespace std;
using namespace agio;

float2 Circle::GetSamplePoint(minstd_rand0 RNG)
{
	float th = uniform_real<float>(0, 2 * 3.1416f)(RNG);
	float r = normal_distribution<float>()(RNG);
	r = atanf(r) * 2 / 3.1416f;

	return float2(cosf(th), sinf(th)) * r;
}

ExperimentInterface::ExperimentInterface()
	: RNG(chrono::high_resolution_clock::now().time_since_epoch().count())
{
}

void * ExperimentInterface::ResetState(void * State)
{
	auto state_ptr = (OrgState*)State;
	
	state_ptr->EatenCount = 0;
	state_ptr->Life = 100;
	state_ptr->Score = 0;

	// Set a random initial orientation
	{
		float theta = uniform_real_distribution<float>(0, 2 * 3.1416f)(rng);
		state_ptr->Orientation.x = cosf(theta);
		state_ptr->Orientation.y = sinf(theta);
	}

	// Find a random spawn area and spawn inside it
	int spawn_idx = uniform_int_distribution<>(0, World.SpawnAreas.size())(rng);
	state_ptr->Position = World.SpawnAreas[spawn_idx].GetSamplePoint(rng);
}

void * ExperimentInterface::MakeState(const Individual * org)
{
	auto state = new OrgState;
	
	ResetState(state);
	
	return state;
}

void ExperimentInterface::DestroyState(void * State)
{
	auto state_ptr = (OrgState*)State;
	delete state_ptr;
}

void * ExperimentInterface::DuplicateState(void * State)
{
	auto new_state = new OrgState;

	*OrgState = *(OrgState*)State;

	return new_state;
}

void ExperimentInterface::ComputeFitness(Population * Pop, void *)
{
	for (auto& org : Pop->GetIndividuals())
		org.Reset();

	// Reset the number of individuals eating plants
	for (auto& area : World.PlantsAreas)
		area.NumEatingInside = 0;

	for (int i = 0; i < MaxSimulationSteps; i++)
	{
		for (auto& org : Pop->GetIndividuals())
		{
			if (!org.InSimulation)
				continue;

			auto state_ptr = (OrgState*)org.GetState();
			state_ptr->Life -= GameplayParams::LifeLostPerTurn;
			if (state_ptr->Life <= 0)
			{
				org.Fitness = state_ptr->Score;
				continue;
			}

			org.DecideAndExecute(World, Pop);

			state_ptr->Score += state_ptr->Life;
			org.Fitness = state_ptr->Score;
		}
	}
}

void ExperimentInterface::Init()
{
	ActionRegistry.resize((int)ActionsIDs::NumberOfActions);

	ActionRegistry[(int)ActionsIDs::Walk] = Action
	(
		[&](void * State, const Population * Pop,Individual * Org, void * World) 
		{
			auto state_ptr = (OrgState*)State;

			state_ptr->Position += GameplayParams::WalkSpeed*state_ptr->Orientation;
			state_ptr->Position = GameplayParams::GameArea.ClampPos(state_ptr->Position);
		}
	);
	ActionRegistry[(int)ActionsIDs::Run] = Action
	(
		[&](void * State, const Population * Pop,Individual * Org, void * World) 
		{
			auto state_ptr = (OrgState*)State;

			state_ptr->Position += GameplayParams::RunSpeed*state_ptr->Orientation;
			state_ptr->Position = GameplayParams::GameArea.ClampPos(state_ptr->Position);
		}
	);
	ActionRegistry[(int)ActionsIDs::TurnLeft] = Action
	(
		[&](void * State, const Population * Pop,Individual * Org, void * World) 
		{
			auto state_ptr = (OrgState*)State;

			// TODO : This could be precomputed
			float th = -GameplayParams::TurnRadians; // Left = counterclockwise
			state_ptr->Position.x = state_ptr->Position.dot(float2(cosf(th), -sinf(th));
			state_ptr->Position.y = state_ptr->Position.dot(float2(sinf(th), cosf(th));
		}
	);
	ActionRegistry[(int)ActionsIDs::TurnRight] = Action
	(
		[&](void * State, const Population * Pop,Individual * Org, void * World) 
		{
			auto state_ptr = (OrgState*)State;

			// TODO : This could be precomputed
			float th = GameplayParams::TurnRadians; // Right = clockwise
			state_ptr->Position.x = state_ptr->Position.dot(float2(cosf(th), -sinf(th));
			state_ptr->Position.y = state_ptr->Position.dot(float2(sinf(th), cosf(th));
		}
	);
	ActionRegistry[(int)ActionsIDs::EatCorpse] = Action
	(
		[&](void * State, const Population * Pop,Individual * Org, void * World) 
		{
			auto state_ptr = (OrgState*)State;

			// Find an organism of a different species that's dead and nearby
			const auto& tag = Org->GetMorphologyTag();
			
			bool any_eaten = false;
			for (auto& individual : Pop->GetIndividuals())
			{
				// Ignore individuals that aren't being simulated right now
				// Also, don't do all the other stuff against yourself. 
				// You already know you don't want to eat yourself
				if (!individual.InSimulation || individual.GetGlobalID() == Org->GetGlobalID())
					continue;

				// Check first if it hasn't been eaten too many times or if it's life is > 0
				// You can only eat dead organisms that haven't been eaten too many times
				auto other_state_ptr = individual.GetState<OrgState>();
				if (other_state_ptr->Life > 0 || other_state_ptr->EatenCount >= GameplayParams::CorpseBitesDuration)
					continue;

				// Check if it's within range
				if ((other_state_ptr->Position - state_ptr->Position).length() < GameplayParams::EatDistance) // TODO : Use squared distance to save a sqrt call
				{
					// Now that we know it's in range, change if it's from a different species
					// Doing this at last because it requires iterating over the component list
					if (tag != individual.GetMorphologyTag())
					{
						any_eaten = true;
						other_state_ptr->EatenCount++;
						state_ptr->Life += GameplayParams::EatCorpseLifeGained;

						break;
					}
				}
			}

			// There's a configurable (could be 0) penalty for doing an action without a valid target
			if (!any_eaten)
				state_ptr->Score -= GameplayParams::WastedActionPenalty;
		}
	);
	ActionRegistry[(int)ActionsIDs::EatPlant] = Action
	(
		[&](void * State, const Population * Pop,Individual * Org, void*) 
		{
			auto state_ptr = (OrgState*)State;
			bool any_eaten = false;
			
			// Check if the organism is inside an area with plants
			for (auto& plant_area : World.PlantsAreas)
			{
				if (plant_area.Area.IsInside(state_ptr->Position))
				{
					any_eaten = true;
					plant_area.NumEatingInside++;
					state_ptr->Life += GameplayParams::EatPlantLifeGained / float(plant_area.NumEatingInside);
				}
			}

			// There's a configurable (could be 0) penalty for doing an action without a valid target
			if (!any_eaten)
				state_ptr->Score -= GameplayParams::WastedActionPenalty;
		}
	);
	ActionRegistry[(int)ActionsIDs::Attack] = Action
	(
		[&](void * State, const Population * Pop,Individual * Org, void * World) 
		{
			auto state_ptr = (OrgState*)State;

			// Find an organism of a different species that's not dead and nearby
			const auto& tag = Org->GetMorphologyTag();
			
			bool any_attacked = false;
			for (auto& individual : Pop->GetIndividuals())
			{
				// Ignore individuals that aren't being simulated right now
				// Also, don't do all the other stuff against yourself. 
				// You already know you don't want to eat yourself
				if (!individual.InSimulation || individual.GetGlobalID() == Org->GetGlobalID())
					continue;

				// Check first if the individual is alive
				auto other_state_ptr = individual.GetState<OrgState>();
				if (other_state_ptr->Life <= 0)
					continue;

				// Check if it's within range
				if ((other_state_ptr->Position - state_ptr->Position).length() < GameplayParams::EatDistance) // TODO : Use squared distance to save a sqrt call
				{
					// Now that we know it's in range, change if it's from a different species
					// Doing this at last because it requires iterating over the component list
					if (tag != individual.GetMorphologyTag())
					{
						any_attacked = true;

						other_state_ptr->Life -= GameplayParams::AttackDamage;

						break;
					}
				}
			}

			// There's a configurable (could be 0) penalty for doing an action without a valid target
			if (!any_attacked)
				state_ptr->Score -= GameplayParams::WastedActionPenalty;
		}
	);
}