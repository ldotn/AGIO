#define WITHOUT_NUMPY
#include <matplotlibcpp.h>

#include <algorithm>
#include <random>
#include <zip.h>
#include <enumerate.h>
#include <iostream>

#include "WorkerPool.h"

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
const int NumberOfRuns = 100;

struct Individual
{
	float Life = BaseLife;
	float AccumulatedLife = 0;
	int PosX = WorldSizeX / 2;
	int PosY = WorldSizeY / 2;
	NEAT::Network * Brain = nullptr;

	void Reset()
	{
		Life = BaseLife;
		AccumulatedLife = 0;
		PosX = WorldSizeX / 2;
		PosY = WorldSizeY / 2;

		if (Brain) Brain->flush();
	}

	void Step()
	{
		if (Life <= 0) return;

		// Using as input the angle and the squared distance to both the closest food and the closest harm
		// Also having as input the current position and the nearby cells
		struct
		{
			/*double dist_food;
			double dist_harm;*/
			double angle_food;
			double angle_harm;
			/*double pos_x;
			double pos_y;
			double nearby_cells[4];*/
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
				//sensors.dist_food = d;
				
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
				//sensors.dist_harm = d;

				// Compute "angle", taking as 0 = looking up ([0,1])
				// The idea is
				//	angle = normalize(V - Pos) . [0,1]
				float offset_x = x - PosX;
				float offset_y = y - PosY;
				sensors.angle_harm = offset_y / sqrtf(offset_x * offset_x + offset_y * offset_y);
			}
		}

		/*sensors.pos_x = PosX;
		sensors.pos_y = PosY;

		sensors.nearby_cells[0] = World[PosX + 1][PosY    ];
		sensors.nearby_cells[1] = World[PosX - 1][PosY    ];
		sensors.nearby_cells[2] = World[PosX    ][PosY + 1];
		sensors.nearby_cells[3] = World[PosY    ][PosY - 1];*/

		// Load them into the network and compute output
		Brain->load_sensors((double*)&sensors);

		bool success = Brain->activate();

		// Select the action to do using the activations as probabilities
		// Need to check that all the activations are not 0
		double act_sum = 0;
		double activations[4];
		for (auto[idx, v] : enumerate(Brain->outputs))
			act_sum += activations[idx] = v->activation; // The activation function is in [0, 1], check line 461 of neat.cpp;

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

		// Finally do the action
		ExecuteAction(action);
	}

	void ExecuteAction(int Action)
	{
		// Execute action
		if (Action == 0)
			PosX++;
		else if (Action == 1)
			PosX--;
		else if (Action == 2)
			PosY++;
		else if (Action == 3)
			PosY--;

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

		// Update life based on current cell
		switch (World[PosX][PosY])
		{
		case FoodCell:
			Life += 7;
			break;
		case HarmCell:
			Life -= 250;
			break;
		}

		// Loose some fixed amount of life each turn
		Life -= 8;

		// Finally accumulate life
		AccumulatedLife += Life;
	}
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
	for (auto [x, y] : zip(harm_x, harm_y))
		World[x][y] = HarmCell;
}

