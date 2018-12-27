#include "Greedy.h"

#include "../PreyPredator/PublicInterfaceImpl.h"
#include <vector>

GreedyBase::GreedyBase() {
    ComponentRef mouth{0,0};
    ComponentRef body{1,0};
    tag.push_back(mouth);
    tag.push_back(body);

	State = Interface->MakeState(this);
}

/*
int GreedyBase::DecideAction(const std::unordered_map<int, float> &SensorsValues) {
    throw "Not implemented";
}*/

const MorphologyTag& GreedyBase::GetMorphologyTag() const {
    return tag;
}

void* GreedyBase::GetState() const {
    return State;
}

void GreedyBase::Reset() {
    Interface->ResetState(State);
}

const std::unordered_map<int, Parameter>& GreedyBase::GetParameters() const {
    return parameters;
}

void GreedyPrey::DecideAndExecute(void *World, const std::vector<BaseIndividual *> &Individuals) {
	float food_distance_x = Interface->GetSensorRegistry()[(int)SensorsIDs::NearestFoodDeltaX].Evaluate(State, Individuals, this, World);
	float food_distance_y = Interface->GetSensorRegistry()[(int)SensorsIDs::NearestFoodDeltaY].Evaluate(State, Individuals, this, World);

	if (food_distance_x == 0 && food_distance_y == 0)
		Interface->GetActionRegistry()[(int)ActionsIDs::EatFood].Execute(State, Individuals, this, World);
	else
	{
		if (food_distance_x >= 1)
			Interface->GetActionRegistry()[(int)ActionsIDs::MoveRight].Execute(State, Individuals, this, World);
		else if (food_distance_x <= -1)
			Interface->GetActionRegistry()[(int)ActionsIDs::MoveLeft].Execute(State, Individuals, this, World);
		else if (food_distance_y >= 1)
			Interface->GetActionRegistry()[(int)ActionsIDs::MoveForward].Execute(State, Individuals, this, World);
		else if (food_distance_y <= -1)
			Interface->GetActionRegistry()[(int)ActionsIDs::MoveBackwards].Execute(State, Individuals, this, World);
	}
}

void GreedyPredator::DecideAndExecute(void *World, const std::vector<BaseIndividual *> &Individuals) {
	float prey_distance_x = Interface->GetSensorRegistry()[(int)SensorsIDs::NearestCompetitorDeltaX].Evaluate(State, Individuals, this, World);
	float prey_distance_y = Interface->GetSensorRegistry()[(int)SensorsIDs::NearestCompetitorDeltaY].Evaluate(State, Individuals, this, World);

	if (prey_distance_x == 0 && prey_distance_y == 0)
		Interface->GetActionRegistry()[(int)ActionsIDs::KillAndEat].Execute(State, Individuals, this, World);
	else
	{
		if (prey_distance_x >= 1)
			Interface->GetActionRegistry()[(int)ActionsIDs::MoveRight].Execute(State, Individuals, this, World);
		else if (prey_distance_x <= -1)
			Interface->GetActionRegistry()[(int)ActionsIDs::MoveLeft].Execute(State, Individuals, this, World);
		else if (prey_distance_y >= 1)
			Interface->GetActionRegistry()[(int)ActionsIDs::MoveForward].Execute(State, Individuals, this, World);
		else if (prey_distance_y <= -1)
			Interface->GetActionRegistry()[(int)ActionsIDs::MoveBackwards].Execute(State, Individuals, this, World);
	}
}