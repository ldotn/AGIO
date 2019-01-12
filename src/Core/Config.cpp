#include "Config.h"

using namespace agio;

void Settings::LoadFromFile(const ConfigLoader& CFG)
{
	CFG.LoadValue(Settings::MinSpeciesAge,"MinSpeciesAge");
	CFG.LoadValue(Settings::ProgressMetricsIndividuals,"ProgressMetricsIndividuals");
	CFG.LoadValue(Settings::ProgressMetricsFalloff,"ProgressMetricsFalloff");
	CFG.LoadValue(Settings::ProgressThreshold,"ProgressThreshold");
	CFG.LoadValue(Settings::SpeciesStagnancyChances,"SpeciesStagnancyChances");
	CFG.LoadValue(Settings::MinIndividualsPerSpecies,"MinIndividualsPerSpecies");
	CFG.LoadValue(Settings::MorphologyTries,"MorphologyTries");
	CFG.LoadValue(Settings::SimulationReplications,"SimulationReplications");
	CFG.LoadValue(Settings::ParameterMutationProb,"ParameterMutationProb");
	CFG.LoadValue(Settings::ParameterDestructiveMutationProb,"ParameterDestructiveMutationProb");
	CFG.LoadValue(Settings::ParameterMutationSpread,"ParameterMutationSpread");
}