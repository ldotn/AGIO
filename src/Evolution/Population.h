#pragma once
#include "Individual.h"

namespace agio
{
	class Population
	{
	public:
		// Creates a new population of random individuals
		// It also separates them into species
		void Spawn(size_t Size);

		 // Computes a single evolutive step
		void Epoch();
	private:
		std::vector<Individual> Individuals;
	
		// Encapsulates the data for a single species
		// TODO : Add whatever additional info is necessary
	public:
		struct Species
		{
			std::vector<int> IndividualsIDs;
		};
	private:
		std::vector<Species> SpeciesVector;

		// Used to keep track of the different morphologies that were tried
		// It's only considering the actions and sensors, not the whole components and parameters
		// Information is lost that way, but shouldn't be necessary
		//  and that should help to reduce dimensionality anyway
		// This is Novelty Search basically
	public:
		struct MorphologyRecord
		{
			int GenerationNumber;
			float AverageFitness;
			Individual::MorphologyTag Morphology;
		};
	private:
		std::vector<MorphologyRecord> MorphologyRegistry;
	};
}