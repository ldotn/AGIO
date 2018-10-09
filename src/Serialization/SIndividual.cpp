#include "SIndividual.h"
#include "SParameter.h"
#include "../Evolution/Individual.h"

using namespace std;
using namespace agio;

SIndividual::SIndividual() {}

SIndividual::SIndividual(Individual &individual)
{
    for(auto const&[key, param] : individual.Parameters)
        parameters.emplace(key, SParameter(param.ID, param.Value));

    brain = SNetwork(individual.Brain);
}