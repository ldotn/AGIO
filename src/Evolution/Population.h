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
		std::vector<NEAT::Innovation *> innovations;
	};

	class Population
	{
	public:
		// TODO : Docs
		Population() : RNG(std::chrono::high_resolution_clock::now().time_since_epoch().count()) {}

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

		// Variables used in mutate_add_node and mutate_add_link (neat)
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
		// It's only considering the actions, sensors and parameters, not the whole components and parameters
		// Information is lost that way, but shouldn't be necessary
		//  and that should help to reduce dimensionality anyway
		// This is Novelty Search basically
		// That tag is the one of the representative individual
		//	which is relevant because of the parameters
	public:
		struct MorphologyRecord
		{
			// TODO : GenerationNumber and AverageFitness seem useless, check that

			// The generation at which this morphology first appeared
			int GenerationNumber;

			// The average fitness across organisms and generations
			// It's a moving average across generations
			float AverageFitness;
			
			// The fitness of the individual that represents this morphology
			// Used to keep the tag of the best one
			float RepresentativeFitness;
		};
	private:
		std::unordered_map<Individual::MorphologyTag, MorphologyRecord> MorphologyRegistry;
	
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
	};
}