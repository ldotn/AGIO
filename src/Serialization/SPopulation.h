#pragma once

#include <vector>
#include <unordered_map>

#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "SIndividual.h"

namespace agio {
    class Population;
};

using namespace std;
using namespace agio;

class SPopulation {
public:
    vector<SIndividual> sIndividuals;
    unordered_map<int, vector<SIndividual*>> speciesMap;

    SPopulation();
    SPopulation(Population *population);

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & sIndividuals;
        ar & speciesMap;
    }
};