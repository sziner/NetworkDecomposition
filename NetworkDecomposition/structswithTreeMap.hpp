#pragma once
#include <vector>
#include <unordered_set>
#include "robin_hood.h"
using namespace std;
enum state_t : uint8_t { ACTIVE, STALLING, FINISHED, KILLED };
enum msg_t : uint8_t { PROPOSE, UPCAST, UPCAST_GROW, DOWNCAST_ACCEPT, DOWNCAST_KILL, ACCEPT, KILL, UPDATE_ACCEPTED, UPDATE_STALLING, UPDATE_KILLED };


struct Message
{
	msg_t type;
	int id;
	int label;
	int value;
};

struct Tree
{
	int prop_count;
	int height;
	uint8_t grow;
	int parent;
	vector<int> children;
};

struct Neighbor
{
	//int node;
	//int port;
	int label;
	int level;
	state_t state;

	bool operator<(const Neighbor& other)
	{
		return label != other.label && state == ACTIVE && (other.state != ACTIVE ||
			level < other.level || level == other.level && (label & 1u << level) && !(other.label & 1u << other.level));
	}
};

struct Node
{
	int label;
	int tokens = 1;
	int level = 0;
	int color;
	robin_hood::unordered_map<int, Neighbor> neighbors;
	//vector<Neighbor> neighbors;
	vector<Message> inbox[2];
	robin_hood::unordered_flat_map<int, Tree> T;
	//vector<Tree> T;
	vector<int> proposals;
	vector<int> upcast;

	/*Node(const int& u, vector<int>& nbr, vector<Node>& nodes) :label(u)
	{
		neighbors.reserve(nbr.size());
		auto v = nbr.begin();
		for (; *v < u; ++v)
		{
			neighbors.emplace_back(Neighbor{ *v,NULL,*v,0,ACTIVE });
			auto it = lower_bound(nodes[*v].neighbors.begin(), nodes[*v].neighbors.end(), u, [](const Neighbor& a, int& value) {return a.node < value; });
			it->port = v;
		}
		for (auto s = nbr.end(); v != s; ++v)
		{
			neighbors.emplace_back(Neighbor{ *v,-1,*v,0,ACTIVE });
		}
		T.emplace_back(Tree{ 0,0, 0,{-1, -1} });
	}*/
};

class Graph
{
	//vector<Node> nodes;
	robin_hood::unordered_map<int, Node> nodes;
	/*Graph(const vector<vector<int>>& adj)
	{
		nodes.reserve(adj.size());
		for (auto u = adj.begin(), s = adj.end(); u != s; ++u)
		{
			nodes.emplace_back(Node(u-adj.begin(), *u, nodes));
		}
	}*/
	int networkdecomposition();
};