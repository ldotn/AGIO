#include "Population.h"
#include "enumerate.h"
#include "Globals.h"
#include "../Core/Config.h"
#include <numeric>
#include <assert.h>
#include <unordered_set>
#include <queue>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <iostream>

// NEAT
#include "innovation.h"
#include "genome.h"
#include "../NEAT/include/population.h"

using namespace std;
using namespace agio;
using namespace fpp;

Population::Population() : RNG(std::chrono::high_resolution_clock::now().time_since_epoch().count())
{
	cur_node_id = 0;
	cur_innov_num = 0;
	CurrentGeneration = 0;
}

MorphologyTag Population::MakeRandomMorphology()
{
	MorphologyTag tag;

	for (auto[gidx, group] : enumerate(Interface->GetComponentRegistry()))
	{
		int components_count = uniform_int_distribution<int>(group.MinCardinality, group.MaxCardinality)(RNG);

		vector<int> index_vec(group.Components.size());
		for (auto[idx, v] : enumerate(index_vec)) v = idx;

		shuffle(index_vec.begin(), index_vec.end(), RNG);

		for (int i = 0; i < components_count; i++)
			tag.push_back({ (int)gidx, index_vec[i] });
	}

	return tag;
}

void Population::Spawn(int SizeMult,int SimSize)
{
	int pop_size = SizeMult * SimSize;

	SimulationSize = SimSize;
	Individuals.reserve(pop_size);

	// The loop is finished when no new species can be found
	//   or when the pop size / (species count + 1)  < Min Species Individuals
	bool finished = false;
	while (!finished)
	{
		// Check if we have enough species
		if (float(pop_size) / float(SpeciesMap.size() + 1) <= Settings::MinIndividualsPerSpecies)
			break;

		// Create a new random morphology
		auto tag = MakeRandomMorphology();
		int tries = 0;
		while (SpeciesMap.find(tag) != SpeciesMap.end())
		{
			tries++;
			if (tries > Settings::MorphologyTries)
			{
				finished = true;
				break;
			}

			tag = MakeRandomMorphology();
		}

		// Now that a species has been found, insert it into the map
		SpeciesMap[tag] = Species();
	}

	// Now assign individuals to the species and create NEAT populations
	int individuals_per_species = pop_size / SpeciesMap.size(); // int divide
	for (auto & [tag, s] : SpeciesMap)
	{
		unordered_set<int> actions_set;
		unordered_set<int> sensors_set;

		for (auto [gidx,cidx] : tag)
		{
			const auto &component = Interface->GetComponentRegistry()[gidx].Components[cidx];

			actions_set.insert(component.Actions.begin(), component.Actions.end());
			sensors_set.insert(component.Sensors.begin(), component.Sensors.end());
		}
		
		vector<int> actions, sensors;

		// Compute action and sensors vectors
		actions.resize(actions_set.size());
		for (auto[idx, action] : enumerate(actions_set))
			actions[idx] = action;
		
		sensors.resize(sensors_set.size());
		for (auto[idx, sensor] : enumerate(sensors_set))
			sensors[idx] = sensor;

		// Find the number of individuals that will be put on this species
		int size = min(individuals_per_species, pop_size - (int)Individuals.size());

		// Create NEAT population
		auto start_genome = new NEAT::Genome(sensors.size(), actions.size(), 0, 0);
		s.NetworksPopulation = new NEAT::Population(start_genome, size);

		// Initialize individuals
		for (int i = 0; i < size; i++)
		{
			Individual org;
			org.Morphology = tag;
			org.Genome = s.NetworksPopulation->organisms[i]->gnome;
			org.Brain = org.Genome->genesis(org.Genome->genome_id);

			org.Actions = actions;
			org.Sensors = sensors;

			org.ActivationsBuffer.resize(org.Actions.size());
			org.SensorsValues.resize(org.Sensors.size());

			for (auto [pidx, param] : enumerate(Interface->GetParameterDefRegistry()))
			{
				// If the parameter is not required, select it randomly, 50/50 chance
				// TODO : Maybe make the selection probability a parameter? Does it makes sense?
				if (param.IsRequired || uniform_int_distribution<int>(0, 1)(RNG))
				{
					Parameter p{};
					p.ID = pidx;
					p.Value = 0.5f * (param.Min + param.Max);
					p.HistoricalMarker = Parameter::CurrentMarkerID;
					p.Min = param.Min;
					p.Max = param.Max;

					// Small shift
					float shift = normal_distribution<float>(0, Settings::ParameterMutationSpread)(RNG);
					p.Value += shift * abs(param.Max - param.Min);

					// Clamp values
					p.Value = clamp(p.Value, param.Min, param.Max);

					org.Parameters[param.UserID] = p;
				}
			}

			// Set morphological parameters of the genome
			// Make a copy
			org.Genome->MorphParams = org.GetParameters();
			org.State = Interface->MakeState(&org);
			Individuals.push_back(move(org));
		}
	}

	// Now shuffle individuals vector
	shuffle(Individuals.begin(), Individuals.end(),RNG);

	// And finally find the id's of the individuals for each species
	for (auto & [tag, s] : SpeciesMap)
	{
		for (auto [idx, org] : enumerate(Individuals))
		{
			if (org.Morphology == tag)
				s.IndividualsIDs.push_back(idx);
		}
	}

	CurrentGeneration = 0;

	// TODO :  Separate organisms by distance in the world too
}

