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

        // Minimum novelty of an individual to add it to the morphology registry
        // TODO : No idea what's the range of this. Maybe it can be automatically adjusted from the prev novelty?
        inline static float NoveltyThreshold = 1.5;

		// Probability of cloning an individual from the registry
		inline static float RegistryCloneProb = 0.1f;

		// Probability of selecting an individual from the registry as parent instead of one of the current population
		inline static float RegistryParentProb = 0.25f;

        // Probability of calling Mutate() on a child
        // TODO : Docs
        inline static float ChildMutationProb = 0.75f;
        inline static float ParameterMutationProb = 0.5f;
        inline static float ParameterDestructiveMutationProb = 0.2f;
		// Controls the spread of the mutation of a parameter when shifting it
		// TODO : better docs?
        inline static float ParameterMutationSpread = 0.025f;

		// TODO : Refactor this so names are consistent
        struct NEAT
        {
            inline static float mutate_add_node_prob = 0.03;
            inline static float mutate_add_link_prob = 0.05;
            inline static float mutate_random_trait_prob = 0.1;
            inline static float mutate_link_trait_prob = 0.1;
            inline static float mutate_node_trait_prob = 0.1;
            inline static float mutate_link_weights_prob = 0.9;
            inline static float mutate_toggle_enable_prob = 0;
            inline static float mutate_gene_reenable_prob = 0;
            inline static float weight_mut_power = 2.5;

        };

    };
}