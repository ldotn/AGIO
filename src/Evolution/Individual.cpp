#include <unordered_set>
#include <chrono>
#include <bitset>
#include <list>
#include <cassert>

#include "enumerate.h"
#include "zip.h"
#include "Individual.h"
#include "Population.h"
#include "../Core/Config.h"

// NEAT
#include "neat.h"
#include "network.h"
#include "organism.h"

using namespace agio;
using namespace std;
using namespace fpp;

Individual::Individual() noexcept
{
    State = nullptr;
    Genome = nullptr;
    Brain = nullptr;
    Fitness = 0;
    OriginalID = GlobalID = CurrentGlobalID.fetch_add(1);
    AccumulatedFitness = 0;
	NeedGenomeDeletion = false;
	RNG = minstd_rand(chrono::high_resolution_clock::now().time_since_epoch().count());
	UseMaxNetworkOutput = uniform_int_distribution<int>(0, 1)(RNG);
}

// it assumes that the sensors are already updated
int Individual::DecideAction()
{
	int action = -1;
		
	if (DecisionMethod == DecideRandomly)
	{
		uniform_int_distribution<int> action_dist(0, Actions.size() - 1);
		int action = action_dist(RNG);
	}
	else
	{
		if (DecisionMethod == UseUserFunction)
			action = ActionsMap[UserDecisionFunction(SensorsValues, this)];
		else
		{
			// Send sensors to brain and activate
			Brain->load_sensors(SensorsValues);
			bool sucess = Brain->activate(); // TODO : Handle failure

			// Select action based on activations
			float act_sum = 0;
			for (auto[idx, v] : enumerate(Brain->outputs))
				act_sum += (ActivationsBuffer[idx] = v->activation); // The activation function is in [0, 1], check line 461 of neat.cpp

			if (UseMaxNetworkOutput)
			{
				float max_v = ActivationsBuffer[0];
				action = 0;

				// Yeah, I know that I'm checking the first element twice, but the performance impact is negligible
				for (auto[idx, v] : enumerate(ActivationsBuffer))
				{
					if (v > max_v)
					{
						max_v = v;
						action = idx;
					}
				}
			}
			else
			{
				if (act_sum > 1e-6)
				{
					// Generate the PCF
					ActivationsBuffer[0] /= act_sum;
					for (int idx = 1; idx < ActivationsBuffer.size(); ++idx)
						ActivationsBuffer[idx] = ActivationsBuffer[idx] / act_sum + ActivationsBuffer[idx - 1];

					// Sample the action distribution
					float px = generate_canonical<float, sizeof(float)*8>(RNG);
					const auto firstIter = ActivationsBuffer.begin();
					const auto positionIter = lower_bound(firstIter, ActivationsBuffer.end(), px);
					action = static_cast<int>(positionIter - firstIter);
					// It can happen that due to numerical errors the last value of the activation buffer is not exactly 1
					//	and on that case positionIter would be equal to ActivationsBuffer.end()
					// Correct that here
					if (action == Actions.size()) --action;
				}
				else
				{
					// Can't decide on an action because all activations are 0
					// Select one action at random
					uniform_int_distribution<int> action_dist(0, Actions.size() - 1);
					action = action_dist(RNG);
				}
			}
		}
	}

	return action;
}

void Individual::DecideAndExecute(void *World, const vector<BaseIndividual*> &Individuals)
{
	// Update sensor values
	for (auto [value, idx] : zip(SensorsValues, Sensors))
		value = Interface->GetSensorRegistry()[idx].Evaluate(State, Individuals, this, World);

	// Decide and execute
	int action = DecideAction();
	return  Interface->GetActionRegistry()[Actions[action]].Execute(State, Individuals, this, World);
}

void Individual::Reset()
{
    Interface->ResetState(State);
    if (Brain) Brain->flush();
    Fitness = 0;
}

Individual::Individual(Individual &&other)
{
    OriginalID = other.OriginalID;
    GlobalID = other.GlobalID;
    State = other.State;
    Parameters = move(other.Parameters);
    Genome = other.Genome;
    Brain = other.Brain;
    Actions = move(other.Actions);
    Sensors = move(other.Sensors);
    SensorsValues = move(other.SensorsValues);
    ActivationsBuffer = move(other.ActivationsBuffer);
    RNG = other.RNG;
    Morphology = move(other.Morphology);
    Fitness = other.Fitness;
    AccumulatedFitness = other.AccumulatedFitness;
	UserDecisionFunction = move(other.UserDecisionFunction);
	DecisionMethod = other.DecisionMethod;
	SensorsMap = move(other.SensorsMap);
	ActionsMap = move(other.ActionsMap);

    other.Genome = nullptr;
    other.Brain = nullptr;
    other.State = nullptr;
}

Individual::Individual(const TagDesc& Desc, NEAT::Genome * InGenome) : Individual()
{
	Morphology = Desc.Tag;
	Genome = InGenome;
	Brain = Genome->genesis(Genome->genome_id);

	Actions = Desc.ActionIDs;
	Sensors = Desc.SensorIDs;

	for (auto[idx, id] : enumerate(Sensors))
		SensorsMap[id] = idx;
	for (auto[idx, id] : enumerate(Actions))
		ActionsMap[id] = idx;

	ActivationsBuffer.resize(Actions.size());
	SensorsValues.resize(Sensors.size());

	for (auto[pidx, param] : enumerate(Interface->GetParameterDefRegistry()))
	{
		// If the parameter is not required, select it randomly, 50/50 chance
		// TODO : Maybe make the selection probability a parameter? Does it makes sense?
		if (param.IsRequired || uniform_int_distribution<int>(0, 1)(RNG))
		{
			Parameter p{};
			p.ID = pidx;
			p.Value = 0.5f * (param.Min + param.Max);
			p.HistoricalMarker = Parameter::CurrentMarkerID;
			p.Min = param.Min;
			p.Max = param.Max;

			// Small shift
			float shift = normal_distribution<float>(0, Settings::ParameterMutationSpread)(RNG);
			p.Value += shift * abs(param.Max - param.Min);

			// Clamp values
			p.Value = clamp(p.Value, param.Min, param.Max);

			Parameters[param.UserID] = p;
		}
	}

	// Set morphological parameters of the genome
	// Make a copy
	Genome->MorphParams = Parameters;
	State = Interface->MakeState(this);
}

Individual::~Individual()
{
    if(Brain) delete Brain;

	// Don't delete the genome here
	// It's the same pointer that's stored on the (NEAT) population, so it'll be deleted when the population is deleted
	// If you delete it here, it'll crash later on when the neat pop is deleted
	// The only time you need to delete it, is when you are explicitly making a duplicate
	if(NeedGenomeDeletion && Genome) delete Genome;

    if (State) Interface->DestroyState(State);
}
