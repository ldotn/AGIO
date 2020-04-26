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

			avg_eaten += (float)state_ptr->EatenCount / state_ptr->Repetitions;
			avg_failed += (float)state_ptr->FailedActionFractionAcc / state_ptr->Repetitions;
			avg_coverage += ((float)state_ptr->VisitedCellsCount / WorldCellCount) / state_ptr->Repetitions;

			min_eaten = min(min_eaten, (float)state_ptr->EatenCount / state_ptr->Repetitions);
			min_failed = min(min_failed, (float)state_ptr->FailedActionFractionAcc / state_ptr->Repetitions);
			min_coverage = min(min_coverage, ((float)state_ptr->VisitedCellsCount / WorldCellCount) / state_ptr->Repetitions);

			max_eaten = max(max_eaten, (float)state_ptr->EatenCount / state_ptr->Repetitions);
			max_failed = max(max_failed, (float)state_ptr->FailedActionFractionAcc / state_ptr->Repetitions);
			max_coverage = max(max_coverage, ((float)state_ptr->VisitedCellsCount / WorldCellCount) / state_ptr->Repetitions);
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
#ifdef RUN_GREEDY
	// Force a reset. REFACTOR!
	for (auto& org : pop.GetIndividuals())
	{
		auto state_ptr = (OrgState*)org.GetState();

		state_ptr->MetricsCurrentGenNumber = -1;
		state_ptr->VisitedCellsCount = 0;
		state_ptr->VisitedCells = {};
		state_ptr->EatenCount = 0;
		state_ptr->FailedActionCountCurrent = 0;
		state_ptr->Repetitions = 0;
		state_ptr->FailableActionCount = 0;
		state_ptr->FailedActionFractionAcc = 0;
	}

	pop.SimulateWithUserFunction(&world, createGreedyActionsMap(),
		[&](const MorphologyTag& tag)
	{
		float max_eaten = 0;
		float min_eaten = numeric_limits<float>::max();
		float avg_eaten = 0;
		float avg_failed = 0;
		float avg_coverage = 0;
		const auto& species = pop.GetSpecies().find(tag)->second;


		for (int id : species.IndividualsIDs)
		{
			const auto& org = pop.GetIndividuals()[id];
			auto state_ptr = ((OrgState*)org.GetState());

			avg_eaten += (float)state_ptr->EatenCount / state_ptr->Repetitions;
			avg_failed += (float)state_ptr->FailedActionFractionAcc / state_ptr->Repetitions;
			avg_coverage += (float)state_ptr->VisitedCellsCount / state_ptr->Repetitions;
			max_eaten = max(max_eaten, (float)state_ptr->EatenCount / state_ptr->Repetitions);
			min_eaten = min(min_eaten, (float)state_ptr->EatenCount / state_ptr->Repetitions);
		}

		avg_eaten /= species.IndividualsIDs.size();
		avg_failed /= species.IndividualsIDs.size();
		avg_coverage /= species.IndividualsIDs.size();

		// Find org type
		bool is_carnivore;
		for (auto[gid, cid] : tag)
		{
			if (gid == 0) // mouth group
			{
				if (cid == 0) // herbivore
					is_carnivore = false;
				else if (cid == 1)
					is_carnivore = true;
			}
		}

		// Update values
		if (is_carnivore)
		{
			avg_eaten_carnivore_greedy.push_back(avg_eaten);
			avg_failed_carnivore_greedy.push_back(avg_failed);
			avg_coverage_carnivore_greedy.push_back(avg_coverage);
			min_eaten_carnivore_greedy.push_back(min_eaten);
			max_eaten_carnivore_greedy.push_back(max_eaten);
		}
		else
		{
			avg_eaten_herbivore_greedy.push_back(avg_eaten);
			avg_failed_herbivore_greedy.push_back(avg_failed);
			avg_coverage_herbivore_greedy.push_back(avg_coverage);
			min_eaten_herbivore_greedy.push_back(min_eaten);
			max_eaten_herbivore_greedy.push_back(max_eaten);
		}

		// Force a reset. REFACTOR!
		for (auto& org : pop.GetIndividuals())
		{
			auto state_ptr = (OrgState*)org.GetState();

			state_ptr->MetricsCurrentGenNumber = -1;
			state_ptr->VisitedCellsCount = 0;
			state_ptr->VisitedCells = {};
			state_ptr->EatenCount = 0;
			state_ptr->FailedActionCountCurrent = 0;
			state_ptr->Repetitions = 0;
			state_ptr->FailableActionCount = 0;
			state_ptr->FailedActionFractionAcc = 0;
		}
	});
