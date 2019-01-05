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
    int DecideAction(const std::unordered_map<int, float>& SensorsValues) override;

    void Reset() override;
    void * GetState() const override;
    const std::unordered_map<int, agio::Parameter>& GetParameters() const override;
    const agio::MorphologyTag& GetMorphologyTag() const override;
	GreedyBase();
};

class GreedyPrey : public GreedyBase {
public:
	void DecideAndExecute(void * World, const std::vector<BaseIndividual*> &Individuals) override;
};

class GreedyPredator : public GreedyBase {
public:
	void DecideAndExecute(void * World, const std::vector<BaseIndividual*> &Individuals) override;
};