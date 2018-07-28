#pragma once
#include <vector>
#include <random>
#include "Globals.h"

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

		// Checks if this individual and the provided one are compatible (can cross)
		bool IsMorphologicallyCompatible(const Individual& Other);

		// Does 3 steps
		//    1) Load the sensors from the world
		//    2) Input that values to the network and compute actions probabilities
		//    3) Select the action to do at random based on the probabilities output by the network
		// The return value is the action to execute
		int DecideAction(void * World);

		// Creates a child by mating this two individuals
		// It assumes that the individuals are compatible
		// The child ID is the ID inside the offsprings
		Individual Mate(const Individual& Other,int ChildID);

		// Reset the state and set fitness to -1
		// Also flush the neural network
		// Should be called before evaluating fitness
		void Reset();

		// Standard interface
		auto GetState() { return State; }
		const auto& GetComponents() const { return Components; }
		const auto& GetParameters() const { return Parameters; }
		const auto& GetGenome() const { return Genome; }

		// Fitness of the last evaluation of this individual
		// Reset sets it to -1
		float LastFitness;
	private:
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
		// Just an ID into the global (user provided) parameters definition registry and the value
		std::vector<Parameter> Parameters;

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
		std::vector<uint64_t> ActionsBitfield;
		std::vector<uint64_t> SensorsBitfield;
	};
}