void Population::Epoch(void * WorldPtr, std::function<void(int)> EpochCallback, bool MuteNEAT)
{
	// First version : The species are fixed and created at the start. Also no parameters

	// Evaluate the entire population
	EvaluatePopulation(WorldPtr);

	// Call the callback
	EpochCallback(CurrentGeneration);

	// Advance generation
	CurrentGeneration++;
	
	// [https://stackoverflow.com/questions/30184998/how-to-disable-cout-output-in-the-runtime]
	if(MuteNEAT)
		cout.setstate(ios_base::failbit);

	// Now do the epoch
	for (auto & [tag, s] : SpeciesMap)
	{
		// Compute local scores
		/*
		for (int idx : s.IndividualsIDs)
		{
			// Find the K nearest inside the species

			// Clear the K buffer
			for (auto & v : CompetitionNearestKBuffer)
			{
				v.first = -1;
				v.second = numeric_limits<float>::max();
			}
			int k_buffer_top = 0;

			// Check against the species individuals
			for (int other_idx : s.IndividualsIDs)
			{
				if (idx == other_idx)
					continue;

				// This is the morphology distance
				float dist = Individuals[idx].GetMorphologyTag().Distance(Individuals[other_idx].GetMorphologyTag());

				// Check if it's in the k nearest
				for (auto[kidx, v] : enumerate(CompetitionNearestKBuffer))
				{
					if (dist < v.second)
					{
						// Move all the values to the right
						for (int i = Settings::LocalCompetitionK - 1; i > kidx; i--)
							CompetitionNearestKBuffer[i] = CompetitionNearestKBuffer[i - 1];

						if (kidx > k_buffer_top)
							k_buffer_top = kidx;
						v.first = other_idx;
						v.second = dist;
						break;
					}
				}
			}

			// With the k nearest found, check how many of them this individual bests
			auto & org = Individuals[idx];
			org.LocalScore = 0;
			for (int i = 0; i <= k_buffer_top; i++)
			{
				const auto& other_org = Individuals[CompetitionNearestKBuffer[i].first];
				if (org.Fitness > other_org.Fitness)
					org.LocalScore++;
			}
		}
		*/

		// First update the fitness values for neat
		priority_queue<float> fitness_queue;
		//float avg_fit = 0;
		auto& pop = s.NetworksPopulation;
		for (auto& neat_org : pop->organisms)
		{
			// Find the organism in the individuals that has this brain
			for (int org_idx : s.IndividualsIDs)
			{
				const auto& org = Individuals[org_idx];
				if (org.Genome == neat_org->gnome)  // comparing pointers
				{
					//neat_org->fitness = org.Fitness; // Here you could add some contribution of the novelty
					
					// Remapping because NEAT appears to only work with positive fitness
					neat_org->fitness = log2(exp2(0.1*org.Fitness) + 1.0) + 1.0;
					//avg_fit += neat_org->fitness;//org.Fitness;
					//neat_org->fitness = org.LocalScore;

					fitness_queue.push(org.Fitness);

					break;
				}
			}
		}
		float avg_fit = 0;
		for (int i = 0; i < Settings::ProgressMetricsIndividuals; i++)
		{
			avg_fit += fitness_queue.top();
			fitness_queue.pop();
		}
		avg_fit /= (float)Settings::ProgressMetricsIndividuals;

		float inv_age = 1.0f / (s.Age + 1);
		float falloff = (1.0f - inv_age) * Settings::ProgressMetricsFalloff + inv_age;
		float smoothed_fitness = (1.0f - falloff)*s.LastFitness + falloff * avg_fit;
		if (s.LastFitness <= 0)
		{
			smoothed_fitness = avg_fit;
			s.ProgressMetric = 0;
		}
		else
			s.ProgressMetric = (1.0f - falloff)*s.ProgressMetric + falloff*((smoothed_fitness - s.LastFitness) / s.LastFitness);

		s.LastFitness = smoothed_fitness;


		// With the fitness updated call the neat epoch
		s.Age++;
		pop->epoch(s.Age);

		// Now replace the individuals with the new brains genomes
		// At this point the old pointers are probably invalid, but is NEAT the one that manages that
		for (auto [idx, org_idx] : enumerate(s.IndividualsIDs))
		{
			auto target_genome = pop->organisms[idx % pop->organisms.size()]->gnome;

			auto& org = Individuals[org_idx];

			org.Genome = target_genome;
			org.Brain = org.Genome->genesis(org.Genome->genome_id); // TODO : Check what actually IS the genome_id

			// On the population creation, the parameters were set from agio -> neat
			// Now the parameters have been evolved by neat, so do the inverse process
			org.Parameters = org.Genome->MorphParams;
		}
	}

	if (MuteNEAT)
		cout.clear();

	// With the species updated, check if there are stagnant ones
	for(auto iter = SpeciesMap.begin();iter != SpeciesMap.end();)
	{
		auto &[tag, s] = *iter;

		if (s.Age < Settings::MinSpeciesAge)
		{
			++iter; // Ignore young species
			continue;
		}

		if (s.ProgressMetric < Settings::ProgressThreshold)
		{
			s.EpochsUnderThreshold++;
			if (s.EpochsUnderThreshold >= Settings::SpeciesStagnancyChances)
			{
				// Store this species to the registry
				SpeciesRecord entry;
				entry.Age = s.Age;
				entry.Morphology = tag;
				entry.IndividualsSize = s.IndividualsIDs.size();
				entry.LastFitness = s.LastFitness;
				entry.HistoricalBestGenome = s.NetworksPopulation->GetBestGenome()->duplicate(0);
				entry.LastGenomes.resize(s.NetworksPopulation->organisms.size());
				for (auto [idx, org] : enumerate(s.NetworksPopulation->organisms))
					entry.LastGenomes[idx] = org->gnome->duplicate(org->gnome->genome_id);

				StagnantSpecies.push_back(move(entry));

				// Reset the species
				s.Age = 0;
				s.LastFitness = 0;
				s.ProgressMetric = 0;
				s.EpochsUnderThreshold = 0;
				auto old_pop_ptr = s.NetworksPopulation;

				// Check if a new species can be generated
				// There might not be any species left that aren't already on the map
				bool found = true;
				auto new_tag = MakeRandomMorphology();
				int tries = 0;
				while (SpeciesMap.find(new_tag) != SpeciesMap.end())
				{
					tries++;
					if (tries > Settings::MorphologyTries)
					{
						found = false;
						break;
					}

					new_tag = MakeRandomMorphology();
				}

				// If no new topology could be found in N tries, recreate this one
				if (tries >= Settings::MorphologyTries)
				{
					// No new species was found, so re-create this one
					// Similar to the delta-encoding that neat does

					// Create a new population starting with the historical best genome and all the other genomes from this species in the registry
					//		First get all the genomes of the best individuals of all the entries on the registry
					vector<NEAT::Genome*> start_genomes;
					for (auto entry : StagnantSpecies)
						if (entry.Morphology == tag)
							start_genomes.push_back(entry.HistoricalBestGenome);

					//		Always add a new genome that's the base one (no hidden, fully connected)
					
					auto base_genome = new NEAT::Genome(
						Individuals[s.IndividualsIDs[0]].Sensors.size(), 
						Individuals[s.IndividualsIDs[0]].Actions.size(), 0, 0);
					start_genomes.push_back(base_genome);

					//		No need to add the current best genome, it's already on the registry at this point
					//		Then create population from the genome list
					s.NetworksPopulation = new NEAT::Population(s.IndividualsIDs.size(), start_genomes);

					//		After the population has been created, delete the new base genome
					//			Don't delete the other as they are in the registry
					delete base_genome;

					// Important : You need to set the starting parameters by hand
					// NEAT can evolve them, but has no knowledge of the registry, so it can't create them
					for (int i = 0; i < s.IndividualsIDs.size(); i++)
						s.NetworksPopulation->organisms[i]->gnome->MorphParams = old_pop_ptr->GetBestGenome()->MorphParams;

					// Replace the genome pointers of the individuals
					for (auto [num,idx] : enumerate(s.IndividualsIDs))
						Individuals[idx].Genome = s.NetworksPopulation->organisms[num]->gnome;

					// Finally delete the old neat pop
					delete old_pop_ptr;

					cout << "!! RESETED SPECIES !!" << endl;
				}
				else
				{
					// "Steal" the ids of the old individuals for the new species
					// For those objects, you'll need to call the destructors, and create new ones in place
					// Also, instead of erasing and inserting, just replace this species in-place for the new one
					cout << "## NEW SPECIES ##" << endl;

					unordered_set<int> actions_set;
					unordered_set<int> sensors_set;

					for (auto [gidx, cidx] : new_tag)
					{
						const auto &component = Interface->GetComponentRegistry()[gidx].Components[cidx];

						actions_set.insert(component.Actions.begin(), component.Actions.end());
						sensors_set.insert(component.Sensors.begin(), component.Sensors.end());
					}

					vector<int> actions, sensors;

					// Compute action and sensors vectors
					actions.resize(actions_set.size());
					for (auto[idx, action] : enumerate(actions_set))
						actions[idx] = action;

					sensors.resize(sensors_set.size());
					for (auto[idx, sensor] : enumerate(sensors_set))
						sensors[idx] = sensor;

					// Take all the individuals from this species
					int size = s.IndividualsIDs.size();

					// Create NEAT population
					//		First get all the genomes of the best individuals of all the entries on the registry
					vector<NEAT::Genome*> start_genomes;
					for (auto entry : StagnantSpecies)
						if(entry.Morphology == new_tag) 
							start_genomes.push_back(entry.HistoricalBestGenome);

					//		Always add a new genome that's the base one (no hidden, fully connected)
					auto base_genome = new NEAT::Genome(sensors.size(), actions.size(), 0, 0);
					start_genomes.push_back(base_genome);

					//		Then create population from the genome list
					s.NetworksPopulation = new NEAT::Population(size, start_genomes);

					//		After the population has been created, delete the new base genome
					//			Don't delete the other as they are in the registry
					delete base_genome;

					// Initialize individuals
					for (int i = 0; i < size; i++)
					{
						Individual org;
						org.Morphology = tag;
						org.Genome = s.NetworksPopulation->organisms[i]->gnome;
						org.Brain = org.Genome->genesis(org.Genome->genome_id);
						org.Actions = actions;
						org.Sensors = sensors;
						org.ActivationsBuffer.resize(org.Actions.size());
                        org.SensorsValues.resize(org.Sensors.size());

						for (auto[pidx, param] : enumerate(Interface->GetParameterDefRegistry()))
						{
							// If the parameter is not required, select it randomly, 50/50 chance
							// TODO : Maybe make the selection probability a parameter? Does it makes sense?
							if (param.IsRequired || uniform_int_distribution<int>(0, 1)(RNG))
							{
								Parameter p{};
								p.ID = pidx;
								p.Value = 0.5f * (param.Min + param.Max);
								p.HistoricalMarker = Parameter::CurrentMarkerID;
								p.Min = param.Min;
								p.Max = param.Max;

								// Small shift
								float shift = normal_distribution<float>(0, Settings::ParameterMutationSpread)(RNG);
								p.Value += shift * abs(param.Max - param.Min);

								// Clamp values
								p.Value = clamp(p.Value, param.Min, param.Max);

								org.Parameters[param.UserID] = p;
							}
						}

						// Set morphological parameters of the genome
						// Make a copy
						org.Genome->MorphParams = org.GetParameters();
						org.State = Interface->MakeState(&org);

						Individuals[s.IndividualsIDs[i]].~Individual();
						new (&Individuals[s.IndividualsIDs[i]]) Individual(move(org));
					}

					// Change key
					++iter; // Advance before changing the key, because that invalidates the iterator
					/*auto handle = SpeciesMap.extract(tag);
					handle.key() = new_tag;
					SpeciesMap.insert(move(handle));*/
					SpeciesMap[new_tag] = move(s);
					SpeciesMap.erase(tag);

					// Finally delete the old neat pop
					delete old_pop_ptr;

					// Already advanced the iterator, don't advance twice
					continue;
				}
			}
		}
		else s.EpochsUnderThreshold = 0;

		++iter;
	}
}

