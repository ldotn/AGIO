#include "../../Utils/Math/Float2.h"
#include <vector>
#include <random>

namespace agio
{
	// While the real enviroment is 3D, for the evolution is simulated as a 2D enviroment
	// This forbids flying organisms
	struct OrgState
	{
		float2 Position;
		float2 Orientation;

		float Life;
		float Score; // Accumulated life

		// This is used when the organism is dead, and someone else is eating the corpse
		// Corpses are ignored after they have been eaten N times
		int EatenCount;
	};

	enum class ActionsIDs
	{
		Walk,
		Run,
		EatCorpse,
		EatPlant,
		Attack,
		TurnLeft,
		TurnRight,

		NumberOfActions
	};

	// Used to represent areas where there are things
	// Things inside the circle are spawned following a gaussian distribution, with mean in the center
	struct Circle
	{
		float2 Center;
		float Radius;

		// Gets a point in the circle following a gaussian distribution
		float2 GetSamplePoint(std::minstd_rand0 RNG);

		// Checks if a point is inside or not
		bool IsInside(float2 Point) { return (Point - Center).length() <= Radius; }
	};

	struct WorldData
	{
		std::vector<Circle> SpawnAreas;

		struct PlantArea
		{
			Circle Area;
			int NumEatingInside;
		};
		std::vector<PlantArea> PlantsAreas;
	};

	struct Rect
	{
		float2 Min;
		float2 Max;

		float2 ClampPos(float2 InPos)
		{
			return InPos.clamp(Min, Max);
		}
	};

	struct GameplayParams
	{
		// TODO : Use the real values, based on the UE4 level
		static float WalkSpeed = 1.0f;
		static float RunSpeed = 2.0f;
		static Rect GameArea = { {0,0}, {100,100} };
		static float TurnRadians = 10.0f * (3.14159f/180.0f);
		static float EatDistance = 0.5f;
		
		static int WastedActionPenalty = 10;

		static int CorpseBitesDuration = 5;
		static float EatCorpseLifeGained = 10;
		static float EatPlantLifeGained = 20;

		static float AttackDamage = 40;
		
		static float LifeLostPerTurn = 5;
	};
}