#include "Population.h"
#include "enumerate.h"
#include "Globals.h"
#include "../Core/Config.h"
#include <numeric>
#include <assert.h>
#include <unordered_set>
#include <queue>



// REMOVE!
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

void Population::Epoch(void * WorldPtr, std::function<void(int)> EpochCallback)
{
	// First version : The species are fixed and created at the start. Also no parameters

	// Evaluate the entire population
	EvaluatePopulation(WorldPtr);

	// Call the callback
	EpochCallback(CurrentGeneration);

	// Advance generation
	CurrentGeneration++;
	
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
		for (int i = 0; i < 5; i++)
		{
			avg_fit += fitness_queue.top();
			fitness_queue.pop();
		}
		avg_fit /= 5.0f;

		// TODO : expose this param
		float falloff = 0.05f;
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
		pop->epoch(CurrentGeneration);

		// Now replace the individuals with the new brains genomes
		// At this point the old pointers are probably invalid, but is NEAT the one that manages that
		for (auto [idx, org_idx] : enumerate(s.IndividualsIDs))
		{
			auto target_genome = pop->organisms[idx % pop->organisms.size()]->gnome;

			auto& org = Individuals[org_idx];

			org.Genome = target_genome;
			org.Brain = org.Genome->genesis(org.Genome->genome_id);

			// On the population creation, the parameters were set from agio -> neat
			// Now the parameters have been evolved by neat, so do the inverse process
			org.Parameters = org.Genome->MorphParams;
		}

		s.Age++;
	}
}

void Population::EvaluatePopulation(void * WorldPtr)
{
	for (auto& org : Individuals)
		org.AccumulatedFitness = 0;

	// Simulate in batches of SimulationSize
	// TODO : This is wrong, you need to take into account the proportion of each species when selecting what to simulate
	// or maybe not, if the individuals are uniformly distributed it should work 
	for (int j = 0; j < Individuals.size() / SimulationSize; j++)
	{
		// TODO : Optimize this, you could pass the current batch id and make the individuals aware of their index in the vector
		for (auto[idx, org] : enumerate(Individuals))
		{
			if (idx >= j * SimulationSize && idx < (j + 1)*SimulationSize)
				org.InSimulation = true;
			else
				org.InSimulation = false;
		}

		for (int i = 0; i < Settings::SimulationReplications; i++)
		{
			// Compute fitness of each individual
			Interface->ComputeFitness(this, WorldPtr);

			// Update the fitness and novelty accumulators
			for (auto& org : Individuals)
				org.AccumulatedFitness += org.Fitness;
		}
	}

	for (auto& org : Individuals)
		org.Fitness = org.AccumulatedFitness / (float)Settings::SimulationReplications;
};

#if 0
Population::ProgressMetrics Population::ComputeProgressMetrics(void * World)
{
	// TODO : For some reason this function is affecting the results of neat!
	return {};

	ProgressMetrics metrics;

	// Compute average novelty
	ComputeNovelty();
	metrics.AverageNovelty = 0;
	for (const auto& org : Individuals)
		metrics.AverageNovelty += org.LastNoveltyMetric;
	metrics.AverageNovelty /= Individuals.size();

	// Do a few simulations to compute fitness
	EvaluatePopulation(World);

	// Backup the evaluated fitness value
	for (auto &org : Individuals)
		org.BackupFitness = org.Fitness;

	for (auto &org : Individuals)
		metrics.AverageFitness += org.Fitness;
	metrics.AverageFitness /= Individuals.size();

	// Now for each species set that to be random, and do a couple of simulations
	metrics.ProgressMetric = 0;
	metrics.MaxFitnessDifference = -numeric_limits<float>::max();
	metrics.MinFitnessDifference = numeric_limits<float>::max();
	for (const auto&[_, species] : SpeciesMap)
	{
		// First compute average fitness of the species when using the network
		// Only consider the 5 best

		float avg_f = 0;
		priority_queue<float> fitness_queue;

		// TODO : Move this to the config file
		const int QueueSize = 5;

		for (int org_id : species.IndividualsIDs)
		{
			fitness_queue.push(-Individuals[org_id].Fitness); // using - because the priority queue is from high to low
			while (fitness_queue.size() > QueueSize)
				fitness_queue.pop();
		}

		float queue_size = fitness_queue.size();
		assert(queue_size == QueueSize);
		while (!fitness_queue.empty())
		{
			avg_f += -fitness_queue.top();
			fitness_queue.pop();
		}
		avg_f /= queue_size;

			//avg_f += Individuals[org_id].Fitness;
		//avg_f /= species.IndividualsIDs.size();

		// Override the use of the network for decision-making
		for (int org_id : species.IndividualsIDs)
			Individuals[org_id].UseNetwork = false;

		// Do a few simulations
		float avg_random_f = 0;
		float count = 0;
		for (int org_id : species.IndividualsIDs)
			Individuals[org_id].AccumulatedFitness = 0;
		
		for (int i = 0; i < Settings::SimulationReplications; i++)
		{
			Interface->ComputeFitness(this, World);

			for (int org_id : species.IndividualsIDs)
				Individuals[org_id].AccumulatedFitness += Individuals[org_id].Fitness / (float)Settings::SimulationReplications;
		}

		for (int org_id : species.IndividualsIDs)
		{
			fitness_queue.push(-Individuals[org_id].AccumulatedFitness); // using - because the priority queue is from high to low
			while (fitness_queue.size() > QueueSize)
				fitness_queue.pop();
		}

		queue_size = fitness_queue.size();
		assert(queue_size == QueueSize);
		while (!fitness_queue.empty())
		{
			avg_random_f += -fitness_queue.top();
			fitness_queue.pop();
		}
		avg_random_f /= queue_size;


		// Accumulate difference
		const float epsilon = 1e-6;
		metrics.ProgressMetric += avg_f - avg_random_f;
		metrics.MaxFitnessDifference = max(metrics.MaxFitnessDifference, avg_f - avg_random_f);
		metrics.MinFitnessDifference = min(metrics.MinFitnessDifference, avg_f - avg_random_f);

		//metrics.AverageFitnessDifference += 100.0f * (avg_f - avg_random_f) / (fabsf(avg_random_f) + 1.0f);
		//metrics.MaxFitnessDifference = max(metrics.MaxFitnessDifference, 100.0f * (avg_f - avg_random_f) / (fabsf(avg_random_f) + 1.0f));
		//metrics.MinFitnessDifference = min(metrics.MinFitnessDifference, 100.0f * (avg_f - avg_random_f) / (fabsf(avg_random_f) + 1.0f));

		// Re-enable the network
		for (int org_id : species.IndividualsIDs)
			Individuals[org_id].UseNetwork = true;
	}
	metrics.ProgressMetric /= SpeciesMap.size();

	// Restore the old fitness values
	for (auto &org : Individuals)
		org.Fitness = org.BackupFitness;

	return metrics;
}
#endif