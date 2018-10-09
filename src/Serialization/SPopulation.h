#pragma once

#include <vector>
#include <unordered_map>

#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "SIndividual.h"

namespace agio 
{
	class Population;

    class SPopulation 
	{
	public:
		std::vector<SIndividual> individuals;
		std::unordered_map<int, std::vector<SIndividual*>> species_map;

		SPopulation();
		SPopulation(Population *population);

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & individuals;
			ar & species_map;
		}
	};
}

