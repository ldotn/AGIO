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
	};

	// Represents an individual in the population
	class Individual
	{
	public:
		// Constructs a new, valid, individual
		Individual();

		// Does 3 steps
		//    1) Load the sensors from the world
		//    2) Input that values to the network and compute actions probabilities
		//    3) Select the action to do at random based on the probabilities output by the network
		// The return value is the action to execute
		int DecideAction(void * World);

		// TODO : Documentation
		void Reset();

		// Standard interface
		auto GetState() { return State; }
		const auto& GetComponents() const { return Components; }
		const auto& GetParameters() const { return Parameters; }
		const auto& GetGenome() const { return Genome; }

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
	};
}