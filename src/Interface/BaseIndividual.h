#pragma once
#include <unordered_map>
#include <vector>
#include "../Evolution/MorphologyTag.h"
#include "genome.h"

namespace agio
{
	// Base interface for the individuals
	// Both the ones for evolution and for serialization derive from here
	class BaseIndividual
	{
	protected:
		// Contains the mapping from sensor id to index on the sensors values vector
		std::unordered_map<int, int> SensorsMap;

		// Contains the mapping from action id to index on the action values vector
		std::unordered_map<int, int> ActionsMap;

		// Stores the current value of the sensors
		std::vector<float> SensorsValues;
	public:
		virtual ~BaseIndividual() = default;

		bool InSimulation = false; // TODO : Remove this!
		
		// Does 4 steps
		//    1) Load the sensors from the world
		//    2) Input that values to the network and compute actions probabilities
		//    3) Select the action to do at random based on the probabilities output by the network
		//	  4) Calls the selected action
		virtual void DecideAndExecute(void * World, const std::vector<BaseIndividual*> &Individuals) = 0;

		// "Lower level" version of the decide functionality
		// It assumes that the sensor values are updated and returns the action ID
		virtual int DecideAction() = 0;

		// Returns the index into the sensors array for the provided sensor ID, or -1 if there is no such sensor
		// Low level, not usually used
		int GetSensorIndex(int SensorID)
		{
			auto iter = SensorsMap.find(SensorID);
			if (iter == SensorsMap.end())
				return -1;

			return iter->second;
		}

		// Used to update the values in the sensor map that's used by DecideAction
		// Low level, usually you just use DecideAndExecute
		// Returns false if the sensor didn't exist on the individual, and true otherwise
		// If you are going to call DecideAction manually, you MUST call this function on each sensor first
		bool UpdateSensorValue(int SensorID, float Value)
		{
			int idx = GetSensorIndex(SensorID);
			if(idx == -1)
				return false;

			SensorsValues[idx] = Value;

			return true;
		}

		// Reset the state and set fitness to -1
		// Also flush the neural network
		// Should be called before evaluating fitness
		virtual void Reset() = 0;

		// Black box
		void * State;

		// Getters
		void * GetState() const { return State; }
		template<typename T>
		T * GetState() const { return (T*)State; }

		virtual const std::unordered_map<int, Parameter>& GetParameters() const = 0;
		virtual const MorphologyTag& GetMorphologyTag() const = 0;
	};
}
