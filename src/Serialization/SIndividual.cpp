#include "SIndividual.h"

#include <unordered_set>
#include <vector>

#include "../Evolution/Individual.h"
#include "enumerate.h"
#include "zip.h"

//NEAT
#include "genome.h"

using namespace std;
using namespace agio;
using namespace fpp;

SIndividual::SIndividual() {}

SIndividual::SIndividual(NEAT::Genome *genome, MorphologyTag morphology)
{
    this->morphologyTag = move(morphology);

    for(auto const&[key, param] : genome->MorphParams)
		parameters[key] = { param.ID, param.Value };

    brain = move(SNetwork(genome->genesis(genome->genome_id)));

    // Reconstruct actions and sensors
    TagDesc tag_desc(morphologyTag);
    Actions = tag_desc.ActionIDs;
    Sensors = tag_desc.SensorIDs;
}

const MorphologyTag& SIndividual::GetMorphologyTag() const
{
    return morphologyTag;
}

void SIndividual::Reset()
{
    brain.flush();
    Interface->ResetState(State);
}

const std::unordered_map<int, Parameter>& SIndividual::GetParameters() const
{
    return parameters;
}

int SIndividual::DecideAction()
{
	// Send sensors to brain and activate
	brain.load_sensors(SensorsValues);
	bool sucess = brain.activate(); // TODO : Handle failure

	// Select action based on activations
	float act_sum = 0;
	for (auto[idx, node_idx] : enumerate(brain.outputs))
		act_sum += ActivationsBuffer[idx] = brain.all_nodes[node_idx].activation; // The activation function is in [0, 1], check line 461 of neat.cpp

	std::minstd_rand RNG = minstd_rand(chrono::high_resolution_clock::now().time_since_epoch().count());
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
		uniform_int_distribution<int> action_dist(0, Actions.size() - 1);
		action = action_dist(RNG);
	}

	return action;
}

void SIndividual::DecideAndExecute(void *World, const std::vector<BaseIndividual *> &Individuals)
{
    // Load sensors
    for (auto[value, idx] : zip(SensorsValues, Sensors))
        value = Interface->GetSensorRegistry()[idx].Evaluate(State, Individuals, this, World);

	// Decide action
	int action = DecideAction();

    // Finally execute the action
    Interface->GetActionRegistry()[Actions[action]].Execute(State, Individuals, this, World);
}