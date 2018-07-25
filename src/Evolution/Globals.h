#pragma once
#include <vector>
#include <string>
#include <functional>

namespace agio
{
	// Base struct for the entries of the global registries
	// Stores an optional name
	struct RegistryEntry
	{
		//int GlobalID;
		std::string FriendlyName;
	};

	// Represents a component of an organism
	// A component defines a set of actions and/or sensors
	struct Component : public RegistryEntry
	{
		std::vector<int> Actions;
		std::vector<int> Sensors;
	};

	// Groups related components together
	// It specifies a cardinality, used when constructing a new individual
	struct ComponentGroup : public RegistryEntry
	{
		// How many items of this group must be on an individual
		// When selecting multiple components of a group there are no repetitions
		int MinCardinality;
		int MaxCardinality;

		std::vector<Component> Components;
	};

	// Encapsulates the range of a parameter
	// Parameters are numeric values, treated as black boxes by the evolution algorithm
	// They give greater flexibility to the user
	struct ParameterDefinition : public RegistryEntry
	{
		// Used when constructing the individuals
		// See "ComponentGroup"
		int MinCardinality;
		int MaxCardinality;

		float Min;
		float Max;
	};

	// Encapsulates the functionality of an action
	// An action takes a state, the population and the world
	// It executes the action, updating the state, the world and the population
	struct Action : public RegistryEntry
	{
		std::function<void(void * State, class Population *,class Individual *, void * World)> Execute;
	};

	// Encapsulates the functionality of a sensor
	struct Sensor : public RegistryEntry
	{
		std::function<float(void * State, void * World, const class Individual *)> Evaluate;
	};

	// Global registries
	extern std::vector<ComponentGroup> ComponentRegistry;
	extern std::vector<ParameterDefinition> ParameterDefRegistry;
	extern std::vector<Action> ActionRegistry;
	extern std::vector<Sensor> SensorRegistry;

	// Global singleton functions
	namespace GlobalFunctions
	{
		// Returns a numeric value that represent how good is an individual
		extern std::function<float(class Individual *, void * World)> ComputeFitness;

		// Decides if an individual is alive or not
		// "Alive" can refer to an abstract notion of active, depending on the user intention
		// The simulation of an individual ends when IsAlive() returns false (or if over a certain number of steps)
		extern std::function<bool(class Individual *, void * World)> IsAlive;

		// Creates a new, possibly random, state
		extern std::function<void*()> MakeState;

		// Sets an state back into it's initial form
		extern std::function<void(void * State)> ResetState;

		// Destroys an state
		extern std::function<void(void*)> DestroyState;
	}
}