void Population::EvaluatePopulation(void * WorldPtr)
{
	for (auto& org : Individuals)
		org.AccumulatedFitness = 0;

	// Create pointer vector
	std::vector<class BaseIndividual*> individuals_ptrs;
	individuals_ptrs.reserve(SimulationSize);

	// Simulate in batches of SimulationSize
	// TODO : This is wrong (or not, see below), you need to take into account the proportion of each species when selecting what to simulate
	// or maybe not, if the individuals are uniformly distributed it should work 
	for (int j = 0; j < Individuals.size() / SimulationSize; j++)
	{
		// TODO : Optimize this, you could pass the current batch id and make the individuals aware of their index in the vector
		individuals_ptrs.resize(0);
		for (auto[idx, org] : enumerate(Individuals))
		{
			if (idx >= j * SimulationSize && idx < (j + 1)*SimulationSize)
				org.InSimulation = true;
			else
				org.InSimulation = false;

			if (org.InSimulation)
				individuals_ptrs.push_back(&Individuals[idx]);
		}

		for (int i = 0; i < Settings::SimulationReplications; i++)
		{
			// Compute fitness of each individual
			Interface->ComputeFitness(individuals_ptrs, WorldPtr);

			// Update the fitness and novelty accumulators
			for (auto org_ptr : individuals_ptrs)
				((Individual*)org_ptr)->AccumulatedFitness += ((Individual*)org_ptr)->Fitness;
		}
	}

	for (auto& org : Individuals)
		org.Fitness = org.AccumulatedFitness / (float)Settings::SimulationReplications;
};

