#include "Population.h"
#include "enumerate.h"
#include "Globals.h"
#include "../Core/Config.h"
#include <numeric>
#include <assert.h>
#include <unordered_set>

// NEAT
#include "innovation.h"

using namespace std;
using namespace agio;
using namespace fpp;

Population::Population() : RNG(std::chrono::high_resolution_clock::now().time_since_epoch().count())
{
	// TODO : Create a bigger population and only simulate a few on each step
	cur_node_id = 0;
	cur_innov_num = 0;
	CurrentGeneration = 0;
}

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

	// TODO :  Separate organisms by distance in the world too
	BuildSpeciesMap();
}

void Population::Epoch(void * WorldPtr, std::function<void(int)> EpochCallback)
{
	auto evaluate_pop = [&]()
	{
		// Compute novelty metric
		ComputeNovelty();

		for (int i = 0; i < 10; i++)
		{
			// Compute fitness of each individual
			Interface->ComputeFitness(this, WorldPtr);

			// Update the fitness and novelty accumulators
			for (auto& org : Individuals)
			{
				org.AccumulatedFitness += org.LastFitness;
				org.AccumulatedNovelty += org.LastNoveltyMetric;
				org.Age++;

				// Replace the last fitness and last novelty with the values of the accumulators
				// TODO : Maybe make some refactor here, all this mess doesn't really looks clean 
				org.LastFitness = org.AccumulatedFitness / org.Age;
				org.LastNoveltyMetric = org.AccumulatedNovelty / org.Age;
			}
		}
	};

	auto dominates = [](const Individual& A, const Individual& B)
	{
		return (A.LastNoveltyMetric >= B.LastNoveltyMetric && A.LastFitness >= B.LastFitness) &&
			(A.LastNoveltyMetric > B.LastNoveltyMetric || A.LastFitness > B.LastFitness);
	};

	auto compute_domination_fronts = [&]()
	{
		unordered_map<Individual::MorphologyTag, vector<unordered_set<int>>> fronts;

		for (auto &[tag, species] : SpeciesMap)
		{
			vector<unordered_set<int>> fronts_vec;

			unordered_set<int> current_front = {};
			for (int this_idx : species->IndividualsIDs)
			{
				auto& org = Individuals[this_idx];

				org.DominatedSet = {};
				org.DominationCounter = 0;
				for (int other_idx : species->IndividualsIDs)
				{
					if (other_idx == this_idx)
						continue;

					auto& other_org = Individuals[other_idx];
					if (dominates(org, other_org))
						org.DominatedSet.insert(other_idx);
					else if (dominates(other_org, org))
						org.DominationCounter++;
				}

				if (org.DominationCounter == 0)
				{
					org.DominationRank = 0;
					current_front.insert(this_idx);
				}
			}

			int i = 0;
			while (!current_front.empty())
			{
				fronts_vec.push_back(current_front);
				unordered_set<int> next_front = {};

				for (int this_idx : current_front)
				{
					auto& org = Individuals[this_idx];
					for (int other_idx : org.DominatedSet)
					{
						auto& other_org = Individuals[other_idx];
						other_org.DominationCounter--;
						if (other_org.DominationCounter == 0)
						{
							other_org.DominationRank = i + 1;
							next_front.insert(other_idx);
						}
					}
				}

				i++;
				current_front = move(next_front);
			}

			fronts[tag] = move(fronts_vec);
		}

		return fronts;
	};

	// Tournament selection (k = 2)
	auto tournament_select = [&](const vector<int>& Parents)
	{
        uniform_int_distribution<int> uid(0, Parents.size() - 1);
		int p0 = Parents[uid(RNG)];
		int p1 = Parents[uid(RNG)];
		while (p0 == p1)
			p1 = Parents[uid(RNG)];

		// Keep the one with the lower rank (less dominated)
		int r0 = Individuals[p0].DominationRank;
		int r1 = Individuals[p1].DominationRank;
		if (r0 < r1)
			return p0;
		else
		{
			if (r1 < r0)
				return p1;
			else
				return (uniform_int_distribution<int>(0, 1)(RNG) ? p0 : p1);
		}
	};

	// Reset innovations
	for (auto &[tag, species] : SpeciesMap)
	{
		for (auto innovation : species->innovations)
			delete innovation;
		species->innovations.resize(0);
	}

	if (CurrentGeneration == 0)
	{
		evaluate_pop();
		auto fronts_map = compute_domination_fronts();

		// Select parents based on the non dominated fronts
		// Not using the crowding distance because novelty already cares about diversity
		Children.resize(0);

		// TODO : For now, generating the same number of children as individuals on the species
		//   this should be changed, the species size should change based on fitness
		for (auto & [tag, species] : SpeciesMap)
		{
			if (species->IndividualsIDs.size() == 1)
			{
				// Only one individual on the species, so just clone it
				// TODO : Find if there's some better way to handle this
				const auto& individual = Individuals[species->IndividualsIDs[0]];
				Children.emplace_back(individual, Individual::Make::Clone);
				continue;
			}

			vector<int> parents;
			parents.reserve(species->IndividualsIDs.size());

			auto& front = fronts_map[tag];
			int i = 0;
			while (parents.size() < species->IndividualsIDs.size())
			{
				for (int idx : front[i])
				{
					if (parents.size() == species->IndividualsIDs.size())
						break;

					parents.push_back(idx);
				}

				i++;
			}

			for (int n = 0; n < species->IndividualsIDs.size(); n++)
			{
				int mom_idx = tournament_select(parents);
				int dad_idx = tournament_select(parents);
				while (mom_idx == dad_idx)
					dad_idx = tournament_select(parents); // TODO : I think this gets stuck if you have only 2 individuals where one is dominated by the other

				Children.emplace_back(Individuals[mom_idx], Individuals[dad_idx], Children.size());
			}
		}

		// Mutate children
		for (auto& child : Children)
			if (uniform_real_distribution<float>()(RNG) <= Settings::ChildMutationProb)
				child.Mutate(this, CurrentGeneration);
	}
	else
	{
		vector<Individual> old_pop = move(Individuals);
		size_t children_size = Children.size();
		Individuals = move(Children);

		// Generate species map for the children
		BuildSpeciesMap();

		// Evaluate
		evaluate_pop();

		for (auto &[tag, species] : SpeciesMap)
		{
			if (species->IndividualsIDs.size() == 1)
			{
				// Only one individual on the species, so just clone it
				// TODO : Find if there's some better way to handle this
				const auto& individual = Individuals[species->IndividualsIDs[0]];
				Children.emplace_back(individual, Individual::Make::Clone);
				continue;
			}
		}

		// Make a copy of the species map
		decltype(SpeciesMap) next_pop_species_map;
		for (auto &[tag, species] : SpeciesMap)
		{
			auto new_ptr = new Species;
			new_ptr->IndividualsIDs = species->IndividualsIDs;
			next_pop_species_map[tag] = new_ptr;

			for (int idx : species->IndividualsIDs)
				Individuals[idx].SpeciesPtr = new_ptr;
		}

		// Merge populations and generate a species map for both
		for (auto& org : old_pop)
			Individuals.push_back(move(org));
		BuildSpeciesMap();

		// Compute fronts
		auto fronts_map = compute_domination_fronts();

		// TODO : For now, generating the same number of children as individuals on the species
		//   this should be changed, the species size should change based on fitness
		for (auto &[tag, species] : next_pop_species_map)
		{
			if (species->IndividualsIDs.size() == 1)
				continue;

			auto& front = fronts_map[tag];

			vector<int> parents;
			parents.reserve(species->IndividualsIDs.size());

			int i = 0;
			while (parents.size() < species->IndividualsIDs.size())
			{
				for (int idx : front[i])
				{
					if (parents.size() == species->IndividualsIDs.size())
						break;

					parents.push_back(idx);
				}

				i++;
			}

			for (int n = 0; n < species->IndividualsIDs.size(); n++)
			{
				int mom_idx = tournament_select(parents);
				int dad_idx = tournament_select(parents);
				while (mom_idx == dad_idx)
					dad_idx = tournament_select(parents); // TODO : I think this gets stuck if you have only 2 individuals where one is dominated by the other

				Children.emplace_back(Individuals[mom_idx], Individuals[dad_idx], Children.size());
			}
		}

		// Mutate children
		for (auto& child : Children)
			if (uniform_real_distribution<float>()(RNG) <= Settings::ChildMutationProb)
				child.Mutate(this, CurrentGeneration);

		// Restore species
		for (auto &[_, s] : SpeciesMap)
		{
			for (auto innovation : s->innovations)
				delete innovation;
			delete s;
		}
		SpeciesMap.clear();
		SpeciesMap = next_pop_species_map;

		// Remove the old pop from the individuals
		// Children are first
		vector<Individual> temp_vec;
		for (int i = 0;i < children_size;i++)
			temp_vec.push_back(move(Individuals[i]));
		Individuals = move(temp_vec);

		ComputeNovelty();
	}

	EpochCallback(CurrentGeneration);
	CurrentGeneration++;
}

