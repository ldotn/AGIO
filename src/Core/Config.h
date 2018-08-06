#pragma once

namespace agio
{
	struct Settings
	{
		Settings() = delete;

		// Loads the settings from a cfg file
		static void LoadFromFile();
		
		// Controls the spread of the mutation of a parameter when shifting it
		// TODO : better docs?
		inline static float ParameterMutationSpread = 0.025f;

		// Number of nearest individuals to consider for the novelty metric
		inline static int NoveltyNearestK = 5;

		// Minimum novelty of an individual to add it to the morphology registry
		// TODO : No idea what's the range of this. Maybe it can be automatically adjusted from the prev novelty?
		inline static float NoveltyThreshold = 4; 

		// Probability of calling Mutate() on a child
		// TODO : Docs
		inline static float ChildMutationProb = 0.1f;
	};
}