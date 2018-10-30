#include "Config.h"

using namespace agio;

void Settings::LoadFromFile(const std::string& FilePath)
{
	ConfigLoader cfg(FilePath);

	cfg.LoadValue(Settings::MinSpeciesAge,"MinSpeciesAge");
	cfg.LoadValue(Settings::ProgressMetricsIndividuals,"ProgressMetricsIndividuals");
	cfg.LoadValue(Settings::ProgressMetricsFalloff,"ProgressMetricsFalloff");
	cfg.LoadValue(Settings::ProgressThreshold,"ProgressThreshold");
	cfg.LoadValue(Settings::SpeciesStagnancyChances,"SpeciesStagnancyChances");
	cfg.LoadValue(Settings::MinIndividualsPerSpecies,"MinIndividualsPerSpecies");
	cfg.LoadValue(Settings::MorphologyTries,"MorphologyTries");
	cfg.LoadValue(Settings::SimulationReplications,"SimulationReplications");
	cfg.LoadValue(Settings::ParameterMutationProb,"ParameterMutationProb");
	cfg.LoadValue(Settings::ParameterDestructiveMutationProb,"ParameterDestructiveMutationProb");
	cfg.LoadValue(Settings::ParameterMutationSpread,"ParameterMutationSpread");
}