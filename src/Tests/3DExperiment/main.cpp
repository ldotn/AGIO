#include "3DExperiment.h"
#include <matplotlibcpp.h>

namespace plt = matplotlibcpp;

// Does the evolution loop and saves the evolved networks to a file

int main()
{
	NEAT::load_neat_params("../cfg/neat_test_config.txt");
	NEAT::mutate_morph_param_prob = Settings::ParameterMutationProb;
	NEAT::destructive_mutate_morph_param_prob = Settings::ParameterDestructiveMutationProb;
	NEAT::mutate_morph_param_spread = Settings::ParameterMutationSpread;

	// Create base interface
	Interface = new AGIOInterface();
	Interface->Init();

	return 0;
}