#include "Utils.h"
#include <fstream>

// NEAT
#include "neat.h"
#include "network.h"
#include "organism.h"
#include "genome.h"

using namespace std;

void agio::DumpNetworkToDot(string Filename, NEAT::Network* Net)
{
	ofstream file(Filename);

	file << "digraph network" << endl;
	file << "{" << endl;

	// Write nodes
	//	Sensors are a box, neurons a circle, bias are hexagons, and output neurons a triangle
	for (auto node : Net->all_nodes)
	{
		file << node->node_id;
		if (node->gen_node_label == NEAT::INPUT)
			file << " [shape=box]";
		else if (node->gen_node_label == NEAT::BIAS)
			file << " [shape=hexagon]";
		else if (node->gen_node_label == NEAT::OUTPUT)
			file << " [shape=invtriangle]";

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
	file << "{ rank=sink; ";
	for (auto output : Net->outputs)
		file << output->node_id << " ";
	file << " }" << endl;

	file << "{ rank=source; ";
	for (auto input : Net->inputs) // The bias is on the inputs
		file << input->node_id << " ";
	file << " }" << endl;

	file << "}" << endl;
}
