#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "SIndividual.h"

namespace agio
{
	struct SComponent
	{
		int GroupID;
		int ComponentID;

		bool operator==(const ComponentRef & other) const
		{
			return GroupID == other.GroupID &&
				ComponentID == other.ComponentID;
		}
	};

	typedef std::vector<SComponent> SMorphologyTag;

	struct SRegistry
	{
		std::unordered_map<SMorphologyTag, vector<SIndividual>> Species;

		
		void LoadFromFile(const string& Path) 
		{ 
			/* TODO : Implement! 
				for record in StagnantSpecies
					org = SIndividual(record.HistoricalBestGenome)
					species[record.Morphology].push_back(org)
			*/
		};
	};
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

			for (auto[gid, cid] : k)
			{
				seed ^= gid + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= cid + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}

			return seed;
		}
	};
}