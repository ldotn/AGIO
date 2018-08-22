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
		void Spawn(size_t Size,int SimulationSize);

		// Replaces the individuals with the ones from the non dominated registry
		void BuildFinalPopulation();

		// Computes a single evolutive step
		// The callback is called just before replacement
		void Epoch(void * World, std::function<void(int)> EpochCallback = [](int){});

		const auto& GetIndividuals() const { return Individuals; }
		auto& GetIndividuals() { return Individuals; }
		const auto& GetSpecies() const { return SpeciesMap; }
		const auto& GetNonDominatedRegistry() const { return NonDominatedRegistry; }
		const auto& GetSimulationIndividuals() const { return SimulationIndividuals; };
		auto& GetSimulationIndividuals() { return SimulationIndividuals; };

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
		// Number of individuals that are simulated in a single simulation step
		int SimulationSize;

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
		std::vector<float> NoveltyNearestKBuffer;
		std::vector<std::pair<int,float>> CompetitionNearestKBuffer;
		std::vector<float> DominationBuffer;
		std::vector<Individual> ChildrenBuffer;

		// Separates the individuals in species based on the actions and sensors
		void BuildSpeciesMap();

		// Historical record of non-dominated individuals found
		// Separated by species
		// TODO : If the world is dynamic the old fitness values might no longer be valid or relevant
		// TODO :  Remove this, it's no longer used!
		std::unordered_map<Individual::MorphologyTag, std::vector<Individual>> NonDominatedRegistry;
	
		// Computes the novelty metric for the population
		void ComputeNovelty();

		// Simulate the population and compute scores
		void EvaluatePopulation(void * WorldPtr);

		// Computes local scores from fitness
		void ComputeLocalScores();

		// Children of the current population. See NSGA-II
		std::vector<Individual> Children;

		// Individuals to be simulated
		std::vector<Individual> SimulationIndividuals;
		
		// Table that maps IDs from the individuals selected for the simulation to IDs in the whole population
		std::vector<int> SimulationIDTable;

		// Map the goes from population ID to simulation ID
		std::unordered_map<int, int> PopulationToSimulationMap;

		// Fills the simulation individuals vector and restores them to the general population, respectively
		void SelectIndividualsForSimulation();
		void RestoreSimulatedIndividuals();
	};
}