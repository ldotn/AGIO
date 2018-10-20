#include "SIndividual.h"
#include "SParameter.h"
#include "../Evolution/Individual.h"

//NEAT
#include "genome.h"

using namespace std;
using namespace agio;

SIndividual::SIndividual() {}

SIndividual::SIndividual(NEAT::Genome *genome)
{
    for(auto const&[key, param] : genome->MorphParams)
        parameters.emplace(key, SParameter(param.ID, param.Value));

    brain = SNetwork(genome->genesis(genome->genome_id));
}