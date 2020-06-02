#include <assert.h>
#include <algorithm>
#include <enumerate.h>
#include <queue>
#include <random>
#include <iostream>
#include <numeric>
#include <chrono>

#include "../../Core/Config.h"
#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include "../../Serialization/SRegistry.h"
#include "../../Utils/Math/Float2.h"

#include "PreyPredator.h"
#include "PublicInterfaceImpl.h"

#include "../Greedy/Greedy.h"

// World data;
WorldData world;

using namespace agio;
using namespace fpp;
using namespace std;

Metrics::Metrics()
{
	//plt::ion();
	// Assuming it's already initialized
	WorldCellCount = WorldSizeX * WorldSizeY;
};

void Metrics::update(Population &pop)
{
	for (const auto&[_, species] : pop.GetSpecies())
	{
		// Find average evaluation metrics
		float avg_eaten = 0;
		float avg_failed = 0;
		float avg_coverage = 0;

		float min_eaten = numeric_limits<float>::max();
		float min_failed = numeric_limits<float>::max();
		float min_coverage = numeric_limits<float>::max();

		float max_eaten = 0;
		float max_failed = 0;
		float max_coverage = 0;

		for (int id : species.IndividualsIDs)
		{
			const auto& org = pop.GetIndividuals()[id];
			auto state_ptr = ((OrgState*)org.GetState());
			
			// SHOULD NEVER BE 0!!
			if (state_ptr->Repetitions == 0)
				continue;

			state_ptr->VisitedCellsCount += state_ptr->VisitedCells.size();
			if(state_ptr->FailableActionCount > 0)
				state_ptr->FailedActionFractionAcc += state_ptr->FailedActionCountCurrent / state_ptr->FailableActionCount;

			float eaten = float(state_ptr->EatenCount) / state_ptr->Repetitions;
			float failed = state_ptr->FailedActionFractionAcc / state_ptr->Repetitions;
			float coverage = (float(state_ptr->VisitedCellsCount) / WorldCellCount) / state_ptr->Repetitions;

			avg_eaten += eaten;
			avg_failed += failed;
			avg_coverage += coverage;

			min_eaten = min(min_eaten, eaten);
			min_failed = min(min_failed, failed);
			min_coverage = min(min_coverage, coverage);

			max_eaten = max(max_eaten, eaten);
			max_failed = max(max_failed, failed);
			max_coverage = max(max_coverage, coverage);
		}

		avg_eaten /= species.IndividualsIDs.size();
		avg_failed /= species.IndividualsIDs.size();
		avg_coverage /= species.IndividualsIDs.size();

		bool is_carnivore = false;
		if (((OrgState*)pop.GetIndividuals()[species.IndividualsIDs[0]].GetState())->IsCarnivore)
		{
			avg_eaten_carnivore.push_back(avg_eaten);
			avg_failed_carnivore.push_back(avg_failed);
			avg_coverage_carnivore.push_back(avg_coverage);

			min_eaten_carnivore.push_back(min_eaten);
			min_failed_carnivore.push_back(min_failed);
			min_coverage_carnivore.push_back(min_coverage);

			max_eaten_carnivore.push_back(max_eaten);
			max_failed_carnivore.push_back(max_failed);
			max_coverage_carnivore.push_back(max_coverage);
			is_carnivore = true;
			age_carnivore.push_back(species.Age);
		}
		else
		{
			avg_eaten_herbivore.push_back(avg_eaten);
			avg_failed_herbivore.push_back(avg_failed);
			avg_coverage_herbivore.push_back(avg_coverage);

			min_eaten_herbivore.push_back(min_eaten);
			min_failed_herbivore.push_back(min_failed);
			min_coverage_herbivore.push_back(min_coverage);

			max_eaten_herbivore.push_back(max_eaten);
			max_failed_herbivore.push_back(max_failed);
			max_coverage_herbivore.push_back(max_coverage);
			age_herbivore.push_back(species.Age);
		}
		cout << "    "<< (is_carnivore ? "[Carnivore]" : "[Herbivore]") << " Eaten: " << avg_eaten << "[max " << max_eaten << "] Failed: " << avg_failed << " Coverage: " << avg_coverage << endl;
	}
}