void Population::ComputeNovelty()
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
					for (int i = Settings::NoveltyNearestK - 1; i > kidx; i--)
						NearestKBuffer[i] = NearestKBuffer[i - 1];

					if (kidx > k_buffer_top)
						k_buffer_top = kidx;
					v = dist;
					break;
				}
			}
		}

		for (auto & tag : MorphologyRegistry)
		{
			// This is the morphology distance
			float dist = org.GetMorphologyTag().Distance(tag);

			// Check if it's in the k nearest
			for (auto[kidx, v] : enumerate(NearestKBuffer))
			{
				if (dist < v)
				{
					// Move all the values to the right
					for (int i = Settings::NoveltyNearestK - 1; i > kidx; i--)
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

		// If the novelty is above the threshold, add it to the registry if it doesn't exist already
		if (novelty > Settings::NoveltyThreshold)
		{
			const auto& tag = org.GetMorphologyTag();
			
			// Using a vector instead of a set because iterations are much more common than insertions
			if (find(MorphologyRegistry.begin(), MorphologyRegistry.end(), tag) == MorphologyRegistry.end())
				MorphologyRegistry.push_back(tag);
		}
	}
}

void Population::BuildFinalPopulation()
{
	vector<Individual> final_pop;
	for (const auto& [_, individuals] : NonDominatedRegistry)
		for (const auto& org : individuals)
			final_pop.emplace_back(org, Individual::Make::Clone);

	Individuals = move(final_pop);
	BuildSpeciesMap();
}

Population::ProgressMetrics Population::ComputeProgressMetrics(void * World,int Replications)
{
	/*
	// Save the current population
	vector<Individual> current_pop = move(Individuals);

	// Now build the population from the registry
	BuildFinalPopulation();*/

	ProgressMetrics metrics{};

	// Compute average novelty
	ComputeNovelty();
	metrics.AverageNovelty = 0;
	for (const auto& org : Individuals)
		metrics.AverageNovelty += org.LastNoveltyMetric;
	metrics.AverageNovelty /= Individuals.size();

	// Do a few simulations to compute fitness
	for (auto& org : Individuals)
		org.AccumulatedFitness = org.Age = 0;
	for (int i = 0; i < Replications; i++)
	{
		Interface->ComputeFitness(this, World);

		for (auto& org : Individuals)
		{
			org.AccumulatedFitness += org.LastFitness;
			org.Age++;
		}
	}

	// Now for each species set that to be random, and do a couple of simulations
	metrics.AverageFitnessDifference = 0;
	for (const auto& [_, species] : SpeciesMap)
	{
		// First compute average fitness of the species when using the network
		float avg_f = 0;
		for (int org_id : species->IndividualsIDs)
			avg_f += Individuals[org_id].AccumulatedFitness
			/ Individuals[org_id].Age;
		avg_f /= species->IndividualsIDs.size();

		// Override the use of the network for decision-making
		for (int org_id : species->IndividualsIDs)
			Individuals[org_id].UseNetwork = false;

		// Do a few simulations
		float avg_random_f = 0;
		float count = 0;
		for (int i = 0; i < Replications; i++)
		{
			Interface->ComputeFitness(this, World);

			for (int org_id : species->IndividualsIDs)
			{
				avg_random_f += Individuals[org_id].LastFitness;
				count++;
			}
		}
		avg_random_f /= count;

		// Accumulate difference
		metrics.AverageFitnessDifference += avg_f - avg_random_f;

		// Re-enable the network
		for (int org_id : species->IndividualsIDs)
			Individuals[org_id].UseNetwork = true;
	}
	metrics.AverageFitnessDifference /= SpeciesMap.size();

	// TODO : Compute standard deviations

	// After all is done, restore the current population
	//Individuals = move(current_pop);

	return metrics;
}
