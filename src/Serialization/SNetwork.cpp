#include <unordered_map>

#include "SNetwork.h"
#include "network.h"

using namespace std;
using namespace agio;

double SNode::getActiveOut()
{
    if (activation_count > 0 || type == NodeType::SENSOR)
        return activation;
    else
        return 0.0;
}

void SNode::flushback(SNetwork& Network)
{
    if (type != NodeType::SENSOR && activation_count > 0)
    {
        for (auto& link : incoming)
        {
            if (Network.all_nodes[link.in_node_idx].activation_count > 0)
				Network.all_nodes[link.in_node_idx].flushback(Network);
        }
    }

    activation_count = 0;
    activation = 0;
}

bool SNetwork::outputsOff()
{
    for (auto& node : all_nodes)
    {
        if (node.activation_count == 0)
            return true;
    }

    return false;
}


bool SNetwork::activate()
{
    //Keep activating until all the outputs have become active
    //(This only happens on the first activation, because after that they
    // are always active)
    bool one_time = false;
    int abort_count = 0;

    while (outputsOff() || !one_time)
    {
        abort_count++;
        if (abort_count == 200)
        {
            // inputs disconnected from outputs!
            return false;
        }


        // For each node, compute the sum of its incoming activation
        for (auto& node : all_nodes)
        {
            if (node.type != NodeType::SENSOR)
            {
                node.activesum = 0;
                node.active_flag = false;

                for (auto& link : node.incoming)
                {
                    if (all_nodes[link.in_node_idx].active_flag || all_nodes[link.in_node_idx].type == NodeType::SENSOR)
                        node.active_flag = true;

                    node.activesum += link.weight * all_nodes[link.in_node_idx].getActiveOut();
                }

            }
        }

        // Now activate all the non-sensor nodes off their incoming activation
        for (auto& node : all_nodes)
        {
            if (node.type != NodeType::SENSOR && node.active_flag)
            {
                node.activation = sigmoid(node.activesum);
            }
        }

        one_time = true;
    }

	return true;
}


void SNetwork::load_sensors(const std::vector<float> &sensorsValues)
{
	for (int i = 0; i < sensorsValues.size(); i++)
		all_nodes[inputs[i]].activation = sensorsValues[i];
}

void SNetwork::flush()
{
    for (int node_idx : outputs)
		all_nodes[node_idx].flushback(*this);
}

SNode::SNode() {
    activesum = 0;
    activation = 0;
    active_flag = false;
}

SNode::SNode(NodeType type) : SNode()
{
    this->type = type;
}

SLink::SLink() {}
SLink::SLink(double w, int in_idx, int out_idx) : SLink()
{
	weight = w;
	in_node_idx = in_idx;
	out_node_idx = out_idx;
}

SNetwork::SNetwork() { }

SNetwork::SNetwork(NEAT::Network *network) : SNetwork()
{
	// variables used for internal logic
	std::unordered_map<NEAT::Link*, SLink> linkMap;
	std::unordered_map<NEAT::NNode*, int> nodeMap;

	int current_node_idx = 0;
	all_nodes.resize(network->all_nodes.size());
    for (auto node : network->all_nodes)
    {
        NodeType type = node->type == NEAT::SENSOR? NodeType::SENSOR : NodeType::NEURON;

		all_nodes[current_node_idx++] = SNode(type);
		nodeMap.emplace(node, current_node_idx - 1);
    }

	auto findLink = [&](NEAT::Link *link)
	{
		SLink slink;
		if (linkMap.find(link) == linkMap.end())
		{
			int in_node = nodeMap[link->in_node];
			int out_node = nodeMap[link->out_node];
			slink = SLink(link->weight, in_node, out_node);

			linkMap.emplace(link, slink);
		}
		else {
			slink = linkMap[link];
		}

		return slink;
	};

    for (auto node : network->inputs)
        inputs.emplace_back(nodeMap[node]);

    for (auto node : network->outputs)
        outputs.emplace_back(nodeMap[node]);

    for (auto node : network->all_nodes)
    {
        for (auto link : node->incoming)
			all_nodes[nodeMap[node]].incoming.emplace_back(findLink(link));

        for (auto link : node->outgoing)
			all_nodes[nodeMap[node]].outgoing.emplace_back(findLink(link));
    }
}

size_t SNode::TotalSize() const
{
	// Start with the size of the class
	size_t ret = sizeof(*this);

	// Add the size of the elements
	for (const auto& link : incoming)
		ret += link.TotalSize();
	for (const auto& link : outgoing)
		ret += link.TotalSize();

	return ret;
}

size_t SNetwork::TotalSize() const
{
	// Start with the size of the class
	size_t ret = sizeof(*this);

	// Add the size of the elements
	ret += inputs.capacity() * sizeof(int);
	ret += outputs.capacity() * sizeof(int);
	
	for (const auto& node : all_nodes)
		ret += node.TotalSize();

	return ret;
}