#include "SRegistry.h"
#include <unordered_map>
#include <vector>

#include "../Evolution/Population.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace agio;
using namespace std;

SRegistry::SRegistry() {}

SRegistry::SRegistry(agio::Population *pop)
{
    unordered_map<MorphologyTag,vector<SIndividual>> individuals;

    for (auto speciesRecord: pop->GetSpeciesRegistry())
        individuals[speciesRecord.Morphology].emplace_back(speciesRecord.HistoricalBestGenome, speciesRecord.Morphology);


    for (auto & [morphology, individual] : individuals)
        Species.emplace_back(move(morphology),move(individual));
}

void SRegistry::save(const std::string& filename)
{
    std::ofstream ofs(filename);
    {
        boost::archive::text_oarchive oa(ofs);
        oa << *this;
    }
}

void SRegistry::load(const std::string &filename)
{

    {
        std::ifstream ifs(filename);
        boost::archive::text_iarchive ia(ifs);

        ia >> *this;
    }
}


