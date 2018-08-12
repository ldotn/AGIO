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
    // clear the species map
	for(auto & [_, s] : SpeciesMap)
    {
        for (auto innovation : s->innovations)
            delete innovation;
		delete s;
    }
    SpeciesMap.clear();

	// put every individual into his corresponding species
	for (auto [idx, org] : enumerate(Individuals))
	{
		auto tag = org.GetMorphologyTag(); // make a copy
		tag.Parameters = {}; // remove parameters


		Species* species_ptr = nullptr;
		auto iter = SpeciesMap.find(tag);
		if (iter == SpeciesMap.end())
		{
			species_ptr = new Species;
			SpeciesMap[tag] = species_ptr;
		}
		else
			species_ptr = iter->second;

		species_ptr->IndividualsIDs.push_back(idx);
		org.SpeciesPtr = species_ptr;
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
		for (auto & idx : s->IndividualsIDs)
			avg_fitness += Individuals[idx].LastFitness;

		avg_fitness /= s->IndividualsIDs.size();

		MorphologyRegistry[tag].AverageFitness = 0.5f * (MorphologyRegistry[tag].AverageFitness + avg_fitness);
	}

	// TODO :  Separate organisms by distance in the world too
	
	// Compute novelty metric
	auto compute_novelty = [&,this]()
	{
		for (auto[idx, org] : enumerate(Individuals))
		{
			// Clear the K buffer
			for (auto & v : NearestKBuffer)
				v = numeric_limits<float>::max();
			int k_buffer_top = 0;

			// Check both against the population and the registry
			for (auto[otherIdx, other] : enumerate(Individuals))
			{
				if (idx == otherIdx)
					continue;

				// This is the morphology distance
				float dist = org.GetMorphologyTag().Distance(other.GetMorphologyTag());

				// Check if it's in the k nearest
				for (auto[kidx, v] : enumerate(NearestKBuffer))
				{
					if (dist < v)
					{
						// Move all the values to the right
						for (int i = Settings::NoveltyNearestK - 1; i < kidx; i--)
							NearestKBuffer[i] = NearestKBuffer[i - 1];

						if (kidx > k_buffer_top)
							k_buffer_top = kidx;
						v = dist;
						break;
					}
				}
			}

			for (auto &[tag, record] : MorphologyRegistry)
			{
				// This is the morphology distance
				float dist = org.GetMorphologyTag().Distance(tag);

				// Check if it's in the k nearest
				for (auto[kidx, v] : enumerate(NearestKBuffer))
				{
					if (dist < v)
					{
						// Move all the values to the right
						for (int i = Settings::NoveltyNearestK - 1; i < kidx; i--)
							NearestKBuffer[i] = NearestKBuffer[i - 1];

						if (kidx > k_buffer_top)
							k_buffer_top = kidx;
						v = dist;
						break;
					}
				}
			}

			// After the k nearest are found, compute novelty metric
			// It's simply the average of the k nearest distances
			float novelty = accumulate(NearestKBuffer.begin(), NearestKBuffer.begin() + k_buffer_top + 1, 0) / float(NearestKBuffer.size());
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
					mr.RepresentativeFitness = org.LastFitness;

					MorphologyRegistry[tag] = mr;
				}
				else
				{
					// The tag is already on the registry
					// Check if this individual is better than the representative
					// If it is, replace the individual and the tag
					if (org.LastFitness > record->second.RepresentativeFitness)
					{
						// This two tags compare equal and have the same hash
						//  but may have different parameter's values
						// Doing this to always keep the parameters of the representative
						auto new_iter = MorphologyRegistry.extract(tag);
						new_iter.key() = tag;
						MorphologyRegistry.insert(move(new_iter));

						auto & record = MorphologyRegistry[tag];
						record.AverageFitness = 0.5f*(record.AverageFitness + org.LastFitness);
						record.RepresentativeFitness = org.LastFitness;
					}
				}
			}
		}
	};
	compute_novelty();

	// Evolve the species
	// TODO: This part would need to be reworked if you want to share individuals between concurrent (independent) executions
	assert(DominationBuffer.capacity() >= Individuals.size());
	ChildrenBuffer.resize(0); // does not affect capacity

	for (auto & [_, species] : SpeciesMap)
	{
		// Mate within species based on NSGA-II
		// As a first, simple implementation, sort based on the number of individuals that dominate you (more is worst)
		// TODO : Properly implement NSGA-II and check if it's better
		assert(species->IndividualsIDs.size() > 0);

		DominationBuffer.resize(0);
		for (int this_idx : species->IndividualsIDs)
		{
			auto& org = Individuals[this_idx];

			// Check if this individual is on the non-dominated front
			org.LastDominationCount = 0;
			for (int other_idx : species->IndividualsIDs)
			{
				if (this_idx == other_idx)
					continue;

				const auto& other_org = Individuals[other_idx];


				if ((other_org.LastNoveltyMetric > org.LastNoveltyMetric && other_org.LastFitness > org.LastFitness) ||
					(other_org.LastFitness == org.LastFitness && other_org.LastNoveltyMetric > org.LastNoveltyMetric) ||
					(other_org.LastNoveltyMetric == org.LastNoveltyMetric && other_org.LastFitness > org.LastFitness) )
				{
					org.LastDominationCount++;
				}
			}

			// Transform from a count to a selection weight (not a probability because they aren't normalized)
			// The transform is simply f(x) = 1 / (x + 1)
			DominationBuffer.push_back(1.0f / (org.LastDominationCount + 1.0f));
		}

		// Now use the domination weights to randomly select parents and cross them
		discrete_distribution<int> domination_dist(DominationBuffer.begin(), DominationBuffer.end());

		//assert(species.IndividualsIDs.size() > 1); // TODO : Find what to do here!
		assert(species->IndividualsIDs.size() > 0);
		if (species->IndividualsIDs.size() == 1)
		{
			// Only one individual on the species, so just clone it
			// TODO : Find if there's some better way to handle this
			//ChildrenBuffer.push_back(Individuals[species->IndividualsIDs[0]]);
			
			// Individuals have copy disabled as a way to ensure proper usage
			// Because of that, the way to clone an individual is to mate it with itself
			const auto& individual = Individuals[species->IndividualsIDs[0]];
			ChildrenBuffer.emplace_back(individual,individual, ChildrenBuffer.size());
		}
		else
		{
			for (int i = 0; i < species->IndividualsIDs.size(); i++)
			{
				int mom_idx = domination_dist(RNG);
				int dad_idx = domination_dist(RNG);

				while (dad_idx == mom_idx) // Don't want someone to mate with itself
					dad_idx = domination_dist(RNG);

				const auto & mom = Individuals[species->IndividualsIDs[mom_idx]];
				const auto & dad = Individuals[species->IndividualsIDs[dad_idx]];

				ChildrenBuffer.emplace_back(mom,dad,ChildrenBuffer.size());
			}
		}

	}

	EpochCallback(CurrentGeneration);

	// Make replacement
	assert(ChildrenBuffer.size() == Individuals.size());

	// Make a temporal copy of the individuals
	// TODO : Find a way to avoid all this! I hate it!
	vector<Individual> parents = move(Individuals);
	decltype(SpeciesMap) parents_species;
	for (auto [tag, species] : SpeciesMap)
	{
		auto ptr = new Species;
		ptr->IndividualsIDs = species->IndividualsIDs;
		parents_species[tag] = ptr;
	}

	// Make a preliminary replacement
	Individuals = move(ChildrenBuffer);

	// Do a call first so that the childs know on what species they are 
	// TODO : I worked really hard to avoid memory allocs, and this part does a BUNCH of them
	//	try to find a way to avoid them
	BuildSpeciesMap();

    // Mutate children
    for (auto& child : Individuals)
        if(uniform_real_distribution<float>()(RNG) <= Settings::ChildMutationProb)
            child.Mutate(this, CurrentGeneration);

	// After mutation, the species might have changed, so this needs to be rebuilt
	// TODO : Find a way to avoid the double call to this
	BuildSpeciesMap();

	// Now compute fitness for the childs
	Interface->ComputeFitness(this, WorldPtr);
	compute_novelty();

	// Compute domination count for the childs
	for (auto & [_, species] : SpeciesMap)
	{
		for (int this_idx : species->IndividualsIDs)
		{
			auto& org = Individuals[this_idx];

			// Check if this individual is on the non-dominated front
			org.LastDominationCount = 0;
			for (int other_idx : species->IndividualsIDs)
			{
				if (this_idx == other_idx)
					continue;

				const auto& other_org = Individuals[other_idx];

				if ((other_org.LastNoveltyMetric > org.LastNoveltyMetric && other_org.LastFitness > org.LastFitness) ||
					(other_org.LastFitness == org.LastFitness && other_org.LastNoveltyMetric > org.LastNoveltyMetric) ||
					(other_org.LastNoveltyMetric == org.LastNoveltyMetric && other_org.LastFitness > org.LastFitness))
				{
					org.LastDominationCount++;
				}
			}
		}
	}

	// Now select final population by mixing from parents and childrens
	// TODO : Try different options
	// TODO : Refactor this so that's not this ugly
	// First find matching species and take the best individuals there
	vector<Individual> final_population;
	for (auto & [tag, species] : SpeciesMap)
	{
		if (final_population.size() == Individuals.size())
			break;

		auto iter = parents_species.find(tag);
		if (iter != parents_species.end())
		{
			// Matching species, so select based on domination count
			vector<pair<bool, int>> individuals_idxs(iter->second->IndividualsIDs.size()+species->IndividualsIDs.size()); // indexes of the individuals, and a bool that says if they are a child
			vector<float> selection_weights(iter->second->IndividualsIDs.size() + species->IndividualsIDs.size());

			for (const auto& [idx, parent_idx] : enumerate(iter->second->IndividualsIDs))
			{
				individuals_idxs[idx] = { false, parent_idx };
				selection_weights[idx] = 1.0f / (parents[parent_idx].LastDominationCount + 1.0f);
			}
				
			for (const auto&[idx, child_idx] : enumerate(species->IndividualsIDs))
			{
				individuals_idxs[iter->second->IndividualsIDs.size() + idx] = { true, child_idx };
				selection_weights[iter->second->IndividualsIDs.size() + idx] = 1.0f / (Individuals[child_idx].LastDominationCount + 1.0f);
			}
			
			// Make selection
			for (int i = 0; i < max(species->IndividualsIDs.size(), iter->second->IndividualsIDs.size()); i++)
			{
				// Randomly select an individual
				Individual * ptr;
				do
				{
					auto [is_child, selected_idx] = individuals_idxs[discrete_distribution<int>(selection_weights.begin(), selection_weights.end())(RNG)];
					
					if (is_child)
						ptr = &Individuals[selected_idx];
					else
						ptr = &parents[selected_idx];
				} while (ptr->GetGenome() == nullptr); // I could select an already moved individual, that is invalid

				final_population.push_back(move(*ptr));

				if (final_population.size() == Individuals.size())
					break;
			}
		}
	}

	// If the population is still under the pop size, randomly select
	while (final_population.size() < Individuals.size())
	{
		if (uniform_int_distribution<int>()(RNG))
		{
			Individual * ptr;
			do
			{
				ptr = &Individuals[uniform_int_distribution<int>(0, Individuals.size())(RNG)];
			} while (ptr->GetGenome() == nullptr); // I could select an already moved individual, that is invalid

			final_population.push_back(move(*ptr));
		}
		else
		{
			Individual * ptr;
			do
			{
				ptr = &parents[uniform_int_distribution<int>(0, parents.size())(RNG)];
			} while (ptr->GetGenome() == nullptr); // I could select an already moved individual, that is invalid

			final_population.push_back(move(*ptr));
		}
			
	}

	// After the final population vector is done, make the swap
	Individuals = move(final_population);

	// Rebuild species map one last time
	BuildSpeciesMap();

	// Delete temporal species copy
	for (auto [_, ptr] : parents_species)
		delete ptr;

	// Finally increase generation number
	CurrentGeneration++;
}

