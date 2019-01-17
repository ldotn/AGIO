#include "../../Utils/Math/Float2.h"
#include <vector>
#include <random>
#include <utility>
#include <unordered_set>

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
		int CorpseRemainingDuration;

		// After an organism dies,
		// or the corpse has been eaten in the case of herbivore, reset it
		// Organisms that died once no longer update score
		bool HasDied;

		int FailedActionCountCurrent;
		int EatenCount;
		int VisitedCellsCount;
		int Repetitions; // Divide the metrics by this to get the average values (the real metrics, otherwise it's the sum)
		int MetricsCurrentGenNumber;
		int FailableActionCount; // Divide the FailedActionCountCurrent by this to get average
		int FailedActionFractionAcc;


		struct pair_hash
		{
			inline size_t operator()(const std::pair<int, int> & v) const
			{
				return v.first ^ v.second;
			}
		};

		std::unordered_set<std::pair<int, int>, pair_hash> VisitedCells;
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
		/*DeltaNearestPartnerX,
		DeltaNearestCompetidorX,
		DeltaNearestCorpseX,
		DeltaNearestPlantAreaX,
		DeltaNearestPartnerY,
		DeltaNearestCompetidorY,
		DeltaNearestCorpseY,
		DeltaNearestPlantAreaY,*/

		Count
	};

	// Used to represent areas where there are things
	struct Circle
	{
		float2 Center;
		float Radius;

		// Gets a point in the circle
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
		static float StartingLife;
	};
}