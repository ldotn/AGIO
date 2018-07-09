#include <iostream>
#include <matplotlibcpp.h>
#include <algorithm>
#include <random>
#include <zip.h>
#include <enumerate.h>

// NEAT
#include "neat.h"
#include "network.h"
#include "population.h"
#include "organism.h"
#include "genome.h"
#include "species.h"

namespace plt = matplotlibcpp;
using namespace std;
using namespace fpp;

const int WorldSizeX = 20;
const int WorldSizeY = 20;

// Need to separate them for the plot
const int FoodCount = 60;
const int HarmCount = 60;
vector<int> food_x(FoodCount);
vector<int> food_y(FoodCount);
vector<int> harm_x(HarmCount);
vector<int> harm_y(HarmCount);

enum CellType { EmptyCell = 0, FoodCell, HarmCell,  };
CellType World[WorldSizeX][WorldSizeY] = {};
mt19937 rng(42);

const int BaseLife = 500;

struct Individual
{
	float Life = BaseLife;
	float AccumulatedLife = 0;
	int PosX = WorldSizeX / 2;
	int PosY = WorldSizeY / 2;

	void Step()
	{
		if (Life <= 0) return;
		// Simple solution to the bounds issue
		//	Kill everyone that steps out of the board!
		// Also, because I added the current position to the sensors, they know where they are
		// so stepping out of the board is something they can prevent.
		// Because it's something they can prevent, they get a 25% penalty to AccumulatedLife
		if (PosX < 1 || PosX > WorldSizeX - 2 || PosY < 1 || PosY > WorldSizeY - 2)
		{
			Life = 0;
			AccumulatedLife -= 0.25f*AccumulatedLife;
			return;
		}

		// Using as input the angle and the squared distance to both the closest food and the closest harm
		// Also having as input the current position and the nearby cells
		struct
		{
			double dist_food;
			double dist_harm;
			double angle_food;
			double angle_harm;
			double pos_x;
			double pos_y;
			double nearby_cells[4];
		} sensors;

		// Load food sensors
		float min_d = numeric_limits<float>::max();
		for (auto [x, y] : zip(food_x, food_y))
		{
			if (x == PosX && y == PosY)
				continue;

			float d = (x - PosX)*(x - PosX) + (y - PosY)*(y - PosY);
			if (d < min_d)
			{
				min_d = d;
				sensors.dist_food = d;
				
				// Compute "angle", taking as 0 = looking up ([0,1])
				// The idea is
				//	angle = normalize(V - Pos) . [0,1]
				float offset_x = x - PosX;
				float offset_y = y - PosY;
				sensors.angle_food = offset_y / sqrtf(offset_x * offset_x + offset_y * offset_y);
			}
		}

		// Load harm sensors
		min_d = numeric_limits<float>::max();
		for (auto [x, y] : zip(harm_x, harm_y))
		{
			if (x == PosX && y == PosY)
				continue;

			float d = (x - PosX)*(x - PosX) + (y - PosY)*(y - PosY);
			if (d < min_d)
			{
				min_d = d;
				sensors.dist_harm = d;

				// Compute "angle", taking as 0 = looking up ([0,1])
				// The idea is
				//	angle = normalize(V - Pos) . [0,1]
				float offset_x = x - PosX;
				float offset_y = y - PosY;
				sensors.angle_harm = offset_y / sqrtf(offset_x * offset_x + offset_y * offset_y);
			}
		}

		sensors.pos_x = PosX;
		sensors.pos_y = PosY;

		sensors.nearby_cells[0] = World[PosX][PosY + 1];
		sensors.nearby_cells[1] = World[PosX][PosY - 1];
		sensors.nearby_cells[2] = World[PosX + 1][PosY];
		sensors.nearby_cells[3] = World[PosY - 1][PosY];

		// Load them into the network and compute output
		Brain->load_sensors((double*)&sensors);

		bool success = Brain->activate();

		// Select the action to do using the activations as probabilities
		// Need to check that all the activations are not 0
		double act_sum = 0;
		double activations[4];
		for (auto [idx, v] : enumerate(Brain->outputs))
			act_sum += activations[idx] = clamp(v->activation,-1.0,1.0)*0.5 + 0.5;//max(0.0,v->activation);

		int action;
		if (act_sum > 1e-6)
		{
			discrete_distribution<int> action_dist(begin(activations), end(activations));
			action = action_dist(rng);
		}
		else
		{
			// Can't decide on an action because all activations are 0
			// Select one action at random
			uniform_int_distribution<int> action_dist(0, 3);
			action = action_dist(rng);
		}

		// Execute action
		if (action == 0)
			PosX++;
		else if (action == 1)
			PosX--;
		else if (action == 2)
			PosY++;
		else if (action == 3)
			PosY--;
		
		// Update life based on current cell
		switch (World[PosX][PosY])
		{
		case FoodCell:
			Life += 7;
			break;
		case HarmCell:
			Life -= 50;
			break;
		}

		// Loose some fixed amount of life each turn
		Life -= 8;

		// Finally accumulate life
		AccumulatedLife += Life;
	}

