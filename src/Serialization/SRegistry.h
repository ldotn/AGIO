#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "SIndividual.h"
#include "../Evolution/MorphologyTag.h"

#include <boost/serialization/vector.hpp>

namespace agio
{
	class SRegistry
	{
	public:
		struct Entry
		{
		    Entry(){}
		    Entry(MorphologyTag morphologyTag, std::vector<SIndividual>&& individuals) :
				Individuals(move(individuals)),
				Morphology(move(morphologyTag))
			{
		    }

			MorphologyTag Morphology;
			std::vector<SIndividual> Individuals;

            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive &ar, const unsigned int version)
            {
                ar & Morphology;
                ar & Individuals;
            }
		};
		std::vector<Entry> Species;

		SRegistry();
		SRegistry(class Population *pop);

        void save(const std::string& filename);
		void load(const std::string& filename);

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
            ar & Species;
        }
	};
}

