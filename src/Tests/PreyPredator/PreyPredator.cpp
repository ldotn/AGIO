#include <matplotlibcpp.h>

#include "../../Serialization/SPopulation.h"
#include "PublicInterfaceImpl.h"
#include "Config.h"

namespace plt = matplotlibcpp;
using namespace agio;
using namespace fpp;
using namespace std;

int main()
{
    minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

    // Initialize base interface
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

    SPopulation pop = SPopulation::load("population.txt");
    while (true)
    {
        for (auto &org : pop.individuals)
        {
            Interface->ResetState(org.state);
            org.brain.flush();
        }

        vector<int> food_x, food_y;
        food_x.resize(world.FoodPositions.size());
        food_y.resize(world.FoodPositions.size());

        bool any_alive = true;
//vector<int> org_x, org_y;
//org_x.reserve(PopulationSize);
//org_y.reserve(PopulationSize);

        while (any_alive)
        {
            for (auto[idx, pos] : enumerate(world.FoodPositions))
            {
                food_x[idx] = pos.x;
                food_y[idx] = pos.y;
            }

            any_alive = false;
            for (auto &org : pop.individuals)
                org.DecideAndExecute(&world, &pop);

// Plot food
            plt::clf();
            plt::plot(food_x, food_y, "gx");

// Plot the individuals with different colors per species
/*for (auto [species_idx,entry] : enumerate(pop.GetSpecies()))
{
auto [_, species] = entry;
org_x.resize(species.IndividualsIDs.size());
org_y.resize(species.IndividualsIDs.size());

for (auto [idx, org_idx] : enumerate(species.IndividualsIDs))
{
auto state_ptr = (OrgState*)pop.GetIndividuals()[org_idx].GetState();
if (state_ptr->Life)
{
any_alive = true;
org_x[idx] = state_ptr->Position.x;
org_y[idx] = state_ptr->Position.y;
}
}

plt::plot(org_x, org_y, string("x") + string("C") + to_string(species_idx));
}*/

// Plot herbivores on blue and carnivores on black
            vector<int> herbivores_x, herbivores_y;
            vector<int> carnivores_x, carnivores_y;
            herbivores_x.reserve(PopulationSize);
            herbivores_y.reserve(PopulationSize);
            carnivores_x.reserve(PopulationSize);
            carnivores_y.reserve(PopulationSize);

            for (const auto &org : pop.individuals)
            {
                auto state_ptr = (OrgState *) org.state;

                any_alive = true;
                if (state_ptr->IsCarnivore)
                {
                    carnivores_x.push_back(state_ptr->Position.x);
                    carnivores_y.push_back(state_ptr->Position.y);
                } else
                {
                    herbivores_x.push_back(state_ptr->Position.x);
                    herbivores_y.push_back(state_ptr->Position.y);
                }
            }
            plt::plot(herbivores_x, herbivores_y, "ob");
            plt::plot(carnivores_x, carnivores_y, "ok");

            plt::xlim(-1, WorldSizeX);
            plt::ylim(-1, WorldSizeY);
            plt::pause(0.1);
        }
    }
}