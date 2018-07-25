#pragma once
#include <string>

namespace agio
{
	// Writes a .dot file of the provided network
	// For visualization purposes
	namespace NEAT
	{
		class Network;
	}
	void DumpNetworkToDot(std::string Filename, class NEAT::Network* Net);
}
