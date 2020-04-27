#include <iostream>
#include "ComplexSystem.h"
#include "../../Core/Config.h"
#include <string>
#include "../../Utils/Utils.h"
#include "../../Serialization/SIndividual.h"
#include "zip.h"
#include "enumerate.h"
#include <fstream>
#include "../../Serialization/SRegistry.h"
#include "neat.h"
#include "../../Utils/WorkerPool.h"
#include "network.h"
#include "genome.h"

using namespace agio;
using namespace std;
using namespace fpp;

// Need to define them somewhere
int WorldSizeX;
int WorldSizeY;
float FoodScoreGain;
float KillScoreGain;
float DeathPenalty;
float FoodCellProportion;
int MaxSimulationSteps;
int SimulationSize;
int PopSizeMultiplier;
int PopulationSize;
int GenerationsCount;
float LifeLostPerTurn;
float BorderPenalty;
float WastedActionPenalty;
float WaterPenalty;
int InitialLife;
string SerializationFile;

int main(int argc, char *argv[])
{
	minstd_rand RNG(chrono::high_resolution_clock::now().time_since_epoch().count());

	string cfg_path = "../src/Tests/ComplexSystem/Config.cfg";

	ConfigLoader loader(cfg_path);
	WorldSizeX = WorldSizeY = -1;

	loader.LoadValue(FoodScoreGain, "FoodScoreGain");
	loader.LoadValue(KillScoreGain, "KillScoreGain");
	loader.LoadValue(DeathPenalty, "DeathPenalty");
	loader.LoadValue(FoodCellProportion, "FoodProportion");
	loader.LoadValue(MaxSimulationSteps, "MaxSimulationSteps");
	loader.LoadValue(SimulationSize, "SimulationSize");
	loader.LoadValue(PopSizeMultiplier, "PopSizeMultiplier");
	PopulationSize = PopSizeMultiplier * SimulationSize;
	loader.LoadValue(GenerationsCount, "GenerationsCount");
	loader.LoadValue(LifeLostPerTurn, "LifeLostPerTurn");
	loader.LoadValue(BorderPenalty, "BorderPenalty");
	loader.LoadValue(WastedActionPenalty, "WastedActionPenalty");
	loader.LoadValue(WaterPenalty, "WaterPenalty");
	loader.LoadValue(InitialLife, "InitialLife");
	loader.LoadValue(SerializationFile, "SerializationFile");

	Settings::LoadFromFile(loader);

	::NEAT::load_neat_params("../cfg/neat_test_config.txt");
	::NEAT::mutate_morph_param_prob = Settings::ParameterMutationProb;
	::NEAT::destructive_mutate_morph_param_prob = Settings::ParameterDestructiveMutationProb;
	::NEAT::mutate_morph_param_spread = Settings::ParameterMutationSpread;

	Interface = new PublicInterfaceImpl();
	Interface->Init();

	// Evolve
	WorldData world = createWorld("../assets/worlds/world0.txt");
	Population pop(&world, 24);
	pop.Spawn(PopSizeMultiplier, SimulationSize);

	// Increase simulation size to get more species at the same time
	SimulationSize = 30;

	cout << "Doing evolution" << endl;
	for (int g = 0; g < GenerationsCount; g++)
	{
		cout << ".";
		pop.Epoch([](int) {}, true);
	}
	cout << endl;
	// Create pop from registry
	pop.CurrentSpeciesToRegistry();
	SRegistry registry(&pop);

	unordered_map<MorphologyTag, int> species_ids;
	vector<BaseIndividual*> individuals;
	unordered_map<int, OrgType> species_types;

	// Select randomly until the simulation size is filled
	for (int i = 0; i < SimulationSize; i++)
	{
		int species_idx = uniform_int_distribution<int>(0, registry.Species.size() - 1)(RNG);
		int individual_idx = uniform_int_distribution<int>(0, registry.Species[species_idx].Individuals.size() - 1)(RNG);

		SIndividual * org = new SIndividual;
		*org = registry.Species[species_idx].Individuals[individual_idx];

		individuals.push_back(org);

		int sid = species_ids.size();
		species_ids[org->GetMorphologyTag()] = sid;

		// Yes, this makes the same asignment a bunch of times, but I don't care, it's just a test
		if (org->HasAction((int)ActionsIDs::EatFoodEnemy))
			species_types[sid] = OrgType::Omnivore;
		else if (org->HasAction((int)ActionsIDs::EatEnemy))
			species_types[sid] = OrgType::Carnivore;
		else
			species_types[sid] = OrgType::Herbivore;
	}

	for (auto &individual : individuals)
		individual->State = Interface->MakeState(individual);

	// Do a few simulations to capture the average fitness of each species
	auto simulate = [&]()
	{
		vector<float> avg_fit(species_ids.size());
		vector<float> avg_fit_acc(species_ids.size());
		fill(avg_fit.begin(), avg_fit.end(), 0);
		fill(avg_fit_acc.begin(), avg_fit_acc.end(), 0);

		for (int rep = 0; rep < Settings::SimulationReplications; rep++)
		{
			vector<float> fitness(individuals.size());
			fill(fitness.begin(), fitness.end(), 0);

			for (auto& org : individuals)
				org->Reset();

			for (int step = 0; step < MaxSimulationSteps; step++)
			{
				for (auto[org_idx, org] : enumerate(individuals))
				{
					if (!org->GetState<OrgState>()->Enabled) continue;

					auto state_ptr = org->GetState<OrgState>();
					if (state_ptr->Life > 0)
					{
						org->DecideAndExecute(&world, individuals);

						state_ptr->Score += 0.1*state_ptr->Life;
						fitness[org_idx] = state_ptr->Score;
					}
				}
			}

			// Average the fitness of all the individuals for each species
			for (auto[org_idx, org] : enumerate(individuals))
			{
				int sid = species_ids[org->GetMorphologyTag()];
				avg_fit[sid] += fitness[org_idx];
				avg_fit_acc[sid]++;
			}
		}

		for (auto[val, count] : zip(avg_fit, avg_fit_acc))
		{
			if (count == 0)
				val = 0;
			else
				val /= count;
		}

		return avg_fit;
	};


	cout << "Writting species ref file" << endl;
	wofstream outf_sref(L"species_ref.csv");
	for (const auto& [sid, type] : species_types)
	{
		outf_sref << sid << L",";
		if (type == OrgType::Omnivore)
			outf_sref << L"Omnívoro" << endl;
		else if (type == OrgType::Carnivore)
			outf_sref << L"Carnívoro" << endl;
		else if (type == OrgType::Herbivore)
			outf_sref << L"Herbívoro" << endl;
	}
	outf_sref.close();

	cout << "Computing baseline" << endl;
	ofstream outf_base("baseline.csv");
	vector<vector<float>> baseline_data(species_ids.size());

	for (int i = 0; i < 100; i++)
	{
		cout << ".";
		auto data = simulate();
		for (int sid = 0; sid < species_ids.size(); sid++)
			baseline_data[sid].push_back(data[sid]);
	}
	for(const auto& vec : baseline_data)
	{
		for (float x : vec)
			outf_base << x << ",";
		outf_base << endl;
	}
	outf_base.close();

	// Now do this all again, but each time disabling one of the species
	cout << "Computing final results" << endl;
	vector<vector<float>> results;
	for (int sid = 0; sid < species_ids.size(); sid++)
	{
		cout << ".";
		for (auto& org : individuals)
		{
			if (species_ids[org->GetMorphologyTag()] == sid)
				org->GetState<OrgState>()->Enabled = false;
			else
				org->GetState<OrgState>()->Enabled = true;
		}

		results.push_back(simulate());
	}

	// Write results to file
	ofstream outf("interrelations.csv");
	for (int sid = 0;sid < species_ids.size();sid++)
	{
		for (const auto& rvec : results)
			outf << rvec[sid] << ",";
		outf << endl;
	}
	outf.close();
}