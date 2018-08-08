#include "Population.h"
#include "enumerate.h"
#include "Globals.h"
#include "../Core/Config.h"
#include <numeric>
#include <assert.h>

using namespace std;
using namespace agio;
using namespace fpp;

void Population::BuildSpeciesMap()
{
	SpeciesMap.clear();

	for (auto [idx, org] : enumerate(Individuals))
	{
		auto tag = org.GetMorphologyTag(); // make a copy
		tag.Parameters = {}; // remove parameters
		SpeciesMap[tag].IndividualsIDs.push_back(idx);
	}
}


void Population::Spawn(size_t Size)
{
	Individuals.resize(Size);
	
	for (auto [idx, org] : enumerate(Individuals))
		org.Spawn(idx);

	DominationBuffer.resize(Size);
	NearestKBuffer.resize(Settings::NoveltyNearestK);
	ChildrenBuffer.resize(Size);
	CurrentGeneration = 0;

	BuildSpeciesMap();
}

void Population::Epoch(void * WorldPtr, std::function<void(int)> EpochCallback)
{
	// Compute fitness of each individual
	/*for (auto& org : Individuals)
		org.Reset();

	// TODO : Create an evaluation list and shuffle it. This way the evaluation order changes. Can't shuffle the individuals list directly because that would break the IDs
	for (auto& org : Individuals)
		org.LastFitness = Interface->ComputeFitness(&org, WorldPtr);*/
	Interface->ComputeFitness(this, WorldPtr);

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
	for (auto [idx,org] : enumerate(Individuals))
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
			float dist = org.GetMorphologyTag().Distance(other.GetMorphologyTag());

			// Check if it's in the k nearest
			for (auto [kidx,v] : enumerate(NearestKBuffer))
			{
				if (dist < v)
				{
					// Move all the values to the right
					for (int i = Settings::NoveltyNearestK - 1; i < kidx; i--)
						NearestKBuffer[i] = NearestKBuffer[i - 1];

					v = dist;
					break;
				}
			}
		}

		for (auto & [tag, record] : MorphologyRegistry)
		{
			if (org.GetGlobalID() == record.Representative.GetGlobalID())
				continue;

			// This is the morphology distance
			float dist = org.GetMorphologyTag().Distance(tag);

			// Check if it's in the k nearest
			for (auto [kidx, v] : enumerate(NearestKBuffer))
			{
				if (dist < v)
				{
					// Move all the values to the right
					for (int i = Settings::NoveltyNearestK - 1; i < kidx; i--)
						NearestKBuffer[i] = NearestKBuffer[i - 1];

					v = dist;
					break;
				}
			}
		}

		// After the k nearest are found, compute novelty metric
		// It's simply the average of the k nearest distances
		float novelty = reduce(NearestKBuffer.begin(), NearestKBuffer.end()) / float(NearestKBuffer.size());
		org.LastNoveltyMetric = novelty;
	
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

	// Evolve the species
	// TODO: This part would need to be reworked if you want to share individuals between concurrent (independent) executions
	assert(DominationBuffer.capacity() >= Individuals.size());
	ChildrenBuffer.resize(0); // does not affect capacity

	for (auto & [_, species] : SpeciesMap)
	{
		// Mate within species based on NSGA-II
		// As a first, simple implementation, sort based on the number of individuals that dominate you (more is worst)
		// TODO : Properly implement NSGA-II and check if it's better
		assert(species.IndividualsIDs.size() > 0);

		DominationBuffer.resize(0);
		for (int this_idx : species.IndividualsIDs)
		{
			const auto& org = Individuals[this_idx];

			// Check if this individual is on the non-dominated front
			float domination_count = 0;
			for (int other_idx : species.IndividualsIDs)
			{
				if (this_idx == other_idx)
					continue;

				const auto& other_org = Individuals[other_idx];

				if (org.LastNoveltyMetric > org.LastNoveltyMetric &&
					other_org.LastFitness > org.LastFitness)
				{
					domination_count++;
				}
			}

			// Transform from a count to a selection weight (not a probability because they aren't normalized)
			// The transform is simply f(x) = 1 / (x + 1)
			DominationBuffer.push_back(1.0f / (domination_count + 1.0f));
		}

		// Now use the domination weights to randomly select parents and cross them
		discrete_distribution<int> domination_dist(DominationBuffer.begin(), DominationBuffer.end());

		assert(species.IndividualsIDs.size() > 1); // TODO : Find what to do here!

		for (int i = 0; i < species.IndividualsIDs.size(); i++)
		{
			int mom_idx = domination_dist(RNG);
			int dad_idx = domination_dist(RNG);

			// TODO : JUST TESTING! This should be enabled
			while (dad_idx == mom_idx) // Don't want someone to mate with itself
				dad_idx = domination_dist(RNG);

			auto& mom = Individuals[species.IndividualsIDs[mom_idx]];
			auto& dad = Individuals[species.IndividualsIDs[dad_idx]];

			ChildrenBuffer.push_back(mom.Mate(dad, ChildrenBuffer.size()));
		}
	}

	// Mutate children
	for (auto& child : ChildrenBuffer)
		if(uniform_real_distribution<float>()(RNG) <= Settings::ChildMutationProb)
			child.Mutate();

	EpochCallback(CurrentGeneration);

	// Make replacement
	// TODO : Test different options
	// Replace all the population by the childs
	assert(ChildrenBuffer.size() == Individuals.size());
	Individuals = ChildrenBuffer;

	// Separate species
	// TODO : I worked really hard to avoid memory allocs, and this part does a BUNCH of them
	//	try to find a way to avoid them
	BuildSpeciesMap();

	// Finally increase generation number
	CurrentGeneration++;
}