#endif
}

void Metrics::plot(Population &pop)
{
	//calculate_metrics(pop);
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
	savef(avg_eaten_herbivore_greedy, "avg_eaten_herbivore_greedy.csv");
	savef(avg_eaten_carnivore_greedy, "avg_eaten_carnivore_greedy.csv");

	savef(avg_failed_herbivore, "avg_failed_herbivore.csv");
	savef(avg_failed_carnivore, "avg_failed_carnivore.csv");
	savef(avg_failed_herbivore_greedy, "avg_failed_herbivore_greedy.csv");
	savef(avg_failed_carnivore_greedy, "avg_failed_carnivore_greedy.csv");

	savef(avg_coverage_herbivore, "avg_coverage_herbivore.csv");
	savef(avg_coverage_carnivore, "avg_coverage_carnivore.csv");
	savef(avg_coverage_herbivore_greedy, "avg_coverage_herbivore_greedy.csv");
	savef(avg_coverage_carnivore_greedy, "avg_coverage_carnivore_greedy.csv");

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

	savef(max_eaten_herbivore_greedy, "max_eaten_herbivore_greedy.csv");
	savef(min_eaten_herbivore_greedy, "min_eaten_herbivore_greedy.csv");
	savef(max_eaten_carnivore_greedy, "max_eaten_carnivore_greedy.csv");
	savef(min_eaten_carnivore_greedy, "min_eaten_carnivore_greedy.csv");
}

void Metrics::calculate_metrics(Population &pop)
	{
		fitness_vec_hervibore.resize(0);
		novelty_vec_hervibore.resize(0);

		for (auto[idx, org] : enumerate(pop.GetIndividuals()))
		{
			if (((OrgState*)org.GetState())->IsCarnivore)
				fitness_vec_carnivore.push_back(org.Fitness);
			else
				fitness_vec_hervibore.push_back(org.Fitness);
		}

		// Only average the best 5
		sort(fitness_vec_hervibore.begin(), fitness_vec_hervibore.end(), [](float a, float b) { return a > b; });
		sort(fitness_vec_carnivore.begin(), fitness_vec_carnivore.end(), [](float a, float b) { return a > b; });

		float avg_f_hervibore = accumulate(fitness_vec_hervibore.begin(),
			fitness_vec_hervibore.begin() + min<int>(fitness_vec_hervibore.size(), 5), 0.0f) / 5.0f;

		float avg_f_carnivore = accumulate(fitness_vec_carnivore.begin(),
			fitness_vec_carnivore.begin() + min<int>(fitness_vec_carnivore.size(), 5), 0.0f) / 5.0f;

		float progress_carnivore, progress_herbivore, rand_f_carnivore, rand_f_hervibore;

		for (const auto &[_, s] : pop.GetSpecies())
		{
			auto org_state = (OrgState*)pop.GetIndividuals()[s.IndividualsIDs[0]].GetState();
			if (org_state->IsCarnivore)
			{
				progress_carnivore = s.ProgressMetric;
				avg_f_carnivore = s.DevMetrics.RealFitness;
				rand_f_carnivore = s.DevMetrics.RandomFitness;
			}
			else
			{
				progress_herbivore = s.ProgressMetric;
				avg_f_hervibore = s.DevMetrics.RealFitness;
				rand_f_hervibore = s.DevMetrics.RandomFitness;
			}
		}

		avg_fitness_herbivore_random.push_back(rand_f_hervibore);
		avg_fitness_herbivore.push_back((avg_f_hervibore));
		avg_progress_herbivore.push_back(progress_herbivore);

		avg_fitness_carnivore.push_back((avg_f_carnivore));
		avg_fitness_carnivore_random.push_back(rand_f_carnivore);
		avg_progress_carnivore.push_back(progress_carnivore);
	}

unique_ptr<agio::Population> runEvolution()
{
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
    auto pop = make_unique<Population>(&world, 24);
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

            metrics.plot(*pop);
        }, true);

    }
    pop->EvaluatePopulation();
    pop->CurrentSpeciesToRegistry();

    SRegistry registry(pop.get());
    registry.save(SerializationFile);

	{
		cout << "Press any letter + enter to exit" << endl;
		int tmp;
		cin >> tmp;
	}

	return pop;
}