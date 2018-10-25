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

void SNode::flushback()
{

    if (type != NodeType::SENSOR && activation_count > 0)
    {
        for (auto link : incoming)
        {
            if (link->in_node->activation_count > 0)
                link->in_node->flushback();
        }
    }

    activation_count = 0;
    activation = 0;
}

bool SNetwork::outputsOff()
{
    for (auto node : all_nodes)
    {
        if (node->activation_count == 0)
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
        for (auto node : all_nodes)
        {
            if (node->type != NodeType::SENSOR)
            {
                node->activesum = 0;
                node->active_flag = false;

                for (auto link : node->incoming)
                {
                    if (link->in_node->active_flag || link->in_node->type == NodeType::SENSOR)
                        node->active_flag = true;

                    node->activesum += link->weight * link->in_node->getActiveOut();
                }

            }
        }

        // Now activate all the non-sensor nodes off their incoming activation
        for (auto node : all_nodes)
        {
            if (node->type != NodeType::SENSOR && node->active_flag)
            {
                node->activation = sigmoid(node->activesum);
            }
        }

        one_time = true;
    }

	return true;
}


void SNetwork::load_sensors(const std::vector<double> &sensorsValues)
{
    for (int i = 0; i < sensorsValues.size(); i++)
        inputs[i]->activation = sensorsValues[i];
}

void SNetwork::flush()
{
    for (auto node : outputs)
        node->flushback();
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

SLink::SLink(double weight, SNode *in_node, SNode *out_node)
{
    this->weight = weight;
    this->in_node = in_node;
    this->out_node = out_node;
}

SNetwork::SNetwork() { WasMoved = false; }

SNetwork::SNetwork(NEAT::Network *network) : SNetwork()
{
    for (auto node : network->all_nodes)
    {
        NodeType type = node->type == NEAT::SENSOR? NodeType::SENSOR : NodeType::NEURON;
        SNode *snode = new SNode(type);

        nodeMap.emplace(node, snode);
        all_nodes.emplace_back(snode);
    }

    for (auto node : network->inputs)
        inputs.emplace_back(nodeMap[node]);

    for (auto node : network->outputs)
        outputs.emplace_back(nodeMap[node]);


    for (auto node : network->all_nodes)
    {
        for (auto link : node->incoming)
            nodeMap[node]->incoming.emplace_back(findLink(link));

        for (auto link : node->outgoing)
            nodeMap[node]->outgoing.emplace_back(findLink(link));
    }
}

SNetwork::SNetwork(SNetwork&& rhs) : 
	inputs(move(rhs.inputs)),
	outputs(move(rhs.outputs)),
	all_nodes(move(rhs.all_nodes)),
	linkMap(move(rhs.linkMap)),
	nodeMap(move(rhs.nodeMap))
{
	rhs.WasMoved = true;
}

SNetwork& SNetwork::operator=(SNetwork&& rhs)
{
	inputs = move(rhs.inputs);
	outputs = move(rhs.outputs);
	all_nodes = move(rhs.all_nodes);
	linkMap = move(rhs.linkMap);
	nodeMap = move(rhs.nodeMap);

	return *this;
}

SNetwork::~SNetwork()
{
	if (WasMoved)
		return;

	for (auto node : all_nodes)
	{
		for (auto link : node->incoming)
			delete link;
		for (auto link : node->outgoing)
			delete link;
		delete node;
	}
}

void SNetwork::Duplicate(SNetwork& Clone) const
{
	// Create a temporal map that converts from the old pointers to the new pointers
	unordered_map<SNode*, SNode*> pointer_map;
	for (SNode * node : all_nodes)
	{
		SNode * new_node = new SNode(node->type);
		*new_node = *node; // There are pointers here that need to be corrected
		pointer_map[node] = new_node;
		Clone.all_nodes.push_back(new_node);
	}

	// Correct pointers
	for (SNode * node : Clone.all_nodes)
	{
		for (auto& link_ptr : node->incoming)
		{
			SLink * new_link = new SLink;

			new_link->in_node = pointer_map.find(link_ptr->in_node)->second;
			new_link->out_node = pointer_map.find(link_ptr->out_node)->second;
			new_link->weight = link_ptr->weight;

			link_ptr = new_link;
		}
		for (auto& link_ptr : node->outgoing)
		{
			SLink * new_link = new SLink;

			new_link->in_node = pointer_map.find(link_ptr->in_node)->second;
			new_link->out_node = pointer_map.find(link_ptr->out_node)->second;
			new_link->weight = link_ptr->weight;

			link_ptr = new_link;
		}
	}
}

SLink* SNetwork::findLink(NEAT::Link *link)
{
    SLink *slink;
    if (linkMap.find(link) == linkMap.end())
    {
        SNode *in_node = nodeMap[link->in_node];
        SNode *out_node = nodeMap[link->out_node];
        slink = new SLink(link->weight, in_node, out_node);

        linkMap.emplace(link, slink);
    } else {
        slink = linkMap[link];
    }

    return slink;
}






