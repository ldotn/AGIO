#include "Population.h"
#include "enumerate.h"
#include "Globals.h"
#include "../Core/Config.h"
#include <numeric>

using namespace std;
using namespace agio;
using namespace fpp;

void Population::Spawn(size_t Size)
{
	Individuals.resize(Size);
	
	for (auto [idx, org] : enumerate(Individuals))
	{
		org.Spawn(idx);
		SpeciesMap[org.GetMorphologyTag()].IndividualsIDs.push_back(idx);
	}

	NoveltyBuffer.resize(Size);
	NearestKBuffer.resize(Settings::NoveltyNearestK);
}

void Population::Epoch(class World * WorldPtr)
{
	// Compute fitness of each individual
	for (auto& org : Individuals)
		org.LastFitness = Interface->ComputeFitness(&org, WorldPtr);

	// Update the average fitness in the morphology registry
	for (auto & [tag,s] : SpeciesMap)
	{
		float avg_fitness = 0;
		for (auto & idx : s.IndividualsIDs)
			avg_fitness += Individuals[idx].LastFitness;

		avg_fitness /= s.IndividualsIDs.size();

		MorphologyRegistry[tag].AverageFitness = 0.5f * (MorphologyRegistry[tag].AverageFitness + avg_fitness);
	}

	// TODO :  Separate organisms by distance in the world too
	
	// Compute novelty metric
	for (auto & [idx,org] : enumerate(Individuals))
	{
		// Clear the K buffer
		for (auto & v : NearestKBuffer)
			v = numeric_limits<float>::max();

		// Check both against the population and the registry
		for (auto& [otherIdx,other] : enumerate(Individuals))
		{
			if (idx == otherIdx)
				continue;

			// This is the morphology distance
			float dist = org.GetMorphologyTag().DistanceTo(other.GetMorphologyTag());

			// Check if it's in the k nearest
			for (auto & [kidx,v] : enumerate(NearestKBuffer))
			{
				if (dist < v)
				{
					// Move all the values to the right
					for (int i = Settings::NoveltyNearestK - 1; i < kidx; i--)
						NearestKBuffer[i] = NearestKBuffer[i - 1];

					v = dist;
				}
			}
		}

		for (auto & [tag, record] : MorphologyRegistry)
		{
			if (org.GetGlobalID() == record.Representative.GetGlobalID())
				continue;

			// This is the morphology distance
			float dist = org.GetMorphologyTag().DistanceTo(tag);

			// Check if it's in the k nearest
			for (auto &[kidx, v] : enumerate(NearestKBuffer))
			{
				if (dist < v)
				{
					// Move all the values to the right
					for (int i = Settings::NoveltyNearestK - 1; i < kidx; i--)
						NearestKBuffer[i] = NearestKBuffer[i - 1];

					v = dist;
				}
			}
		}

		// After the k nearest are found, compute novelty metric
		// It's simply the average of the k nearest distances
		NoveltyBuffer[idx] = reduce(NearestKBuffer.begin(), NearestKBuffer.end()) / float(NearestKBuffer.size());
	}
}