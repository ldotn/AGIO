#pragma once
#include "Individual.h"
#include <unordered_map>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include "../Utils/WorkerPool.h"
#include "../Core/Config.h"

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
		std::unique_ptr<NEAT::Population> NetworksPopulation;

		// Used to track progress
		float CurrentEpochMeanFitness = 0;
		float SmoothedFitness = 0; // moving average
		float ProgressMetric = 0; // Moving average difference of fitness with last
		
		int Age = 0;

		// The species are checked for stagnancy on each epoch
		// Each species has N "chances"
		// If after N consecutive epochs the progress is below the threshold, it's considered stagnant
		int EpochsUnderThreshold;
	};

	// Used to register the species that got stuck and where removed from the simulation
	// The same species might be more than one time, because it may have become stagnant, got removed, and then later on created again
	// That does make sense, because as the species change, some that was stuck might not be stuck anymore
	struct SpeciesRecord
	{
		MorphologyTag Morphology;
		NEAT::Genome * HistoricalBestGenome;
		std::vector<std::pair<float,NEAT::Genome*>> LastBestGenomes; // ProgressMetricsIndividuals size
		int IndividualsSize;
		int Age;
		float SmoothedFitness;
	};

	class Population
	{
	public:
		// TODO : Docs
		Population(void* BaseWorld = nullptr, int EvaluationThreads = Settings::PopulationEvalThreads);
		~Population();

		// Creates a new population of random individuals
		// It also separates them into species
		// The size of the population is equal to SimulationSize*PopulationSizeMultiplier
		// The simulation size is the number of individuals that are simulated at the same time
		void Spawn(int PopulationSizeMultiplier, int SimulationSize);

		// Computes a single evolutive step
		// The callback is called right after evaluation
		// The MuteOutput parameter disables console output
		void Epoch(std::function<void(int)> EpochCallback = [](int){}, bool MuteOutput = false);

		const auto& GetIndividuals() const { return Individuals; }
		auto& GetIndividuals() { return Individuals; }
		const auto& GetSpecies() const { return SpeciesMap; }
		const auto& GetSpeciesRegistry() const { return StagnantSpecies; }

		// Runs the simulation with the provided decision functions, replacing one species at a time
		// Useful to compare the evolved network against some known decision function
		// It calls a callback function each time a species finishes simulating
		void SimulateWithUserFunction(std::unordered_map<MorphologyTag, decltype(Individual::UserDecisionFunction)> FunctionsMap, std::function<void(const MorphologyTag&)> Callback);

		// Evaluates the population
		void EvaluatePopulation();
		void CurrentSpeciesToRegistry();

		// Generates a report of the registry
		void SaveRegistryReport(const std::string& Path);
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

		// Used for parallel evaluation
		WorkerPool Workers;
		std::vector<void*> WorldArray;
	};
}