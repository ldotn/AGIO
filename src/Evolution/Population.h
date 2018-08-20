#pragma once
#include "Individual.h"
#include <unordered_map>
#include <chrono>
#include <functional>
#include <memory>

// Forward declaration
namespace NEAT
{
	class Innovation;
}

namespace agio
{
	// Encapsulates the data for a single species
	struct Species
	{
		std::vector<int> IndividualsIDs;
		// TODO : Maybe refactor this, don't like the idea of using pointers like this when it's resources that only last an epoch
		std::vector<NEAT::Innovation *> innovations;
	};

	class Population
	{
	public:
		// TODO : Docs
		Population();

		// Creates a new population of random individuals
		// It also separates them into species
		void Spawn(size_t Size);

		// Replaces the individuals with the ones from the non dominated registry
		void BuildFinalPopulation();

		// Computes a single evolutive step
		// The callback is called just before replacement
		void Epoch(void * World, std::function<void(int)> EpochCallback = [](int){});

		const auto& GetIndividuals() const { return Individuals; }
		auto& GetIndividuals() { return Individuals; }
		const auto& GetSpecies() const { return SpeciesMap; }
		const auto& GetNonDominatedRegistry() const { return NonDominatedRegistry; }

		// Returns several metrics that allow one to measure the progress of the evolution
		// TODO : More comprehensive docs maybe?
		struct ProgressMetrics
		{
			float AverageNovelty;
			float NoveltyStandardDeviation;
			float AverageFitnessDifference;
			float FitnessDifferenceStandardDeviation;
		};
		ProgressMetrics ComputeProgressMetrics(void * World,int Replications);

		// Variables used in mutate_add_node and mutate_add_link (neat)
		// TODO : Refactor this so that the naming is consistent
		int cur_node_id;
		double cur_innov_num;
	private:
		int CurrentGeneration;
		std::vector<Individual> Individuals;
		std::minstd_rand RNG;

		// Map from the morphology tag (that's what separates species) to the species
		// IMPORTANT! : The parameters are REMOVED before creating the map
		//	The tag cares about the parameters, but species are only separated by actions and sensors
		std::unordered_map<Individual::MorphologyTag, Species*> SpeciesMap;

		// Used to keep track of the different morphologies that were tried
		std::vector<Individual::MorphologyTag> MorphologyRegistry;
	
		// Buffer vectors
		std::vector<float> NearestKBuffer;
		std::vector<float> DominationBuffer;
		std::vector<Individual> ChildrenBuffer;

		// Separates the individuals in species based on the actions and sensors
		void BuildSpeciesMap();

		// Historical record of non-dominated individuals found
		// Separated by species
		// TODO : If the world is dynamic the old fitness values might no longer be valid or relevant
		std::unordered_map<Individual::MorphologyTag, std::vector<Individual>> NonDominatedRegistry;
	
		// Computes the novelty metric for the population
		void ComputeNovelty();





		// Children of the current population. See NSGA-II
		std::vector<Individual> Children;
	};
}