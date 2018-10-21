#pragma once

#include <unordered_map>
#include "genome.h"

class IPopulation;

class IIndividual {
    virtual void DecideAndExecute(void *World, const IPopulation *Pop) = 0;
    virtual std::unordered_map<int, Parameter> GetParameters() = 0;
    virtual void* GetState();
};

class IPopulation {

};