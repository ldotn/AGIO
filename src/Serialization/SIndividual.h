#pragma once

#include <unordered_map>

#include <boost/serialization/access.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>


#include "SParameter.h"
#include "SNetwork.h"

namespace agio {
    class Individual;
}

using namespace agio;

class SIndividual {
public:
    std::vector<int> Actions;
    std::vector<int> Sensors;
    std::unordered_map<int, SParameter> parameters;
    SNetwork brain;
    void * state;
    int species_id;

    SIndividual();
    SIndividual(Individual &individual);

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & species_id;
        ar & parameters;
        ar & brain;
        ar & Actions;
        ar & Sensors;
    }
};