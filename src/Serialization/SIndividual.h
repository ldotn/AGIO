#pragma once

#include <boost/serialization/access.hpp>

namespace agio {
    class Individual;
}

using namespace agio;

class SIndividual {
public:
    int speciesId;

    SIndividual();
    SIndividual(Individual &individual);

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & speciesId;
    }
};