Metrics::~Metrics()
{
	auto timestamp = to_string(chrono::system_clock::now().time_since_epoch().count()) + "_";
	auto savef = [&timestamp](const vector<float>& vec, const string& fname)
	{
		ofstream file(timestamp + fname);

		for (auto[gen, x] : enumerate(vec))
			file << gen << "," << x << endl;
	};

	savef(age_herbivore, "age_herbivore.csv");
	savef(age_carnivore, "age_carnivore.csv");

	savef(avg_eaten_herbivore, "avg_eaten_herbivore.csv");
	savef(avg_eaten_carnivore, "avg_eaten_carnivore.csv");

	savef(avg_failed_herbivore, "avg_failed_herbivore.csv");
	savef(avg_failed_carnivore, "avg_failed_carnivore.csv");

	savef(avg_coverage_herbivore, "avg_coverage_herbivore.csv");
	savef(avg_coverage_carnivore, "avg_coverage_carnivore.csv");

	savef(min_eaten_herbivore, "min_eaten_herbivore.csv");
	savef(min_failed_herbivore, "min_failed_herbivore.csv");
	savef(min_coverage_herbivore, "min_coverage_herbivore.csv");

	savef(max_eaten_herbivore, "max_eaten_herbivore.csv");
	savef(max_failed_herbivore, "max_failed_herbivore.csv");
	savef(max_coverage_herbivore, "max_coverage_herbivore.csv");

	savef(min_eaten_carnivore, "min_eaten_carnivore.csv");
	savef(min_failed_carnivore, "min_failed_carnivore.csv");
	savef(min_coverage_carnivore, "min_coverage_carnivore.csv");

	savef(max_eaten_carnivore, "max_eaten_carnivore.csv");
	savef(max_failed_carnivore, "max_failed_carnivore.csv");
	savef(max_coverage_carnivore, "max_coverage_carnivore.csv");
}

unique_ptr<agio::Population> runEvolution()
{
	auto start_t = chrono::high_resolution_clock::now();

    minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

    NEAT::load_neat_params("../src/Tests/PreyPredator/NEATConfig.txt");
    NEAT::mutate_morph_param_prob = Settings::ParameterMutationProb;
    NEAT::destructive_mutate_morph_param_prob = Settings::ParameterDestructiveMutationProb;
    NEAT::mutate_morph_param_spread = Settings::ParameterMutationSpread;

	// Create and fill the world
	world.fill(FoodCellCount, WorldSizeX, WorldSizeY);

    // Create base interface
    Interface = new PublicInterfaceImpl();
    Interface->Init();

    // Spawn population
    auto pop = make_unique<Population>(&world);
    pop->Spawn(PopSizeMultiplier, SimulationSize);

    // Do evolution loop
    Metrics metrics;
    for (int g = 0; g < GenerationsCount; g++)
    {
		((PublicInterfaceImpl*)Interface)->CurrentGenNumber = g;

        pop->Epoch([&](int gen)
        {
			cout << "Generation : " << gen << endl;
            metrics.update(*pop);
            cout << endl;
        }, true);

    }
    pop->EvaluatePopulation();
    pop->CurrentSpeciesToRegistry();

    SRegistry registry(pop.get());
    registry.save(SerializationFile);

	auto end_t = chrono::high_resolution_clock::now();
	cout << "Time (s) : " << chrono::duration_cast<chrono::seconds>(end_t - start_t).count() << endl;

	{
		cout << "Press any letter + enter to exit" << endl;
		int tmp;
		cin >> tmp;
	}

	return pop;
}