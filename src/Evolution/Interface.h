#pragma once

#include <unordered_map>

class IPopulation;

class IParameter {
    virtual float getValue() = 0;
};

class IIndividual {
    virtual void DecideAndExecute(void *World, const IPopulation *Pop) = 0;
    virtual std::unordered_map<int, IParameter*> GetParameters() = 0;
    virtual void* GetState();
};

class IPopulation {

};