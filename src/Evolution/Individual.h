#pragma once
#include <vector>
#include <random>
#include "Globals.h"
#include <atomic>
#include <unordered_set>
#include <memory>
#include <unordered_map>
#include "genome.h"
#include "MorphologyTag.h"
#include "../Interface/BaseIndividual.h"
namespace agio
{
	// Represents an individual in the population
	class Individual : public BaseIndividual
	{
		friend class Population;
	public:
		// Constructor that set everything to null and seeds the RNG
		Individual() noexcept;

		// Constructor that clones an individual
		// The extra dummy parameter is just to separate this from a copy ctor
		/*enum class Make { Clone }; // Ignored, just to remember that this ctor makes a copy
		Individual(const Individual& Parent, Make);*/
		
		// Prevent copying
		Individual(const Individual&) = delete;

		// Clears all the resources used by the individual
		~Individual();

		// Move constructor that sets the resources to nullptr of the moved from object after the move
		// That's necessary so that the destructor doesn't try to release a moved resource
		Individual(Individual &&) noexcept;


		// Constructor that makes an individual from a NEAT genome and a morphology tag
		// NOTE : It duplicates the genome!
		Individual(MorphologyTag Morphology,NEAT::Genome * Genome);

		// Does 4 steps
		//    1) Load the sensors from the world
		//    2) Input that values to the network and compute actions probabilities
		//    3) Select the action to do at random based on the probabilities output by the network
		//	  4) Calls the selected action
		void DecideAndExecute(void * World, const class Population*);

		// "Lower level" version of the decide functionality
		// It expects as input the values for the sensors (as an id->value map) and returns the action ID
		int DecideAction(const std::unordered_map<int, float>& SensorsValues) override;

		// Reset the state and set fitness to -1
		// Also flush the neural network
		// Should be called before evaluating fitness
		void Reset() override;

		// Standard interface
		//auto GetState() override const { return State; }
		void * GetState() const override { return State; }
		//template<typename T>
		//T * GetState() const { return (T*)State; }
		//const auto& GetParameters() const { return Parameters; }
		const std::unordered_map<int, Parameter>& GetParameters() const override { return Parameters; }
		
		const auto& GetGenome() const { return Genome; }
		
		const MorphologyTag& GetMorphologyTag() const override { return Morphology;  }
		
		int GetGlobalID() const { return GlobalID; }
		int GetOriginalID() const { return OriginalID; }

		// Instantiation of the parameters
		// The map takes as keys the user id
		std::unordered_map<int, Parameter> Parameters;

		// The genome and the neural network it generates
		// Used to control de individual
		NEAT::Genome * Genome;
		NEAT::Network * Brain;

		// Fitness of the last evaluation of this individual
		// Reset sets it to 0
		float Fitness;
		
		// TODO : Refactor this, it feels dirty
		float AccumulatedFitness;
		//float AccumulatedNovelty;
		//float EvaluationsCount;
		bool InSimulation;
		float BackupFitness;

		// Serializes the individual to a file
		void DumpToFile(const std::string& FilePath);

		// Allows to override the network and just select actions randomly. Used to compute the progress metrics
		bool UseNetwork = true;

		struct
		{
			float PrevFitness;
		} DevMetrics;
	private:
		// The current id, across all individuals and all populations
		// Used to generate a global, unique id for the individuals
		int GlobalID;
		int OriginalID; // This is usually equal to the GlobalID, except when an individual is cloned. Then it's equal to the original individual ID
		inline static std::atomic<int> CurrentGlobalID = 0;

		// The state is defined by the user
		// For this code, it's a black box
		void * State;

		// On individual creation, this lists are filled from the components
		// They make the action decision faster,
		// because otherwise you would need to recreate them on each call
		std::vector<int> Actions;
		std::vector<int> Sensors;
		std::vector<float> SensorsValues;
		std::vector<float> ActivationsBuffer;

		std::minstd_rand RNG;

		// IDs of the components in the global (user provided) components registry
		MorphologyTag Morphology;

		// When this individual was created directly from a genome, the genome is duplicated, so it needs to be deleted
		bool NeedGenomeDeletion;
	};

	inline void swap(Individual& a, Individual& b)
	{
		Individual c(std::move(a)); 
		new (&a) Individual(std::move(b)); 
		new (&b) Individual(std::move(c));
	}
}