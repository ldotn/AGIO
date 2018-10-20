#include <assert.h>
#include <algorithm>
#include <enumerate.h>
#include <matplotlibcpp.h>
#include <random>
#include <queue>

#include "../../Interface/Interface.h"
#include "../../Core/Config.h"
#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include "../../Serialization/SPopulation.h"
#include "../../Utils/Math/Float2.h"
#include "Config.h"
#include "PublicInterfaceImpl.h"
#include "neat.h"

namespace plt = matplotlibcpp;
using namespace agio;
using namespace fpp;
using namespace std;

// TODO : Refactor, this could be spread on a couple files.
//   Need to see if that's actually better though

int main()
{
	minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

	// TODO : REMOVE THIS! I put it here just in the off case that the reason why the population is not improving is because some parameter wasn't loaded
	NEAT::load_neat_params("../cfg/neat_test_config.txt");
	NEAT::mutate_morph_param_prob = Settings::ParameterMutationProb;
	NEAT::destructive_mutate_morph_param_prob = Settings::ParameterDestructiveMutationProb;
	NEAT::mutate_morph_param_spread = Settings::ParameterMutationSpread;

	// Create base interface
	Interface = unique_ptr<PublicInterface>(new PublicInterfaceImpl);
	Interface->Init();

	// Create and fill the world
	WorldData world;
	world.FoodPositions.resize(FoodCellCount);
	for (auto& pos : world.FoodPositions)
	{
		pos.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
		pos.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
	}

	// Spawn population
	Population pop;
	pop.Spawn(PopSizeMultiplier,SimulationSize);
	
	// Do evolution loop
	vector<float> fitness_vec_hervibore;
	vector<float> novelty_vec_hervibore;
	vector<float> fitness_vec_carnivore;
	vector<float> novelty_vec_carnivore;

	vector<float> fitness_vec_registry;
	vector<float> novelty_vec_registry;
	vector<float> avg_fitness;
	vector<float> avg_novelty;
	vector<float> avg_fitness_carnivore;
	vector<float> avg_novelty_carnivore;

	vector<float> avg_fitness_difference;
	vector<float> avg_fitness_network;
	vector<float> avg_fitness_random;
	vector<float> avg_novelty_registry;
	vector<float> species_count;
	vector<float> min_fitness_difference;
	vector<float> max_fitness_difference;

	// TODO : Make the plot work in debug
#ifndef _DEBUG
	plt::ion();
#endif

	for (int g = 0; g < GenerationsCount; g++)
	{
		pop.Epoch(&world, [&](int gen)
		{
			cout << "Generation : " << gen << endl;
			cout << "    Species Size [" << pop.GetSpecies().size() << "] : ";
			for (const auto&[_, species] : pop.GetSpecies())
				cout << species->IndividualsIDs.size() << " | " << species->ProgressMetric <<  " , ";
			cout << endl;

#ifdef _DEBUG
			auto metrics = pop.ComputeProgressMetrics(&world);
			cout << metrics.ProgressMetric << endl;
			return;
#endif
			// Every some generations graph the fitness & novelty of the individuals of the registry
			if (true)//(gen % 10 == 0)
			{
				fitness_vec_hervibore.resize(0);
				novelty_vec_hervibore.resize(0);
				fitness_vec_carnivore.resize(0);
				novelty_vec_carnivore.resize(0);

				for (auto [idx, org] : enumerate(pop.GetIndividuals()))
				{
					if (org.GetState<OrgState>()->IsCarnivore)
					{
						fitness_vec_carnivore.push_back(org.Fitness);
						novelty_vec_carnivore.push_back(org.LastNoveltyMetric);
					}
					else
					{
						fitness_vec_hervibore.push_back(org.Fitness);
						novelty_vec_hervibore.push_back(org.LastNoveltyMetric);
					}
				}

				/*float avg_f = 0;
				float avg_n = 0;

				int registry_count = 0;
				fitness_vec_registry.resize(0);
				novelty_vec_registry.resize(0);
				for (const auto&[_, individuals] : pop.GetNonDominatedRegistry())
				{
					for (const auto& org : individuals)
					{
						avg_f += org.LastFitness;
						avg_n += org.LastNoveltyMetric;

						fitness_vec_registry.push_back(org.LastFitness);
						novelty_vec_registry.push_back(org.LastNoveltyMetric);

						registry_count++;
					}
				}

				avg_f /= (float)registry_count;
				avg_n /= (float)registry_count;*/
#ifdef _DEBUG
				//cout << "    Avg Fitness (registry) : " << avg_f << " , Avg Novelty (registry) : " << avg_n << endl;
#else
				/*auto metrics = pop.ComputeProgressMetrics(&world, 10);
				avg_fitness_difference.push_back(metrics.AverageFitnessDifference);
				avg_novelty_registry.push_back(metrics.AverageNovelty);

				avg_f = accumulate(fitness_vec.begin(), fitness_vec.end(), 0) / fitness_vec.size();
				avg_n = accumulate(novelty_vec.begin(), novelty_vec.end(), 0) / novelty_vec.size();

				avg_fitness.push_back(avg_f);
				avg_novelty.push_back(avg_n);
				species_count.push_back(pop.GetNonDominatedRegistry().size());

				plt::clf();

				plt::subplot(7, 1, 1);
				plt::loglog(fitness_vec, novelty_vec, "x");

				plt::subplot(7, 1, 2);
				plt::loglog(fitness_vec_registry, novelty_vec_registry, string("x"));

				plt::subplot(7, 1, 3);
				plt::plot(avg_fitness, "r");

				plt::subplot(7, 1, 4);
				plt::plot(avg_novelty, "g");

				plt::subplot(7, 1, 5);
				plt::plot(species_count, "k");

				plt::subplot(7, 1, 6);
				plt::plot(avg_fitness_difference, "r");
				//plt::hist(fitness_vec_registry);

				plt::subplot(7, 1, 7);
				plt::plot(avg_novelty_registry, "g");
				//plt::hist(novelty_vec_registry);*/

				//auto metrics = pop.ComputeProgressMetrics(&world);
				/*avg_fitness_difference.push_back(metrics.AverageFitnessDifference);
				avg_fitness_network.push_back(metrics.AverageFitness);
				avg_fitness_random.push_back(metrics.AverageRandomFitness);
				avg_novelty_registry.push_back(metrics.AverageNovelty);

				min_fitness_difference.push_back(metrics.MinFitnessDifference);
				max_fitness_difference.push_back(metrics.MaxFitnessDifference);*/

				plt::clf();

				plt::subplot(3, 1, 1);
				plt::plot(fitness_vec_hervibore, novelty_vec_hervibore, "xb");
				plt::plot(fitness_vec_carnivore, novelty_vec_carnivore, "xk");
				//plt::loglog(fitness_vec, novelty_vec, "x");

				// Only average the best 5
				sort(fitness_vec_hervibore.begin(), fitness_vec_hervibore.end(), [](float a, float b) { return a > b; });
				sort(fitness_vec_carnivore.begin(), fitness_vec_carnivore.end(), [](float a, float b) { return a > b; });

				float avg_f_hervibore = accumulate(fitness_vec_hervibore.begin(), fitness_vec_hervibore.begin() + min<int>(fitness_vec_hervibore.size(), 5), 0.0f) / 5.0f;
				float avg_n_hervibore = accumulate(novelty_vec_hervibore.begin(), novelty_vec_hervibore.begin() + min<int>(novelty_vec_hervibore.size(), 5), 0.0f) / 5.0f;
				float avg_f_carnivore = accumulate(fitness_vec_carnivore.begin(), fitness_vec_carnivore.begin() + min<int>(fitness_vec_carnivore.size(), 5), 0.0f) / 5.0f;
				float avg_n_carnivore = accumulate(novelty_vec_carnivore.begin(), novelty_vec_carnivore.begin() + min<int>(novelty_vec_carnivore.size(), 5), 0.0f) / 5.0f;
				for (auto[_, s] : pop.GetSpecies())
				{
					if (pop.GetIndividuals()[s->IndividualsIDs[0]].GetState<OrgState>()->IsCarnivore)
						avg_n_carnivore = s->ProgressMetric;
					else
						avg_n_hervibore = s->ProgressMetric;
				}

				avg_fitness.push_back((avg_f_hervibore));
				avg_fitness_carnivore.push_back((avg_f_carnivore));

				avg_novelty_carnivore.push_back(avg_n_carnivore);
				avg_novelty.push_back(avg_n_hervibore);


				//cout << metrics.AverageFitnessDifference << endl;


				plt::subplot(3, 1, 2);
				plt::plot(avg_fitness, "b");
				plt::plot(avg_fitness_carnivore, "k");

				plt::subplot(3, 1, 3);
				plt::plot(avg_novelty, "b");
				plt::plot(avg_novelty_carnivore, "k");

				//plt::subplot(4, 1, 4);
				//plt::plot(avg_fitness_difference, "r");
				//plt::plot(min_fitness_difference, "k");
				//plt::plot(max_fitness_difference, "b");

				//plt::subplot(5, 1, 5);
				//plt::plot(avg_novelty_registry, "g");

				plt::pause(0.01);
#endif
			}
		});

	}

	pop.EvaluatePopulation(&world);
	// After evolution is done, replace the population with the ones of the registry
	// another interesting option would be to select 5 random individuals from the non dominated front, maybe for each species
	// Find the best 5 prey and predator
	{
		auto comparator = [](pair<int, float> a, pair<int, float> b) { return a.second > b.second; };
		priority_queue<pair<int, float>, vector<pair<int, float>>, decltype(comparator)> prey_queue(comparator), predator_queue(comparator);

		for (auto[idx, org] : enumerate(pop.GetIndividuals()))
		{
			if (org.GetState<OrgState>()->IsCarnivore)
			{
				predator_queue.push({ idx, org.Fitness });
				if (predator_queue.size() == 6)
					predator_queue.pop();
			}
			else
			{
				prey_queue.push({ idx, org.Fitness });
				if (prey_queue.size() == 6)
					prey_queue.pop();
			}
		}

		vector<Individual> tmp;
		while (not predator_queue.empty())
		{
			tmp.push_back(move(pop.GetIndividuals()[predator_queue.top().first]));
			predator_queue.pop();
		}

		while (not prey_queue.empty())
		{
			tmp.push_back(move(pop.GetIndividuals()[prey_queue.top().first]));
			prey_queue.pop();
		}

		for (auto& org : tmp)
			org.InSimulation = true;

		pop.GetIndividuals() = move(tmp);
	}

    pop.save("population.txt");
}