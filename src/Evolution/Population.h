#pragma once
#include "Individual.h"
#include <unordered_map>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace NEAT
{
	class Innovation;
	class Population;
	class Genome;
}

namespace agio
{
	// Forward declaration
	class SPopulation;

	// Encapsulates the data for a single species
	struct Species
	{
		Species() = default;

		std::vector<int> IndividualsIDs;

		// Each species has a NEAT population that represents the brains
		NEAT::Population * NetworksPopulation;

		// Used to track progress
		float LastFitness = 0; // moving average
		float ProgressMetric = 0; // Moving average difference of fitness with last
		
		int Age = 0;

		// The species are checked for stagnancy on each epoch
		// Each species has N "chances"
		// If after N consecutive epochs the progress is below the threshold, it's considered stagnant
		int EpochsUnderThreshold;

		struct 
		{
			float RandomFitness;
			float RealFitness;
		} DevMetrics;
	};

	// Used to register the species that got stuck and where removed from the simulation
	// The same species might be more than one time, because it may have become stagnant, got removed, and then later on created again
	// That does make sense, because as the species change, some that was stuck might not be stuck anymore
	struct SpeciesRecord
	{
		MorphologyTag Morphology;
		NEAT::Genome * HistoricalBestGenome;
		std::vector<NEAT::Genome*> LastGenomes;
		int IndividualsSize;
		int Age;
		float LastFitness;
	};

	class Population
	{
	public:
		// TODO : Docs
		Population();

		// Creates a new population of random individuals
		// It also separates them into species
		// The size of the population is equal to SimulationSize*PopulationSizeMultiplier
		// The simulation size is the number of individuals that are simulated at the same time
		void Spawn(int PopulationSizeMultiplier, int SimulationSize);

		// Computes a single evolutive step
		// The callback is called right after evaluation
		// The MuteNEAT parameter disables console output from neat
		void Epoch(void * World, std::function<void(int)> EpochCallback = [](int){}, bool MuteNEAT = false);

		const auto& GetIndividuals() const { return Individuals; }
		auto& GetIndividuals() { return Individuals; }
		const auto& GetSpecies() const { return SpeciesMap; }
		const auto& GetSpeciesRegistry() const { return StagnantSpecies; }
		// Returns several metrics that allow one to measure the progress of the evolution
		// TODO : More comprehensive docs maybe?
		struct ProgressMetrics
		{
			ProgressMetrics() { memset(this, 0, sizeof(ProgressMetrics)); }
			float AverageNovelty;
			float NoveltyStandardDeviation;
			float ProgressMetric;
			float AverageFitness;
			float AverageRandomFitness;
			float FitnessDifferenceStandardDeviation;
			float MaxFitnessDifference;
			float MinFitnessDifference;
		};
		//ProgressMetrics ComputeProgressMetrics(void * World);
		void ComputeDevMetrics(void * World);

		// Variables used in mutate_add_node and mutate_add_link (neat)
		// TODO : Refactor this so that the naming is consistent
		int cur_node_id;
		double cur_innov_num;

		// Evaluates the population
		void EvaluatePopulation(void * WorldPtr);
		void CurrentSpeciesToRegistry();
	private:
		friend SPopulation;
		int CurrentGeneration;
		std::vector<Individual> Individuals;

		std::minstd_rand RNG;

		// Map from the morphology tag (that's what separates species) to the species
		std::unordered_map<MorphologyTag, Species> SpeciesMap;

		// Used to keep track of the different morphologies that were tried
		std::vector<MorphologyTag> MorphologyRegistry;

		std::vector<SpeciesRecord> StagnantSpecies;

		// Number of individuals simulated at the same time. 
		// The entire population is simulated, but on batches of SimulationSize
		int SimulationSize;

		// Creates a random morphology
		MorphologyTag MakeRandomMorphology();
	};
}