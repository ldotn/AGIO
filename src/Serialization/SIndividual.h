#pragma once

#include <unordered_map>

#include <boost/serialization/access.hpp>
#include <boost/serialization/unordered_map.hpp>


#include "SParameter.h"
#include "SNetwork.h"

namespace agio
{
	class Individual;

	class SIndividual 
	{
	public:
		int species_id;
		std::unordered_map<int, SParameter> parameters;
		SNetwork brain;

		SIndividual();
		SIndividual(Individual &individual);

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & species_id;
			ar & parameters;
			ar & brain;
		}
	};
}