float EvaluteNEATOrganism(NEAT::Organism * Org)
{
	Individual tmp_org;
	tmp_org.Brain = Org->net;

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

	float median = runs[NumberOfRuns / 2];

	// Compute mean with 'exponential' weights based on difference to median
	float exp_avg = 0;
	float weight_acc = 0;
	for (auto value : runs)
	{
		float w = (value - median);
		w = 1.0f / (w*w + 1);

		exp_avg += value * w;
		weight_acc += w;
	}

	exp_avg /= weight_acc;

	return median;//accumulate(begin(runs), end(runs), 0.0f) / (float)NumberOfRuns;
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

float BaselineRandom()
{
	Individual tmp_org;

	float runs[NumberOfRuns];

	// Do multiple runs, and return the median
	// This way outliers are eliminated
	for (int j = 0; j < NumberOfRuns; j++)
	{
		BuildWorld();

		tmp_org.Reset();

		const int max_iters = 1000;
		for (int i = 0; i < max_iters; i++)
		{
			if (tmp_org.Life <= 0) break;

			// Choose actions randomly
			uniform_int_distribution<int> action_dist(0, 3);
			tmp_org.ExecuteAction(action_dist(rng));
		}

		runs[j] = tmp_org.AccumulatedLife;
	}

	sort(begin(runs), end(runs));

	float median = runs[NumberOfRuns / 2];

	return median;//accumulate(begin(runs), end(runs), 0.0f) / (float)NumberOfRuns;
}

float BaselineGreedy()
{
	Individual tmp_org;

	float runs[NumberOfRuns];

	// Do multiple runs, and return the median
	// This way outliers are eliminated
	for (int j = 0; j < NumberOfRuns; j++)
	{
		BuildWorld();

		tmp_org.Reset();

		const int max_iters = 1000;
		for (int i = 0; i < max_iters; i++)
		{
			if (tmp_org.Life <= 0) break;

			CellType nearby_cells[4];
			nearby_cells[0] = World[tmp_org.PosX + 1][tmp_org.PosY    ];
			nearby_cells[1] = World[tmp_org.PosX - 1][tmp_org.PosY    ];
			nearby_cells[2] = World[tmp_org.PosX    ][tmp_org.PosY + 1];
			nearby_cells[3] = World[tmp_org.PosX    ][tmp_org.PosY - 1];

			int action = -1;

			// If there was an action previously selected, and there's a new option, replace randomly
			// This encourages exploration
			auto random_replace = [&](int NewAction)
			{
				if (action == -1)
					action = NewAction;
				else
				{
					uniform_int_distribution<int> replace_dist(0, 1);
					if (replace_dist(rng))
						action = NewAction;
				}
			};

			// First try to move to the food
			for (int a = 0; a < 4; a++)
			{
				if (nearby_cells[a] == FoodCell)
					random_replace(a);
			}

			// Then try to move to an empty cell
			if (action == -1)
			{
				for (int a = 0; a < 4; a++)
				{
					if (nearby_cells[a] == EmptyCell)
						random_replace(a);
				}
			}

			// If can't do any of the above, move randomly
			if (action == -1)
			{
				uniform_int_distribution<int> action_dist(0, 3);
				action = action_dist(rng);
			}
			
			tmp_org.ExecuteAction(action);
		}

		runs[j] = tmp_org.AccumulatedLife;
	}

	sort(begin(runs), end(runs));

	float median = runs[NumberOfRuns / 2];

	return median;//accumulate(begin(runs), end(runs), 0.0f) / (float)NumberOfRuns;
}

int main()
{	
	// Load NEAT parameters
	NEAT::load_neat_params("../cfg/neat_test_config.txt");

	// Create population
	auto start_genome = new NEAT::Genome(2, 4, 0, 0);
	auto pop = new NEAT::Population(start_genome, NEAT::pop_size);

	// Run evolution
	//	Also write average fitness to file
	ofstream fit_file("epochs.csv");
	fit_file << "Generation,Avg Fitness,Avg Real Fitness,Best Historic Fitness,Baseline Random Avg Real Fitness,Baseline Greedy Avg Real Fitness" << endl;

	float best_fitness = 0;

	for (int i = 0;i < 400;i++)
	{
		for (auto& org : pop->organisms)
		{
			org->fitness = EvaluteNEATOrganism(org);
			if (org->fitness > best_fitness)
				best_fitness = org->fitness;
		}

		// Finally turn to the next generation
		pop->epoch(i + 1);

		// Write to file
		fit_file << i << "," 
				 << pop->mean_fitness << "," 
				 << pop->mean_real_fitness << ","
				 << best_fitness << ","
				 //<< pop->max_real_fitness << ","
				 << BaselineRandom() << ","
				 << BaselineGreedy()
			<< endl;
	}

	// Simulate and plot positions for the best organism
	plt::ion();

	// After evolution is finished, evaluate the population one last time, but now with an increased number of tries
	Individual best_org;
	best_fitness = 0;

	for (auto org : pop->organisms)
	{
		float fitness = 0;

		const int repetitions = 20;
		for(int i = 0;i < repetitions;i++)
			fitness += EvaluteNEATOrganism(org);

		fitness /= (float)repetitions;

		if (fitness > best_fitness)
		{
			best_fitness = fitness;
			best_org.Brain = org->net;
		}
	}

	DumpNetworkToDot("best.dot",best_org.Brain);

	while (true)
	{
		BuildWorld();

		best_org.Reset();

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