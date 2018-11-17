#include <assert.h>
#include <algorithm>
#include <enumerate.h>
#include <matplotlibcpp.h>
#include <queue>
#include <random>

#include "../../Core/Config.h"
#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include "../../Serialization/SRegistry.h"
#include "../../Utils/Math/Float2.h"

#include "DiversityPlot.h"
#include "PublicInterfaceImpl.h"

namespace plt = matplotlibcpp;
using namespace agio;
using namespace fpp;
using namespace std;

void runEvolution()
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
    WorldData world = createWorld();


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
    vector<float> avg_fitness_herbivore;
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
    plt::ion();

    for (int g = 0; g < GenerationsCount; g++)
    {
        pop.Epoch(&world, [&](int gen)
        {
            cout << "Generation : " << gen << endl;
            cout << "    Species Size [" << pop.GetSpecies().size() << "] : ";
            for (const auto&[_, species] : pop.GetSpecies())
                cout << species.IndividualsIDs.size() << " | " << species.ProgressMetric << " , ";
            cout << endl;

            // Every some generations graph the fitness & novelty of the individuals of the registry
            if (gen % 10 == 0)
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

                plt::clf();

                // Only average the best 5

                sort(fitness_vec_hervibore.begin(), fitness_vec_hervibore.end(), [](float a, float b)
                { return a > b; });
                sort(fitness_vec_carnivore.begin(), fitness_vec_carnivore.end(), [](float a, float b)
                { return a > b; });

                float avg_f_hervibore = accumulate(fitness_vec_hervibore.begin(),
                                                   fitness_vec_hervibore.begin() + min<int>(fitness_vec_hervibore.size(),5), 0.0f) / 5.0f;

                float avg_f_carnivore = accumulate(fitness_vec_carnivore.begin(),
                                                   fitness_vec_carnivore.begin() + min<int>(fitness_vec_carnivore.size(), 5), 0.0f) / 5.0f;

                float progress_carnivore, progress_herbivore, rand_f_carnivore, rand_f_hervibore;

                pop.ComputeDevMetrics(&world);

                for (const auto &[_, s] : pop.GetSpecies())
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

                avg_fitness_herbivore_random.push_back(rand_f_hervibore);
                avg_fitness_herbivore.push_back((avg_f_hervibore));
                avg_progress_herbivore.push_back(progress_herbivore);

                avg_fitness_carnivore.push_back((avg_f_carnivore));
                avg_fitness_carnivore_random.push_back(rand_f_carnivore);
                avg_progress_carnivore.push_back(progress_carnivore);


                plt::subplot(2, 2, 1);
                plt::plot(avg_fitness_herbivore, "b");
                plt::plot(avg_fitness_herbivore_random, "r");

                plt::subplot(2, 2, 2);
                plt::plot(avg_fitness_carnivore, "k");
                plt::plot(avg_fitness_carnivore_random, "r");

                plt::subplot(2, 2, 3);
                plt::plot(avg_progress_herbivore, "b");
                plt::plot(avg_progress_carnivore, "k");

                plt::pause(0.01);
            }
        });

    }
    pop.EvaluatePopulation(&world);
    pop.CurrentSpeciesToRegistry();

    SRegistry registry(&pop);
    registry.save(SerializationFile);
}