#include "MorphologyTag.h"
#include "enumerate.h"
#include <unordered_set>
#include <algorithm>
#include "Globals.h"

using namespace fpp;
using namespace std;
using namespace agio;

TagDesc::TagDesc(const MorphologyTag & InTag)
{
	Tag = InTag;

	// See below on the usage of unordered_set vs set
	unordered_set<int> actions_set;
	unordered_set<int> sensors_set;

	for (auto[gidx, cidx] : Tag)
	{
		const auto &component = Interface->GetComponentRegistry()[gidx].Components[cidx];

		actions_set.insert(component.Actions.begin(), component.Actions.end());
		sensors_set.insert(component.Sensors.begin(), component.Sensors.end());
	}

	ActionsCount = actions_set.size();
	SensorsCount = sensors_set.size();

	// Compute action and sensors vectors
	ActionIDs.resize(ActionsCount);
	for (auto[idx, action] : enumerate(actions_set))
		ActionIDs[idx] = action;

	SensorIDs.resize(SensorsCount);
	for (auto[idx, sensor] : enumerate(sensors_set))
		SensorIDs[idx] = sensor;

	// Sort the actions and the sensors vectors
	// This is important! Otherwise mating between individuals is meaningless, because the order is arbitrary
	//  and the same input could mean different things for two individuals of the same species
	// I could use a set instead of an unordered set, but want to be totally sure the vectors are in the exact same position
	sort(ActionIDs.begin(), ActionIDs.end());
	sort(SensorIDs.begin(), SensorIDs.end());
}