void Population::ComputeDevMetrics(void * World)
{
	ProgressMetrics metrics;

	EvaluatePopulation(World);
	for (auto& org : Individuals)
		org.DevMetrics.PrevFitness = org.Fitness;

    // Now for each species set that to be random, and do a couple of simulations
    // Override the use of the network for decision-making
	for (auto &[_, species] : SpeciesMap)
	{
		species.DevMetrics = {};
		vector<pair<float,float>> orgs_f(species.IndividualsIDs.size());

		for (auto [idx,org_id] : enumerate(species.IndividualsIDs))
			Individuals[org_id].UseNetwork = false;

		EvaluatePopulation(World);

		for (auto [idx,org_id] : enumerate(species.IndividualsIDs))
		{
			Individuals[org_id].UseNetwork = true;

			float base_f = Individuals[org_id].DevMetrics.PrevFitness;
			float random_f = Individuals[org_id].Fitness;
			orgs_f[idx] = { base_f,random_f };
		}
			
		sort(orgs_f.begin(), orgs_f.end(), [](auto a, auto b)
		{
			return a.first > b.first;
		});

		for (int i = 0; i < 5 && i < orgs_f.size() - 6; i--)
		{
			auto[base_f, random_f] = orgs_f[i];
			species.DevMetrics.RandomFitness += random_f;
			species.DevMetrics.RealFitness += base_f;
		}
			
		species.DevMetrics.RandomFitness /= min((float)orgs_f.size(), 5.0f);
		species.DevMetrics.RealFitness /= min((float)orgs_f.size(), 5.0f);
	}
}

void Population::CurrentSpeciesToRegistry()
{
	// Before serialization, put best individuals into the registry.
	for (auto &[_,species] : SpeciesMap)
	{
		Individual *bestIndividual = nullptr;
		for (auto &individualId : species.IndividualsIDs)
		{
			Individual &individual = Individuals[individualId];
			if (!bestIndividual || individual.Fitness > bestIndividual->Fitness)
				bestIndividual = &individual;
		}

		if (bestIndividual != nullptr)
		{
			SpeciesRecord entry;
			entry.Age = species.Age;
			entry.Morphology = bestIndividual->GetMorphologyTag();
			entry.LastFitness = bestIndividual->Fitness;
			entry.HistoricalBestGenome = bestIndividual->GetGenome();
			StagnantSpecies.push_back(entry);
		}
	}
}