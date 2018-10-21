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

    brain = SNetwork(genome->genesis(genome->genome_id));

    // Reconstruct actions and sensors
    unordered_set<int> actions_set;
    unordered_set<int> sensors_set;

    for(const auto &componentRef : morphologyTag) {
        const ComponentGroup &group = Interface->GetComponentRegistry()[componentRef.GroupID];
        Component component = group.Components[componentRef.ComponentID];

        actions_set.insert(component.Actions.begin(), component.Actions.end());
        sensors_set.insert(component.Sensors.begin(), component.Sensors.end());
    }

    // Convert the action and sensors set to vectors
    Actions.resize(actions_set.size());
    for (auto[idx, action] : enumerate(actions_set))
        Actions[idx] = action;

    Sensors.resize(sensors_set.size());
    for (auto[idx, sensor] : enumerate(sensors_set))
        Sensors[idx] = sensor;

    // Sort the actions and the sensors vectors
    // This is important! Otherwise mating between individuals is meaningless, because the order is arbitrary
    //  and the same input could mean different things for two individuals of the same species
    sort(Actions.begin(), Actions.end());
    sort(Sensors.begin(), Sensors.end());
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

void* SIndividual::GetState() const
{
    return State;
}

const std::unordered_map<int, Parameter>& SIndividual::GetParameters() const
{
    return parameters;
}

int SIndividual::DecideAction(const std::unordered_map<int, float> &ValuesMap)
{
    std::vector<float> ActivationsBuffer(Actions.size());
    std::vector<double> SensorsValues(Sensors.size());

    // Load sensors
    for (auto [value, idx] : zip(SensorsValues, Sensors))
        value = ValuesMap.find(idx)->second; // TODO : Check if the value exists first

    // Send sensors to brain and activate
    brain.load_sensors(SensorsValues);
    bool sucess = brain.activate(); // TODO : Handle failure

    // Select action based on activations
    float act_sum = 0;
    for (auto[idx, v] : enumerate(brain.outputs))
        act_sum += ActivationsBuffer[idx] = v->activation; // The activation function is in [0, 1], check line 461 of neat.cpp

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

    // Return final action id
    return Actions[action];
}

void SIndividual::DecideAndExecute(void *World, const std::vector<BaseIndividual *> &Individuals)
{
    std::vector<float> ActivationsBuffer(Actions.size());
    std::vector<double> SensorsValues(Sensors.size());

    // Load sensors
    for (auto[value, idx] : zip(SensorsValues, Sensors))
        value = Interface->GetSensorRegistry()[idx].Evaluate(State, Individuals, this, World);

    // Send sensors to brain and activate
    brain.load_sensors(SensorsValues);
    bool sucess = brain.activate(); // TODO : Handle failure

    // Select action based on activations
    float act_sum = 0;
    for (auto[idx, v] : enumerate(brain.outputs))
        act_sum += ActivationsBuffer[idx] = v->activation; // The activation function is in [0, 1], check line 461 of neat.cpp

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

    // Return final action id
    // Finally execute the action
    Interface->GetActionRegistry()[Actions[action]].Execute(State, Individuals, this, World);
}