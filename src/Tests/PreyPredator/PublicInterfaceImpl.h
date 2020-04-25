#pragma once

#include <unordered_set>
#include <vector>

#include "../../Utils/Math/Float2.h"
#include "../../Evolution/Globals.h"
#include "../../Evolution/Individual.h"
#include "../../Evolution/Population.h"

using namespace std;
using namespace agio;

struct OrgState
{
    float Score = 0;
    float2 Position;

    bool IsCarnivore;

	float FailedActionCountCurrent;
	int EatenCount;
	int VisitedCellsCount;
	int Repetitions; // Divide the metrics by this to get the average values (the real metrics, otherwise it's the sum)
	int MetricsCurrentGenNumber;
	float FailableActionCount; // Divide the FailedActionCountCurrent by this to get average
	float FailedActionFractionAcc;

	struct pair_hash
	{
		inline size_t operator()(const pair<int, int> & v) const
		{
			return v.first ^ v.second;
		}
	};

	unordered_set<pair<int, int>,pair_hash> VisitedCells;
};

enum class ActionsIDs
{
    MoveForward,
    MoveBackwards,
    MoveLeft,
    MoveRight,

    EatFood,
    KillAndEat,

    NumberOfActions
};

enum class SensorsIDs
{
	NearestCompetitorDeltaX,
	NearestCompetitorDeltaY,

	NearestFoodDeltaX,
	NearestFoodDeltaY,

    NumberOfSensors
};

struct WorldData
{
    vector<float2> FoodPositions;

    void fill(int FoodCellCount, int WorldSizeX, int WorldSizeY)
    {
		minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

		FoodPositions.resize(FoodCellCount);
		for (auto &pos : FoodPositions)
		{
			pos.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
			pos.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
		}
    }
};

class PublicInterfaceImpl : public PublicInterface
{
private:
    minstd_rand RNG = minstd_rand(chrono::high_resolution_clock::now().time_since_epoch().count());
public:
    virtual ~PublicInterfaceImpl() override {};

    void Init() override;

    void * MakeState(const BaseIndividual * org) override;

    void ResetState(void * State) override;

    void DestroyState(void * State) override;

    void * DuplicateState(void * State) override;

	// Needs to be set BEFORE calling epoch
	// Used by the evaluation metrics
	int CurrentGenNumber = 0;

    // How many steps did the last simulation took before everyone died
    int LastSimulationStepCount = 0;

    // This computes fitness for the entire population
    void ComputeFitness(const std::vector<class BaseIndividual*>& Individuals, void * World) override;
};

class Metrics {
public:
	vector<float> fitness_vec_hervibore;
	vector<float> novelty_vec_hervibore;
	vector<float> fitness_vec_carnivore;
	vector<float> novelty_vec_carnivore;

	vector<float> fitness_vec_registry;
	vector<float> novelty_vec_registry;
	vector<float> avg_fitness_herbivore;
	vector<float> avg_progress_herbivore;
	vector<float> avg_fitness_carnivore;
	vector<float> avg_progress_carnivore;

	vector<float> avg_eaten_herbivore;
	vector<float> avg_eaten_carnivore;
	vector<float> avg_failed_herbivore;
	vector<float> avg_failed_carnivore;
	vector<float> avg_coverage_herbivore;
	vector<float> avg_coverage_carnivore;

	vector<float> min_eaten_herbivore;
	vector<float> min_eaten_carnivore;
	vector<float> min_failed_herbivore;
	vector<float> min_failed_carnivore;
	vector<float> min_coverage_herbivore;
	vector<float> min_coverage_carnivore;

	vector<float> max_eaten_herbivore;
	vector<float> max_eaten_carnivore;
	vector<float> max_failed_herbivore;
	vector<float> max_failed_carnivore;
	vector<float> max_coverage_herbivore;
	vector<float> max_coverage_carnivore;

	vector<float> min_eaten_herbivore_greedy;
	vector<float> min_eaten_carnivore_greedy;
	vector<float> max_eaten_herbivore_greedy;
	vector<float> max_eaten_carnivore_greedy;

	vector<float> avg_eaten_herbivore_greedy;
	vector<float> avg_eaten_carnivore_greedy;
	vector<float> avg_failed_herbivore_greedy;
	vector<float> avg_failed_carnivore_greedy;
	vector<float> avg_coverage_herbivore_greedy;
	vector<float> avg_coverage_carnivore_greedy;

	vector<float> avg_fitness_carnivore_random;
	vector<float> avg_fitness_herbivore_random;

	vector<float> avg_fitness_difference;
	vector<float> avg_fitness_network;
	vector<float> avg_fitness_random;
	vector<float> avg_novelty_registry;
	vector<float> species_count;
	vector<float> min_fitness_difference;
	vector<float> max_fitness_difference;

	Metrics();

	void update(Population &pop);

	void plot(Population &pop);

	~Metrics();
private:
	void calculate_metrics(Population &pop);
	float WorldCellCount;
};