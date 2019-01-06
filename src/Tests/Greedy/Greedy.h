#pragma once

#include "../../Interface/BaseIndividual.h"
#include "../../Evolution/MorphologyTag.h"
#include "genome.h"
#include <unordered_map>

class GreedyBase : public agio::BaseIndividual {
public:
    agio::MorphologyTag tag;
    std::unordered_map<int, agio::Parameter> parameters;

    virtual void DecideAndExecute(void * World, const std::vector<BaseIndividual*> &Individuals) = 0;
    int DecideAction() override;

    void Reset() override;
    const std::unordered_map<int, agio::Parameter>& GetParameters() const override;
    const agio::MorphologyTag& GetMorphologyTag() const override;
};

class GreedyPrey : public GreedyBase {
public:
    GreedyPrey();
	void DecideAndExecute(void * World, const std::vector<BaseIndividual*> &Individuals) override;
};

class GreedyPredator : public GreedyBase {
public:
    GreedyPredator();
	void DecideAndExecute(void * World, const std::vector<BaseIndividual*> &Individuals) override;
};