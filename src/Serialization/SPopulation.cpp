#include "SPopulation.h"
#include "../Evolution/Population.h"

SPopulation::SPopulation() {}

SPopulation::SPopulation(Population *population)
{
    // Save the individuals
    for(Individual &individual : population->Individuals)
        sIndividuals.emplace_back(SIndividual(individual));

    // Save the species map
    population->BuildSpeciesMap();
    int speciesId = 0;
    for (auto const &[_, species]: population->SpeciesMap)
    {
        vector<SIndividual *> speciesIndividuals;
        for (int individualId : species->IndividualsIDs)
        {
            SIndividual *individual = &sIndividuals[individualId];
            individual->speciesId = speciesId;

            speciesIndividuals.emplace_back(individual);
        }
        speciesMap.emplace(speciesId, speciesIndividuals);
        speciesId++;
    }
}
