#include <assert.h>
#include <algorithm>
#include <enumerate.h>
#include <queue>
#include <random>
#include <iostream>
#include <numeric>

#include "../../Core/Config.h"
#include "../../Evolution/Population.h"
#include "../../Evolution/Globals.h"
#include "../../Serialization/SRegistry.h"
#include "../../Utils/Math/Float2.h"

#include "ComplexSystem.h"
#include "PublicInterfaceImpl.h"

using namespace agio;
using namespace fpp;
using namespace std;

unique_ptr<agio::Population> runEvolution(const std::string& WorldPath, bool NoOutput)
{
    minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

    NEAT::load_neat_params("../cfg/neat_test_config.txt");
    NEAT::mutate_morph_param_prob = Settings::ParameterMutationProb;
    NEAT::destructive_mutate_morph_param_prob = Settings::ParameterDestructiveMutationProb;
    NEAT::mutate_morph_param_spread = Settings::ParameterMutationSpread;

	// Create and fill the world
	WorldData world = createWorld(WorldPath);

    // Create base interface
    Interface = new PublicInterfaceImpl();
    Interface->Init();

    // Spawn population
    auto pop = make_unique<Population>(&world);
    pop->Spawn(PopSizeMultiplier, SimulationSize);

    for (int g = 0; g < GenerationsCount; g++)
    {
		pop->Epoch([&](int gen)
        {
			if (NoOutput) return;

            cout << "Generation " << gen 
				<< ", " << pop->GetSpecies().size() << " Species" << endl;

			cout << "------------------------------" << endl;
			for (const auto& [_, species] : pop->GetSpecies())
			{
				cout << "    "
					 << " Size : " << species.IndividualsIDs.size()
				     << endl;
				cout << "    "
					 << " Progress : " << species.ProgressMetric
				     << endl;
				cout << "    "
					<< " Type (carnivore, herbivore, omnivore) : " << (int)pop->GetIndividuals()[species.IndividualsIDs[0]].GetState<OrgState>()->Type
					<< endl;

				float avg_fit = 0;
				float max_fit = numeric_limits<float>::lowest();
				float avg_eaten = 0;
				for (int id : species.IndividualsIDs)
				{
					const auto& org = pop->GetIndividuals()[id];

					avg_fit += org.Fitness;
					max_fit = max(max_fit, org.Fitness);
					avg_eaten += org.GetState<OrgState>()->EatenCount;
				}
				avg_fit /= species.IndividualsIDs.size();
				avg_eaten /= species.IndividualsIDs.size();

				// TODO : Add the avg fitness of the 5 best
				cout << "    "
					<< " Fitness (avg) : " << avg_fit
					<< endl;
				cout << "    "
					<< " Fitness (max) : " << max_fit
					<< endl;
				cout << "    "
					<< " Avg Eaten : " << avg_eaten
					<< endl;

				cout << "------------------------------" << endl;
			}
         
			cout << endl;
        },true);

    }
    pop->EvaluatePopulation();
    pop->CurrentSpeciesToRegistry();

    SRegistry registry(pop.get());
    registry.save(SerializationFile);

	return pop;
}