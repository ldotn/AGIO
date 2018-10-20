#pragma once

#include <unordered_map>

#include <boost/serialization/access.hpp>
#include <boost/serialization/unordered_map.hpp>


#include "SParameter.h"
#include "SNetwork.h"

namespace NEAT {
	class Genome;
}

namespace agio
{
	class Individual;

	class SIndividual 
	{
	public:
		std::unordered_map<int, SParameter> parameters;
		SNetwork brain;

		SIndividual();
		SIndividual(NEAT::Genome *genome);

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & parameters;
			ar & brain;
		}
	};
}