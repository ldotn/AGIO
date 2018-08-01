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
	CurrentGeneration = 0;
}

void Population::Epoch(class World * WorldPtr)
{
	// Compute fitness of each individual
	for (auto& org : Individuals)
		org.Reset();

	// TODO : Create an evaluation list and shuffle it. This way the evaluation order changes. Can't shuffle the individuals list directly because that would break the IDs
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
			for (auto [kidx, v] : enumerate(NearestKBuffer))
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
		float novelty = reduce(NearestKBuffer.begin(), NearestKBuffer.end()) / float(NearestKBuffer.size());
		NoveltyBuffer[idx] = novelty;
	
		// If the novelty is above the threshold, add it to the registry
		if (novelty > Settings::NoveltyThreshold)
		{
			// First check if it already exists
			const auto& tag = org.GetMorphologyTag();
			auto record = MorphologyRegistry.find(tag);

			if (record == MorphologyRegistry.end())
			{
				MorphologyRecord mr;
				mr.AverageFitness = org.LastFitness;
				mr.GenerationNumber = CurrentGeneration;
				mr.Representative = org;

				MorphologyRegistry[tag] = mr;
			}
			else
			{
				// The tag is already on the registry
				// Check if this individual is better than the representative
				// If it is, replace the individual and the tag
				if (org.LastFitness > record->second.Representative.LastFitness)
				{
					// This two tags compare equal and have the same hash
					//  but may have different parameter's values
					// Doing this to always keep the parameters of the representative
					auto new_iter = MorphologyRegistry.extract(tag);
					new_iter.key() = tag;
					MorphologyRegistry.insert(move(new_iter));

					auto & record = MorphologyRegistry[tag];
					record.AverageFitness = 0.5f*(record.AverageFitness + org.LastFitness);
					record.Representative = org;
				}
			}
		}
	}

	// Mate within species based on NSGA-II

	// Mutate children

	// Make replacement

	// Finally increase generation number
	CurrentGeneration++;
}