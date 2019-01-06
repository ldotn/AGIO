#pragma once

#include "../../Interface/BaseIndividual.h"
#include "../../Evolution/MorphologyTag.h"
#include "../PreyPredator/PublicInterfaceImpl.h"

#include "genome.h"
#include <unordered_map>

inline int DecideGreedyPrey(const std::vector<float>& SensorValues, BaseIndividual *org) {
	float food_distance_x = SensorValues[org->GetSensorIndex((int)SensorsIDs::NearestFoodDeltaX)];
	float food_distance_y = SensorValues[org->GetSensorIndex((int)SensorsIDs::NearestFoodDeltaY)];

	ActionsIDs action;
	if (abs(food_distance_x) <= 1 && abs(food_distance_y) <= 1)
		action = ActionsIDs::EatFood;
	else
	{
		if (food_distance_x > 0)
			action = ActionsIDs::MoveRight;
		else if (food_distance_x < 0)
			action = ActionsIDs::MoveLeft;
		else if (food_distance_y > 0)
			action = ActionsIDs::MoveForward;
		else
			action = ActionsIDs::MoveBackwards;
	}

	return (int) action;
}

inline int DecideGreedyPredator(const std::vector<float>& SensorValues, BaseIndividual *org) {
	float prey_distance_x = SensorValues[org->GetSensorIndex((int)SensorsIDs::NearestCompetitorDeltaX)];
	float prey_distance_y = SensorValues[org->GetSensorIndex((int)SensorsIDs::NearestCompetitorDeltaY)];
	
	ActionsIDs action;
	if (abs(prey_distance_x) <= 1 && abs(prey_distance_y) <= 1)
		action = ActionsIDs::KillAndEat;
	else
	{
		if (prey_distance_x > 0)
			action = ActionsIDs::MoveRight;
		else if (prey_distance_x < 0)
			action = ActionsIDs::MoveLeft;
		else if (prey_distance_y > 0)
			action = ActionsIDs::MoveForward;
		else
			action = ActionsIDs::MoveBackwards;
	}

	return (int) action;
}

inline agio::MorphologyTag createPreyTag()
{
	ComponentRef mouth{0,0};
	ComponentRef body{1,0};

	agio::MorphologyTag tag;
	tag.push_back(mouth);
	tag.push_back(body);

	return tag;
}

inline agio::MorphologyTag createPredatorTag()
{
	ComponentRef mouth{0,1};
	ComponentRef body{1,0};

	agio::MorphologyTag tag;
	tag.push_back(mouth);
	tag.push_back(body);

	return tag;
}

inline std::unordered_map<MorphologyTag, decltype(Individual::UserDecisionFunction)> createGreedyActionsMap()
{
	std::unordered_map<MorphologyTag, decltype(Individual::UserDecisionFunction)> map;
	map[createPreyTag()] = DecideGreedyPrey;
	map[createPredatorTag()] = DecideGreedyPredator;

	return map;
}