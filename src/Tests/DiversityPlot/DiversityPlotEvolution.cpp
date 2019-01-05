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

    for (int g = 0; g < GenerationsCount; g++)
    {
        pop.Epoch(&world, [&](int gen)
        {
            cout << "Generation " << gen 
				<< ", " << pop.GetSpecies().size() << " Species" << endl;

			cout << "------------------------------" << endl;
			for (const auto& [_, species] : pop.GetSpecies())
			{
				cout << "    "
					 << " Size : " << species.IndividualsIDs.size()
				     << endl;
				cout << "    "
					 << " Progress : " << species.ProgressMetric
				     << endl;

				float avg_fit = 0;
				float max_fit = numeric_limits<float>::lowest();
				for (int id : species.IndividualsIDs)
				{
					const auto& org = pop.GetIndividuals()[id];

					avg_fit += org.Fitness;
					max_fit = max(max_fit, org.Fitness);
				}
				avg_fit /= species.IndividualsIDs.size();

				// TODO : Add the avg fitness of the 5 best
				cout << "    "
					<< " Fitness (avg) : " << avg_fit
					<< endl;
				cout << "    "
					<< " Fitness (max) : " << max_fit
					<< endl;
				cout << "------------------------------" << endl;
			}
         
			cout << endl;
        },true);

    }
    pop.EvaluatePopulation(&world);
    pop.CurrentSpeciesToRegistry();

    SRegistry registry(&pop);
    registry.save(SerializationFile);
}