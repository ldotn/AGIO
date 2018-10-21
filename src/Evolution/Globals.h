#pragma once
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace agio
{
	// Base struct for the entries of the global registries
	// Stores an optional name
	// TODO : Is this actually useful? Maybe just remove it
	struct RegistryEntry
	{
		//int GlobalID;
		std::string FriendlyName;
	};

	// Represents a component of an organism
	// A component defines a set of actions and/or sensors
	struct Component //: public RegistryEntry
	{
		std::vector<int> Actions;
		std::vector<int> Sensors;
	};

	// Groups related components together
	// It specifies a cardinality, used when constructing a new individual
	struct ComponentGroup //: public RegistryEntry
	{
		// How many items of this group must be on an individual
		// When selecting multiple components of a group there are no repetitions
		// MinCardinality <= Components.size() <= MaxCardinality
		int MinCardinality;
		int MaxCardinality;

		std::vector<Component> Components;
	};

	// Encapsulates the range of a parameter
	// Parameters are numeric values, treated as black boxes by the evolution algorithm
	// They give greater flexibility to the user
	struct ParameterDefinition : public RegistryEntry
	{
		ParameterDefinition() = default;
		ParameterDefinition(bool Required,float MinValue,float MaxValue, int ID, std::string Name = "")
		{
			IsRequired = Required;
			Min = MinValue;
			Max = MaxValue;
			FriendlyName = Name;
			UserID = ID;
		}

		// TODO : Maybe rework this, linking the parameters to the components
		bool IsRequired; // If true this parameter will be in all individuals
		float Min;
		float Max;

		int UserID; // Used to reference the parameters later
	};

	// Encapsulates the functionality of an action
	// An action takes a state, the population and the world
	// It executes the action, updating the state, the world and the population
	struct Action : public RegistryEntry
	{
		std::function<void(void * State,const std::vector<class BaseIndividual*> &, class BaseIndividual*, void * World)> Execute;

		Action() = default;
		Action(decltype(Execute) ActionFunction, std::string Name = "")
		{
			Execute = move(ActionFunction);
			FriendlyName = move(Name);
		}
	};

	// Encapsulates the functionality of a sensor
	struct Sensor : public RegistryEntry
	{
		// TODO : Maybe having it return only one value always is not the best idea
		//  Refactor this to return a vector
		std::function<float(void * State, const std::vector<class BaseIndividual*> &, const class BaseIndividual *, void * World)> Evaluate;
		
		Sensor() = default;
		Sensor(decltype(Evaluate) SensorFunction, std::string Name = "")
		{
			Evaluate = move(SensorFunction);
			FriendlyName = move(Name);
		}
	};

	// TODO : Docs?
	class PublicInterface
	{
	public:
		virtual ~PublicInterface() {};

		// Fills the registries
		virtual void Init() = 0;

		// Computes fitness for all the individuals in a population
		virtual void ComputeFitness(const std::vector<class BaseIndividual*>& Individuals, void * World) = 0;

		// Decides if an individual is alive or not
		// "Alive" can refer to an abstract notion of active, depending on the user intention
		// The simulation of an individual ends when IsAlive() returns false (or if over a certain number of steps)
		//virtual bool IsAlive(class Individual *, void * World) = 0;

		// Creates a new, possibly random, state
		virtual void * MakeState(const class BaseIndividual *) = 0;

		// Sets an state back into it's initial form
		virtual void ResetState(void * State) = 0;

		// Destroys an state
		virtual void DestroyState(void * State) = 0;

		// Creates a new state that has the same values as the provided state
		virtual void * DuplicateState(void * State) = 0;

		// Getters
		const auto & GetComponentRegistry() const { return ComponentRegistry; }
		const auto & GetParameterDefRegistry() const { return ParameterDefRegistry; }
		auto & GetParameterDefRegistry() { return ParameterDefRegistry; }
		const auto & GetActionRegistry() const { return ActionRegistry; }
		const auto & GetSensorRegistry() const { return SensorRegistry; }
	protected:
		std::vector<ComponentGroup> ComponentRegistry;
		std::vector<ParameterDefinition> ParameterDefRegistry;
		std::vector<Action> ActionRegistry;
		std::vector<Sensor> SensorRegistry;
	};

	// Singleton variable that stores a pointer to the public interface
	// The user MUST initialize it before using, by implementing the interface and creating a new object of the derived class
	extern PublicInterface* Interface;
}
