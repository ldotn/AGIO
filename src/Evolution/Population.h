#pragma once
#include "Individual.h"
#include <unordered_map>

namespace agio
{
	class Population
	{
	public:
		// Creates a new population of random individuals
		// It also separates them into species
		void Spawn(size_t Size);

		 // Computes a single evolutive step
		void Epoch(class World *);

		const auto& GetIndividuals() { return Individuals; }
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
		// Map from the morphology tag (that's what separates species) to the species
		std::unordered_map<Individual::MorphologyTag,Species> SpeciesMap;

		// Used to keep track of the different morphologies that were tried
		// It's only considering the actions and sensors, not the whole components and parameters
		// Information is lost that way, but shouldn't be necessary
		//  and that should help to reduce dimensionality anyway
		// This is Novelty Search basically
	public:
		struct MorphologyRecord
		{
			// The generation at which this morphology first appeared
			int GenerationNumber;

			// The average fitness across organisms and generations
			// It's a moving average across generations
			float AverageFitness;
			
			// The historic best individual representative of this morphology
			Individual Representative;
		};
	private:
		std::unordered_map<Individual::MorphologyTag, MorphologyRecord> MorphologyRegistry;
	
		// Buffer vectors
		std::vector<float> NoveltyBuffer;
		std::vector<float> NearestKBuffer;
	};
}