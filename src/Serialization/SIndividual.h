#pragma once

#include <unordered_map>

#include <boost/serialization/access.hpp>
#include <boost/serialization/unordered_map.hpp>


#include "SParameter.h"
#include "SNetwork.h"
#include "../Interface/BaseIndividual.h"
#include "../Evolution/MorphologyTag.h"

namespace NEAT {
	class Genome;
}

namespace agio
{
	class Individual;

	class SIndividual : public BaseIndividual
	{
	public:
		std::unordered_map<int, SParameter> parameters;
		SNetwork brain;
		void *state;
		MorphologyTag morphologyTag;

		SIndividual();
		SIndividual(NEAT::Genome *genome);

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & parameters;
			ar & brain;
		}

		void DecideAndExecute(void * World, const std::vector<BaseIndividual*> &Individuals) override;
		int DecideAction(const std::unordered_map<int, float>& SensorsValues) override;
		void Reset() override;

		void * GetState() const override;
		const std::unordered_map<int, Parameter>& GetParameters() const override;
		const MorphologyTag& GetMorphologyTag() const override;
	};
}