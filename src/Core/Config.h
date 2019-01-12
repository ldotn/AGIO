#pragma once
#include "ConfigLoader.h"
#include <string>

namespace agio
{
    struct Settings
    {
        Settings() = delete;

        // Loads the settings from a cfg file
        static void LoadFromFile(const ConfigLoader& CFGFile);

		// Minimum age before a species is considered to be removed
		inline static int MinSpeciesAge = 20;

		// Number of individuals to consider when computing progress metrics
		inline static int ProgressMetricsIndividuals = 5;

		// Controls how smooth is the interpolation when computing the progress metric
		// Lower = more smooth
		inline static float ProgressMetricsFalloff = 0.025f;

		// Minimum progress that an species has to be making. 
		// Less than this is considered to be stuck and it may be removed if it keeps stuck
		inline static float ProgressThreshold = 0.005f;

		// Number of consecutive epochs that an species must have a progress lower than the threshold to be removed
		inline static int SpeciesStagnancyChances = 1e10;//10;

		// Minimum number of individuals allowed on a species
		inline static int MinIndividualsPerSpecies = 50;

		// Number of times to try when creating a new morphology
		inline static int MorphologyTries = 10;

		// Number of replications to do when simulating
		inline static int SimulationReplications = 10;

        inline static float ParameterMutationProb = 0.1f;
        inline static float ParameterDestructiveMutationProb = 0.1f;

		// Controls the spread of the mutation of a parameter when shifting it
		// TODO : better docs?
        inline static float ParameterMutationSpread = 0.025f;
    };
}