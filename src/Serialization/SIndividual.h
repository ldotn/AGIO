#pragma once

#include <unordered_map>
#include <vector>

#include <boost/serialization/access.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>


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
		std::unordered_map<int, Parameter> parameters;
		SNetwork brain;
		MorphologyTag morphologyTag;

		std::vector<int> Actions;
		std::vector<int> Sensors;

		SIndividual();
		SIndividual(NEAT::Genome *genome, MorphologyTag morphologyTag);
		~SIndividual() override = default; // SNetworks know how to clean themselves
		SIndividual(SIndividual&&) = default; // Only SNetwork is not trivially moved, and that implements it's own move operators

		// Creates a clone
		// Can't just make a copy because there are pointers involved
		void Duplicate(SIndividual& CloneTarget) const
		{
			CloneTarget.parameters = parameters;
			CloneTarget.Actions = Actions;
			CloneTarget.Sensors = Sensors;
			brain.Duplicate(CloneTarget.brain);
		}

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & parameters;
			ar & brain;
			ar & Actions;
			ar & Sensors;
		}

		void DecideAndExecute(void * World, const std::vector<BaseIndividual*> &Individuals) override;
		int DecideAction(const std::unordered_map<int, float>& SensorsValues) override;
		void Reset() override;

		void * GetState() const override;
		const std::unordered_map<int, Parameter>& GetParameters() const override;
		const MorphologyTag& GetMorphologyTag() const override;
	};
}