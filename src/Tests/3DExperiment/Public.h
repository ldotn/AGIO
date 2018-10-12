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

	enum class ActionID
	{
		Walk,
		Run,
		EatCorpse,
		EatPlant,
		Attack,
		TurnLeft,
		TurnRight,

		Count
	};

	enum class SensorID
	{
		CurrentLife,
		CurrentPosX,
		CurrentPosY,
		CurrentOrientationX,
		CurrentOrientationY,

		DistanceNearestPartner,
		DistanceNearestCompetidor,
		DistanceNearestCorpse,
		DistanceNearestPlantArea,
		
		LifeNearestCompetidor,

		AngleNearestPartner,
		AngleNearestCompetidor,
		AngleNearestCorpse,
		AngleNearestPlantArea,

		Count
	};

	// Used to represent areas where there are things
	// Organisms inside the circle are spawned following a gaussian distribution, with mean in the center
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

		float2 ClampPos(float2 Pos)
		{
			return Pos.clamp(Min, Max);
		}

		// Normalizes a position inside the rect to be in [0,1] range
		float2 Normalize(float2 Pos)
		{
			return (Pos - Min) / (Max - Min);
		}
	};

	struct GameplayParams
	{
		static float WalkSpeed;
		static float RunSpeed;
		static Rect GameArea;
		static float TurnRadians;
		static float EatDistance;
		
		static int WastedActionPenalty;

		static int CorpseBitesDuration;
		static float EatCorpseLifeGained;
		static float EatPlantLifeGained;

		static float AttackDamage;
		
		static float LifeLostPerTurn;
	};
}