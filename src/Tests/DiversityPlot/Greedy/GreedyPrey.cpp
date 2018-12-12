#include "GreedyPrey.h"

#include "../PublicInterfaceImpl.h"
#include <vector>

GreedyPrey::GreedyPrey() {
    ComponentRef mouth{0,0};
    ComponentRef body{1,0};
    tag.push_back(mouth);
    tag.push_back(body);
}

void GreedyPrey::DecideAndExecute(void *World, const std::vector<BaseIndividual *> &Individuals) {
    float food_distance_x = Interface->GetSensorRegistry()[(int)SensorsIDs::NearestFoodDistanceX].Evaluate(State, Individuals, this, World);
    float food_distance_y = Interface->GetSensorRegistry()[(int)SensorsIDs::NearestFoodDistanceY].Evaluate(State, Individuals, this, World);

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

int GreedyPrey::DecideAction(const std::unordered_map<int, float> &SensorsValues) {
    throw 'Not implemented';
}

const MorphologyTag& GreedyPrey::GetMorphologyTag() const {
    return tag;
}

void* GreedyPrey::GetState() const {
    return State;
}

void GreedyPrey::Reset() {
    Interface->ResetState(State);
}

const std::unordered_map<int, Parameter>& GreedyPrey::GetParameters() const {
    return parameters;
}