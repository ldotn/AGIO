#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "SIndividual.h"

// Need to define it first so that the map inside SRegistry sees the hash instanciation first
namespace agio
{
	struct SComponent
	{
		int GroupID;
		int ComponentID;

		bool operator==(const SComponent & other) const
		{
			return GroupID == other.GroupID &&
				ComponentID == other.ComponentID;
		}
	};

	typedef std::vector<SComponent> SMorphologyTag;
}

// This needs to be outside of the agio namespace
namespace std
{
	template <>
	struct hash<agio::SMorphologyTag>
	{
		std::size_t operator()(const agio::SMorphologyTag& k) const
		{
			// Ref : [https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector]
			std::size_t seed = k.size();

			//for (auto[gid, cid] : k)
			for (auto data : k) // No C++ 17 on UE4 :(
			{
				seed ^= data.GroupID + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= data.ComponentID + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}

			return seed;
		}
	};
}

namespace agio
{
	struct SRegistry
	{
		std::unordered_map<SMorphologyTag, std::vector<SIndividual>> Species;

		
		void LoadFromFile(const std::string& Path) 
		{ 
			/* TODO : Implement! 
				for record in StagnantSpecies
					org = SIndividual(record.HistoricalBestGenome)
					species[record.Morphology].push_back(org)
			*/
		};
	};
}

