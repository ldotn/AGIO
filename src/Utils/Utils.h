#pragma once
#include <string>
#include <chrono>

namespace NEAT
{
	class Network;
}

namespace agio
{
	// Writes a .dot file of the provided network
	// For visualization purposes
	void DumpNetworkToDot(std::string Filename, NEAT::Network* Net);

#define PROFILE_INNER(var, expr, c) auto __start##c = std::chrono::high_resolution_clock::now();\
										expr\
									auto __end##c = std::chrono::high_resolution_clock::now();\
									std::chrono::duration<float, std::milli> __time##c = __end##c - __start##c;\
									float var = __time##c.count();\

	// Evaluates the expr and stores in "var" the number of milliseconds 
#define PROFILE_EXPAND(var, expr, c) PROFILE_INNER(var, expr, c)
#define PROFILE(var, expr) PROFILE_EXPAND(var, expr, __COUNTER__)
}
