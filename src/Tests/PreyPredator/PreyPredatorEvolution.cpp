#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include <assert.h>
#include "../../Utils/Math/Float2.h"
#include <algorithm>
#include <random>
#include <matplotlibcpp.h>
#include <enumerate.h>
#include <queue>
#include "../../Core/Config.h"
#include "Config.h"
#include "PublicInterfaceImpl.h"
#include "../../Serialization/SRegistry.h"

namespace plt = matplotlibcpp;
using namespace std;
using namespace agio;
using namespace fpp;

int runSimulation()
{
    minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

    NEAT::load_neat_params("../cfg/neat_test_config.txt");
    NEAT::mutate_morph_param_prob = Settings::ParameterMutationProb;
    NEAT::destructive_mutate_morph_param_prob = Settings::ParameterDestructiveMutationProb;
    NEAT::mutate_morph_param_spread = Settings::ParameterMutationSpread;

    // Create base interface
    Interface = new PublicInterfaceImpl();
    Interface->Init();

    // Create and fill the world
    WorldData world;
    world.FoodPositions.resize(FoodCellCount);
    for (auto &pos : world.FoodPositions)
    {
        pos.x = uniform_int_distribution<int>(0, WorldSizeX - 1)(RNG);
        pos.y = uniform_int_distribution<int>(0, WorldSizeY - 1)(RNG);
    }

    // Spawn population
    Population pop;
    pop.Spawn(PopSizeMultiplier, SimulationSize);

    // Do evolution loop
    vector<float> fitness_vec_hervibore;
    vector<float> novelty_vec_hervibore;
    vector<float> fitness_vec_carnivore;
    vector<float> novelty_vec_carnivore;

    vector<float> fitness_vec_registry;
    vector<float> novelty_vec_registry;
    vector<float> avg_fitness;
    vector<float> avg_progress_herbivore;
    vector<float> avg_fitness_carnivore;
    vector<float> avg_progress_carnivore;

    vector<float> avg_fitness_carnivore_random;
    vector<float> avg_fitness_herbivore_random;

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
                cout << species.IndividualsIDs.size() << " | " << species.ProgressMetric << " , ";
            cout << endl;

#ifdef _DEBUG
            return;
#endif
            // Every some generations graph the fitness & novelty of the individuals of the registry
            if (true)//(gen % 10 == 0)
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

                //plt::subplot(3, 1, 1);
                //plt::plot(fitness_vec_hervibore, novelty_vec_hervibore, "xb");
                //plt::plot(fitness_vec_carnivore, novelty_vec_carnivore, "xk");
                //plt::loglog(fitness_vec, novelty_vec, "x");

                // Only average the best 5

                sort(fitness_vec_hervibore.begin(), fitness_vec_hervibore.end(), [](float a, float b)
                { return a > b; });
                sort(fitness_vec_carnivore.begin(), fitness_vec_carnivore.end(), [](float a, float b)
                { return a > b; });

                float avg_f_hervibore = accumulate(fitness_vec_hervibore.begin(), fitness_vec_hervibore.begin() +
                                                                                  min<int>(fitness_vec_hervibore.size(),
                                                                                           5), 0.0f) / 5.0f;
                //float avg_n_hervibore = accumulate(novelty_vec_hervibore.begin(), novelty_vec_hervibore.begin() + min<int>(novelty_vec_hervibore.size(), 5), 0.0f) / 5.0f;
                float avg_f_carnivore = accumulate(fitness_vec_carnivore.begin(), fitness_vec_carnivore.begin() +
                                                                                  min<int>(fitness_vec_carnivore.size(),
                                                                                           5), 0.0f) / 5.0f;
                //float avg_n_carnivore = accumulate(novelty_vec_carnivore.begin(), novelty_vec_carnivore.begin() + min<int>(novelty_vec_carnivore.size(), 5), 0.0f) / 5.0f;
                float progress_carnivore, progress_herbivore, rand_f_carnivore, rand_f_hervibore;

                pop.ComputeDevMetrics(&world);

                for (auto[_, s] : pop.GetSpecies())
                {
                    auto org_state = (OrgState*)pop.GetIndividuals()[s.IndividualsIDs[0]].GetState();
                    if (org_state->IsCarnivore)
                    {
                        progress_carnivore = s.ProgressMetric;
                        avg_f_carnivore = s.DevMetrics.RealFitness;
                        rand_f_carnivore = s.DevMetrics.RandomFitness;
                    } else
                    {
                        progress_herbivore = s.ProgressMetric;
                        avg_f_hervibore = s.DevMetrics.RealFitness;
                        rand_f_hervibore = s.DevMetrics.RandomFitness;
                    }
                }

                avg_fitness_carnivore_random.push_back(rand_f_carnivore);
                avg_fitness_herbivore_random.push_back(rand_f_hervibore);

                avg_fitness.push_back((avg_f_hervibore));
                avg_fitness_carnivore.push_back((avg_f_carnivore));

                avg_progress_carnivore.push_back(progress_carnivore);
                avg_progress_herbivore.push_back(progress_herbivore);


                //cout << metrics.AverageFitnessDifference << endl;


                plt::subplot(2, 2, 1);
                plt::plot(avg_fitness, "b");
                plt::plot(avg_fitness_herbivore_random, "r");

                plt::subplot(2, 2, 2);
                plt::plot(avg_fitness_carnivore, "k");
                plt::plot(avg_fitness_carnivore_random, "r");

                plt::subplot(2, 2, 3);
                plt::plot(avg_progress_herbivore, "b");
                plt::plot(avg_progress_carnivore, "k");

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

    SRegistry registry(&pop);
    registry.save("serialization.txt");
}