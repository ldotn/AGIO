#include "Individual.h"
#include "Globals.h"
#include <random>
#include "enumerate.h"
#include "zip.h"
#include <algorithm>
#include <unordered_set>
#include <chrono>

// NEAT
#include "neat.h"
#include "network.h"
#include "population.h"
#include "organism.h"
#include "genome.h"
#include "species.h"

using namespace agio;
using namespace std;
using namespace fpp;

Individual::Individual() : RNG(chrono::high_resolution_clock::now().time_since_epoch().count())
{
	// TODO : Find if there's a way to avoid the memory allocations
	unordered_set<int> actions_set;
	unordered_set<int> sensors_set;

	// Construct the components list
	for (auto [gidx, group] : enumerate(Interface->GetComponentRegistry()))
	{
		// TODO : Profile what's the best way to do this
		int components_count = uniform_int_distribution<int>(group.MinCardinality, group.MaxCardinality)(RNG);
		
		vector<int> index_vec(group.Components.size());
		for (auto [idx, v] : enumerate(index_vec)) v = idx;
		
		shuffle(index_vec.begin(), index_vec.end(), RNG);

		for (int i = 0; i < components_count; i++)
		{
			Components.push_back({(int)gidx,index_vec[i] });

			const auto& component = group.Components[index_vec[i]];

			actions_set.insert(component.Actions.begin(), component.Actions.end());
			sensors_set.insert(component.Sensors.begin(), component.Sensors.end());
		}
	}

	// Construct the parameters list
	for (auto [pidx, param] : enumerate(Interface->GetParameterDefRegistry()))
	{
		int count = uniform_int_distribution<int>(param.MinCardinality, param.MaxCardinality)(RNG);

		for (int i = 0; i < count; i++)
			Parameters.push_back({ (int)pidx, uniform_real_distribution<float>(param.Min,param.Max)(RNG) });
	}

	// Convert the action and sensors set to vectors
	Actions.resize(actions_set.size());
	for (auto [idx, action] : enumerate(actions_set))
		Actions[idx] = action;

	Sensors.resize(sensors_set.size());
	for (auto [idx, sensor] : enumerate(sensors_set))
		Sensors[idx] = sensor;

	SensorsValues.resize(Sensors.size());
	ActivationsBuffer.resize(Actions.size());

	// Create base genome and network
	Genome = new NEAT::Genome(Sensors.size(), Actions.size(), 0, 0);
	Brain = Genome->genesis(Genome->genome_id);

	// Create a new state
	State = Interface->MakeState();
}

int Individual::DecideAction(void * World)
{
	// Load sensors
	for (auto [value, idx] : zip(SensorsValues, Sensors))
		value = Interface->GetSensorRegistry()[idx].Evaluate(State, World, this);

	// Send sensors to brain and activate
	Brain->load_sensors(SensorsValues);
	bool sucess = Brain->activate(); // TODO : Handle failure

	// Select action based on activations
	float act_sum = 0;
	for (auto [idx, v] : enumerate(Brain->outputs))
		act_sum += ActivationsBuffer[idx] = v->activation; // The activation function is in [0, 1], check line 461 of neat.cpp

	int action;
	if (act_sum > 1e-6)
	{
		discrete_distribution<int> action_dist(begin(ActivationsBuffer), end(ActivationsBuffer));
		action = action_dist(RNG);
	}
	else
	{
		// Can't decide on an action because all activations are 0
		// Select one action at random
		uniform_int_distribution<int> action_dist(0, Actions.size()-1);
		action = action_dist(RNG);
	}

	return action;
}

void Individual::Reset()
{
	Interface->ResetState(State);
	if (Brain) Brain->flush();
}