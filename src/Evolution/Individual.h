#pragma once
#include <vector>
#include <random>
#include "Globals.h"
#include <atomic>

// Forward declaration
namespace NEAT
{
	class Genome;
	class Network;
}

namespace agio
{
	// Represents an instantiation of a specific parameter
	struct Parameter
	{
		int ID;
		float Value;
		int HistoricalMarker; // Used to keep track of changes, same as NEAT
	};

	// Represents an individual in the population
	class Individual
	{
	public:
		// Constructor that set everything to null and seeds the RNG
		Individual();

		// Constructs a new, valid, individual
		// Takes as input the ID of the individual inside the population
		void Spawn(int ID);

		// Does 4 steps
		//    1) Load the sensors from the world
		//    2) Input that values to the network and compute actions probabilities
		//    3) Select the action to do at random based on the probabilities output by the network
		//	  4) Calls the selected action
		void DecideAndExecute(void * World, const class Population*);

		// Creates a child by mating this two individuals
		// It assumes that the individuals are compatible
		// The child ID is the ID inside the offsprings
		Individual Mate(const Individual& Other,int ChildID);

		// Reset the state and set fitness to -1
		// Also flush the neural network
		// Should be called before evaluating fitness
		void Reset();

		// Standard interface
		auto GetState() const { return State; }
		template<typename T>
		T * GetState() const { return (T*)State; }
		const auto& GetComponents() const { return Components; }
		const auto& GetParameters() const { return Parameters; }
		const auto& GetGenome() const { return Genome; }
		const auto& GetMorphologyTag() const { return Morphology;  }
		int GetGlobalID() const { return GlobalID; }

		// Fitness of the last evaluation of this individual
		// Reset sets it to -1
		float LastFitness;

		// Novelty metric associated to the last evaluation
		// Reset sets it to -1
		float LastNoveltyMetric;

		// Pointer to the current species where the individual belongs
		class Species *SpeciesPtr;

		// Serializes the individual to a file
		void DumpToFile(const std::string& FilePath);

		// Mutates this individual
		void Mutate(class Population *pop, int generation);
	private:
		// The current id, across all individuals and all populations
		// Used to generate a global, unique id for the individuals
		int GlobalID;
		inline static std::atomic<int> CurrentGlobalID = 0;

		// The state is defined by the user
		// For this code, it's a black box
		void * State;

		// IDs of the components in the global (user provided) components registry
		struct ComponentRef
		{
			int GroupID;
			int ComponentID;
		};
		std::vector<ComponentRef> Components;

		// Instantiation of the parameters
		// The map takes as keys the user id
		std::unordered_map<int,Parameter> Parameters;

		// The genome and the neural network it generates
		// Used to control de individual
		// Forward declaration
		NEAT::Genome * Genome;
		NEAT::Network * Brain;

		// On individual creation, this lists are filled from the components
		// They make the action decision faster,
		// because otherwise you would need to recreate them on each call
		std::vector<int> Actions;
		std::vector<int> Sensors;
		std::vector<float> SensorsValues;
		std::vector<float> ActivationsBuffer;

		std::minstd_rand RNG;

		// Used to determine compatibility between individuals
		// Each element in the array it's a 64 bit bitfield
		// Each bit flags if the action/sensor is in the set or not
		// This LOOSES INFORMATION compared to the components list
		// You CAN'T know the components that make up an individual from the morphology tag
		//	there may be multiple components that provide the same sensors/actions
		// TODO : Maybe find a better name for this?
		// It also keeps track of the parameters
		// For equality only the historical markers are considered
		// For distance, the historical markers are ignored
	public:
		struct MorphologyTag
		{
			int NumberOfActions;
			int NumberOfSensors;

			std::vector<uint64_t> ActionsBitfield;
			std::vector<uint64_t> SensorsBitfield;
			std::unordered_map<int, Parameter> Parameters;
			NEAT::Genome * ControlGenome;

			// Checks that the actions and sensors are the same, 
			//  and that the parameters have the same historical marker
			bool operator==(const MorphologyTag &) const;

			// Takes the parameters into account and genes
			float Distance(const MorphologyTag &) const;

			// Compares actions and sensors to see if two individuals can mate
			bool IsCompatible(const MorphologyTag &) const;
		};
	private:
		MorphologyTag Morphology;
	};
}

// This needs to be outside of the agio namespace
namespace std
{
	template <>
	struct hash<agio::Individual::MorphologyTag>
	{
		std::size_t operator()(const agio::Individual::MorphologyTag& k) const
		{
			std::size_t seed = k.NumberOfActions + k.NumberOfSensors;
			
			// Ref : [https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector]
			for (auto& i : k.ActionsBitfield)
				seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for (auto& i : k.SensorsBitfield)
				seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for (auto& [_,p] : k.Parameters)
			{
				seed ^= p.ID + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= p.HistoricalMarker + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}

			return seed;
		}
	};

}