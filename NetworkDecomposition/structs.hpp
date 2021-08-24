#pragma once
#include <vector>
#include <functional>
#include "robin_hood.h"

namespace sz_nd
{

enum state_t : uint8_t { ACTIVE, STALLING, FINISHED, KILLED };
//enum msg_t : uint8_t { PROPOSE, UPCAST, UPCAST_GROW, DOWNCAST_ACCEPT, DOWNCAST_KILL, ACCEPT, KILL, UPDATE_ACCEPTED, UPDATE_STALLING, UPDATE_KILLED };
using std::vector;

using tree_idx = int;
using node_id = int;

using DowncastMessage = tree_idx;//tree

struct UpcastMessage
{
	tree_idx tree;
	int p_count;
	int min;
	int max;
};

//using tree_node = std::pair<node_id, tree_idx>;
struct tree_node
{
	node_id id;
	tree_idx tree;
};

struct Tree
{
	int prop_count = 0;
	int height = 0;
	int hmin = 0;
	int hmax = 0;
	tree_node parent;
	vector<tree_node> children;
};

struct BaseNode
{
	const node_id id;
	int label;
	int level;
	//state_t state;

	bool operator<(const BaseNode& o)
	{
		return label != o.label
			&& (level < o.level || (level == o.level && (label & 1u << level) && !(o.label & 1u << o.level)));
	}
};



class Graph
{
	using Neighbor = BaseNode;
	struct Node : public BaseNode
	{
		/*int label;
		int level = 0;*/
		int tokens = 1;
		int color;
		//robin_hood::unordered_map<int, Neighbor> neighbors;
		vector<Neighbor> neighbors[4];
		/*vector<Neighbor> active_neighbors;
		vector<Neighbor> stalling_neighbors;
		vector<Neighbor> finished_neighbors;
		vector<Neighbor> killed_neighbors;*/
		vector<Tree> T;
		vector<tree_idx> to_upcast;
		//robin_hood::unordered_flat_set<tree_idx> to_upcast;
		tree_node new_parent;
		vector<tree_node> proposals;

		struct Inbox
		{
			void swap(Inbox& o)
			{
				upcast.swap(o.upcast);
				downcast_accept.swap(o.downcast_accept);
				downcast_kill.swap(o.downcast_kill);
				std::swap(answer, o.answer);
			}
			int answer=0;
			vector<DowncastMessage> downcast_kill;
			vector<DowncastMessage> downcast_accept;
			vector<UpcastMessage> upcast;
		} mail, mailbox;

	public:
		bool is_proposing()
		{
			return new_parent.id != id;
		}

		bool is_terminal(tree_idx t_idx)
		{
			return t_idx == T.size() - 1;
		}

		void kill()
		{
			new_parent.id = -1;
		}

		void accept(tree_idx t_idx)
		{
			new_parent.tree = t_idx;
		}

		template <state_t state>
		void update_nbr(Neighbor nbr)
		{
			neighbors[state].emplace_back(nbr);
		}


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
	/*Graph(const vector<vector<int>>& adj)
	{
		nodes.reserve(adj.size());
		for (auto u = adj.begin(), s = adj.end(); u != s; ++u)
		{
			nodes.emplace_back(u-adj.begin(), *u, nodes);
		}
	}*/
	int networkdecomposition();
	void process_messages();

};
}//namespace sz_nd