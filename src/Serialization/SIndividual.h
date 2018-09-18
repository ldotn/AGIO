#pragma once

#include <unordered_map>

#include <boost/serialization/access.hpp>
#include <boost/serialization/unordered_map.hpp>


#include "SParameter.h"

namespace agio {
    class Individual;
}

using namespace agio;

class SIndividual {
public:
    int speciesId;
    std::unordered_map<int, SParameter> parameters;

    SIndividual();
    SIndividual(Individual &individual);

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & speciesId;
        ar & parameters;
    }
};