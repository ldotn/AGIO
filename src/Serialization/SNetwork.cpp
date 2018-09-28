#include <unordered_map>

#include "SNetwork.h"

double SNode::getActiveOut()
{
    if (activation_count > 0)
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

SNetwork::SNetwork() {}

SNetwork::SNetwork(NEAT::Network *network)
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






