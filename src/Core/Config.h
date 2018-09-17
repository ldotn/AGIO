#pragma once

namespace agio
{
    struct Settings
    {
        Settings() = delete;

        // Loads the settings from a cfg file
        static void LoadFromFile();

        // Number of nearest individuals to consider for the novelty metric
		inline static int NoveltyNearestK = 10;

		// Number of nearest individuals inside the species to consider for the local competition
		inline static int LocalCompetitionK = 15;

		// Minimum age before a species is considered to be removed
		inline static int MinSpeciesAge = 20;

		// Number of individuals to consider when computing progress metrics
		inline static int ProgressMetricsIndividuals = 5;

		// Controls how smooth is the interpolation when computing the progress metric
		// Lower = more smooth
		inline static float ProgressMetricsFalloff = 0.025f;

		// Minimum progress that an species has to be making. 
		// Less than this is considered to be stuck and it may be removed if it keeps stuck
		inline static float ProgressThreshold = 0.01f;

		// Number of consecutive epochs that an species must have a progress lower than the threshold to be removed
		inline static int SpeciesStagnancyChances = 10;

        // Minimum novelty of an individual to add it to the morphology registry
        // TODO : No idea what's the range of this. Maybe it can be automatically adjusted from the prev novelty?
        inline static float NoveltyThreshold = 2.0;

		// Probability of cloning an individual from the registry
		inline static float RegistryCloneProb = 0.1f;

		// Probability of selecting an individual from the registry as parent instead of one of the current population
		inline static float RegistryParentProb = 0.25f;

		// Minimum number of individuals allowed on a species
		inline static int MinIndividualsPerSpecies = 50;

		// Number of times to try when creating a new morphology
		inline static int MorphologyTries = 10;

		// Number of replications to do when simulating
		inline static int SimulationReplications = 10;

        // Probability of calling Mutate() on a child
        // TODO : Docs
		inline static float ChildMutationProb = 0.25f;
        inline static float ComponentMutationProb = 0.00f;

        // ComponentAddProbability + ComponentRemoveProbability + ComponentChangeProbability must sum 1
        inline static float ComponentAddProbability = 0.3f;
        inline static float ComponentRemoveProbability = 0.3f;
        inline static float ComponentChangeProbability = 0.3f;
        inline static float ParameterMutationProb = 0.1f;
        inline static float ParameterDestructiveMutationProb = 0.1f;
		// Controls the spread of the mutation of a parameter when shifting it
		// TODO : better docs?
        inline static float ParameterMutationSpread = 0.025f;

		// TODO : Refactor this so names are consistent
        struct NEAT
        {
            inline static float MutateAddNodeProb = 0.03;
            inline static float MutateAddLinkProb = 0.05;
            inline static float MutateRandomTraitProb = 0.1;
            inline static float MutateLinkTraitProb = 0.1;
            inline static float MutateNodeTraitProb = 0.1;
            inline static float MutateLinkWeightsProb = 0.9;
            inline static float MutateToggleEnableProb = 0;
            inline static float MutateGeneReenableProb = 0;
            inline static float WeightMutPower = 2.5;

        };

    };
}