	NEAT::Network * Brain = nullptr;
};

void BuildWorld()
{
	for (auto& x : food_x)
		x = uniform_int_distribution<>(0, WorldSizeX - 1)(rng);
	for (auto& y : food_y)
		y = uniform_int_distribution<>(0, WorldSizeY - 1)(rng);
	for (auto& x : harm_x)
		x = uniform_int_distribution<>(0, WorldSizeX - 1)(rng);
	for (auto& y : harm_y)
		y = uniform_int_distribution<>(0, WorldSizeY - 1)(rng);

	memset(World, EmptyCell, sizeof(CellType)*WorldSizeX*WorldSizeY);

	for (auto [x, y] : zip(food_x, food_y))
		World[x][y] = FoodCell;
	for (auto [x, y] : zip(harm_x, harm_x))
		World[x][y] = HarmCell;
}

float EvaluteNEATOrganism(NEAT::Organism * Org)
{
	Individual tmp_org;
	tmp_org.Brain = Org->net;

	const int NumberOfRuns = 20;
	float runs[NumberOfRuns];
	
	// Do multiple runs, and return the median
	// This way outliers are eliminated
	for (int j = 0; j < NumberOfRuns; j++)
	{
		BuildWorld();

		tmp_org.Life = BaseLife;
		tmp_org.AccumulatedLife = 0;
		tmp_org.Brain = Org->net;
		tmp_org.Brain->flush();
		tmp_org.PosX = WorldSizeX / 2;
		tmp_org.PosY = WorldSizeY / 2;

		const int max_iters = 1000;
		for (int i = 0; i < max_iters; i++)
		{
			if (tmp_org.Life <= 0) break;

			tmp_org.Step();
		}

		runs[j] = tmp_org.AccumulatedLife;
	}
	
	sort(begin(runs), end(runs));

	return runs[NumberOfRuns / 2];
}

// Writes a .dot file of the provided network
// For visualization purposes
void DumpNetworkToDot(string Filename,NEAT::Network* Net)
{
	ofstream file(Filename);

	file << "digraph network" << endl;
	file << "{" << endl;

	// Write nodes
	//	Sensors are a box, neurons a circle, and output neurons a triangle
	for (auto node : Net->genotype->nodes)
	{
		file << node->node_id;
		if (node->type == NEAT::SENSOR)
			file << " [shape=box]";
		else
		{
			// Search if the node is on the outputs
			for (auto out : Net->outputs)
			{
				if (out->node_id == node->node_id)
				{
					file << " [shape=invtriangle]";
					break;
				}
			}
		}
		file << ";" << endl;
	}

	// Write links
	for (auto gene : Net->genotype->genes)
	{
		file << gene->lnk->in_node->node_id
			 << "->"
			 << gene->lnk->out_node->node_id
			 << ";" << endl;
	}

	// Make all sensors be on the same line, and all outputs be on the same line too
	file << "{ rank=same; ";
	for (auto output : Net->outputs)
		file << output->node_id << " ";
	file << " }" << endl;

	file << "{ rank=same; ";
	for (auto output : Net->inputs)
		file << output->node_id << " ";
	file << " }" << endl;

	file << "}" << endl;
}

int main()
{	
	// Load NEAT parameters
	NEAT::load_neat_params("../NEAT/pole2_markov.ne");

	// Create population
	// 10 inputs, 4 outputs
	auto start_genome = new NEAT::Genome(10, 4, 0, 0);
	auto pop = new NEAT::Population(start_genome, NEAT::pop_size);

	// Run evolution
	for (int i = 0;i < 100;i++)
	{
		// Evaluate organisms
		for (auto& org : pop->organisms)
			org->fitness = EvaluteNEATOrganism(org);
		
		// Finally turn to the next generation
		pop->epoch(i + 1);
	}

	// Simulate and plot positions for the best organism
	plt::ion();

	double best_fitness = 0;
	Individual best_org;
	for (auto org : pop->organisms)
	{
		org->fitness = EvaluteNEATOrganism(org);

		if (org->fitness >= best_fitness)
		{
			best_fitness = org->fitness;
			best_org.Brain = org->net;
		}
	}

	DumpNetworkToDot("best.dot",best_org.Brain);

	while (true)
	{
		BuildWorld();

		best_org.Life = BaseLife;
		best_org.AccumulatedLife = 0;
		best_org.PosX = WorldSizeX / 2;
		best_org.PosY = WorldSizeY / 2;
		best_org.Brain->flush();

		while (best_org.Life > 0)
		{
			//BuildWorld();

			best_org.Step();
			cout << best_org.Life << " " << best_org.AccumulatedLife << endl;

			plt::clf();

			plt::plot(food_x, food_y, "bo");
			plt::plot(harm_x, harm_y, "ro");
			plt::plot({ (float)best_org.PosX }, { (float)best_org.PosY }, "xk");

			plt::xlim(0, WorldSizeX - 1);
			plt::ylim(0, WorldSizeY - 1);

			plt::pause(0.01);
		}
	}

	// TODO : CLEAR EVERYTHING HERE!

	return 0;
}