#pragma once

#include <math.h>
#include <unordered_map>
#include <vector>

#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

// Forward declarations
namespace NEAT
{
	class Network;
	class NNode;
	class Link;
}

namespace agio
{
	// Forward declarations
	class SNode;
	class SNetwork;

	enum class NodeType 
	{
		NEURON = 0,
		SENSOR = 1
	};

	inline double sigmoid(double value) 
	{
		return (1 / (1 + (exp(-(4.924273 * value))))); // look at neat.cpp line 482
	}

	class SLink 
	{
	public:
		friend SNode;
		friend SNetwork;

		double weight;
		int in_node_idx;
		int out_node_idx;

		SLink();
		SLink(double weight, int in_node_idx, int out_node_idx);

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & weight;
			ar & in_node_idx;
			ar & out_node_idx;
		}
	};

	class SNode 
	{
	public:
		NodeType type;
		double activation;

		std::vector<SLink> incoming;
		std::vector<SLink> outgoing;

		SNode();
		SNode(NodeType type);
	private:
		friend SNetwork;
		friend SLink;

		double activesum;
		bool active_flag;
		int activation_count;

		double getActiveOut();
		void flushback(SNetwork& Network);

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & type;
			ar & activation;
			ar & incoming;
			ar & outgoing;
		}
	};

	class SNetwork
	{
	public:
		std::vector<int> inputs;
		std::vector<int> outputs;
		std::vector<SNode> all_nodes;

		void flush();
		bool activate();
		void load_sensors(const std::vector<float> &sensorsValues);

		SNetwork();
		SNetwork(NEAT::Network *network);

	private:
		bool outputsOff();

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & all_nodes;
			ar & inputs;
			ar & outputs;
		}
	};
}