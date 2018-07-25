#include "Globals.h"

using namespace std;

// Definition of extern variables
namespace agio
{
	vector<ComponentGroup> ComponentRegistry;
	vector<ParameterDefinition> ParameterDefRegistry;
	vector<Action> ActionRegistry;
	vector<Sensor> SensorRegistry;

	namespace GlobalFunctions
	{
		function<float(class Individual *, void * World)> ComputeFitness;
		function<bool(class Individual *, void * World)> IsAlive;		
		std::function<void*()> MakeState;
		std::function<void(void * State)> ResetState;
		std::function<void(void*)> DestroyState;
	}
}