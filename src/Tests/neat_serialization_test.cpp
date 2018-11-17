#include "neat.h"
#include "network.h"
#include "genome.h"
#include <iostream>
#include <fstream>
#include <random>
#include "zip.h"
#include "../Serialization/SNetwork.h"

using namespace fpp;
using namespace std;
using namespace NEAT;
using namespace agio;

int main()
{
	ofstream out_f("network_values.csv");

	mt19937 rng(42);
	for (int i = 0; i < 1000; i++)
	{
		cout << ".";
		int num_in = uniform_int_distribution<int>(1, 100)(rng);
		int num_out = uniform_int_distribution<int>(1, 100)(rng);
		int num_hidden = uniform_int_distribution<int>(1, 100)(rng);
		int type = uniform_int_distribution<int>(0, 2)(rng);

		Genome neat_net(num_in, num_out, num_hidden, type);
		auto net = neat_net.genesis(0);
		auto serialization_net = SNetwork(net);

		vector<float> inputs(num_in);
		
		for (int j = 0; j < 100; j++)
		{
			for (auto v : inputs)
				v = uniform_real_distribution<float>(-1e6, 1e6)(rng);
			net->load_sensors(inputs);
			net->activate();

			serialization_net.load_sensors(inputs);
			serialization_net.activate();

			double error = 0;
			for (auto[v0, v1] : zip(net->outputs, serialization_net.outputs))
				error += abs(double(v0->activation) - double(v1->activation));
			out_f << error << endl;
		}
	}
}