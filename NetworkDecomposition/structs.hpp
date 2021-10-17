#pragma once
#include <vector>
//#include <functional>
#include "robin_hood.h"

namespace sz_nd {

enum state_t : uint8_t { ACTIVE, STALLING, FINISHED, KILLED };
//enum msg_t : uint8_t { PROPOSE, UPCAST, UPCAST_GROW, DOWNCAST_ACCEPT, DOWNCAST_KILL, ACCEPT, KILL, UPDATE_ACCEPTED, UPDATE_STALLING, UPDATE_KILLED };
using std::vector;
using uint = uint32_t;

struct Proposal {
	uint id;
	uint depth;
};

struct cluster {
	uint node_count = 1;
	uint tokens = 1;
	vector<Proposal> proposals;
	//uint level = 0;
	//uint depth;
};

class Graph {
	struct BaseNode {
		//const int id;
		int label;
		int level;
		int depth;
		state_t state;
		
		bool operator<(const BaseNode& o) const {
			return (state == ACTIVE) && (level < o.level || (level == o.level && (((label >> level) & 1u) && !((o.label >> o.level) & 1u))));
		}
	};

	struct Node : public BaseNode {
		//int tokens = 1;
		//int color;
		//int depth;
		//vector<int> active_neighbors;

		/*Node(const int& u, vector<int>& nbr, vector<Node>& nodes) :label(u)
		{
			neighbors.reserve(nbr.size());
			auto v = nbr.begin();
			for (; *v < u; ++v)
			{
				neighbors.emplace_back(*v,*v,0);
				auto it = lower_bound(nodes[*v].neighbors.begin(), nodes[*v].neighbors.end(), u, [](const Neighbor& a, int& value) {return a.node < value; });
				it->port = v;
			}
			for (auto s = nbr.end(); v != s; ++v)
			{
				neighbors.emplace_back(*v,*v,0);
			}
			T.emplace_back(0,0, 0, 0, {-1, -1});
		}*/
	};

	//vector<Node> nodes;
	robin_hood::unordered_map<int, Node> nodes;
	vector<vector<int>>& adj;
	/*Graph(const vector<vector<int>>& adj)
	{
		nodes.reserve(adj.size());
		for (auto u = adj.begin(), s = adj.end(); u != s; ++u)
		{
			nodes.emplace_back(u-adj.begin(), *u, nodes);
		}
	}*/
	int networkdecomposition(vector<uint>& colors);

};
}//namespace sz_nd