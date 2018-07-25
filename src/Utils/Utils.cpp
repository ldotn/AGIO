#include "Utils.h"
#include <fstream>

// NEAT
#include "neat.h"
#include "network.h"
#include "organism.h"
#include "genome.h"

using namespace std;

void DumpNetworkToDot(string Filename, NEAT::Network* Net)
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
