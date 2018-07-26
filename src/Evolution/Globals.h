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

	// TODO : Docs
	class PublicInterface
	{
	public:
		// Fills the registries
		virtual void Init() = 0;

		// Returns a numeric value that represent how good is an individual
		virtual float ComputeFitness(class Individual *, void * World) = 0;

		// Decides if an individual is alive or not
		// "Alive" can refer to an abstract notion of active, depending on the user intention
		// The simulation of an individual ends when IsAlive() returns false (or if over a certain number of steps)
		virtual bool IsAlive(class Individual *, void * World) = 0;

		// Creates a new, possibly random, state
		virtual void * MakeState() = 0;

		// Sets an state back into it's initial form
		virtual void ResetState(void * State) = 0;

		// Destroys an state
		virtual void DestroyState(void * State) = 0;

		// Getters
		const auto & GetComponentRegistry() { return ComponentRegistry; }
		const auto & GetParameterDefRegistry() { return ParameterDefRegistry; }
		const auto & GetActionRegistry() { return ActionRegistry; }
		const auto & GetSensorRegistry() { return SensorRegistry; }
	private:
		std::vector<ComponentGroup> ComponentRegistry;
		std::vector<ParameterDefinition> ParameterDefRegistry;
		std::vector<Action> ActionRegistry;
		std::vector<Sensor> SensorRegistry;
	};

	// Singleton variable that stores a pointer to the public interface
	// The user MUST initialize it before using, by implementing the interface and creating a new object of the derived class
	extern std::unique_ptr<PublicInterface> Interface;
}
