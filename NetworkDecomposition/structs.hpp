#pragma once
#include <vector>
//#include <functional>
#include "robin_hood.h"

namespace sz_nd
{

enum state_t : uint8_t { ACTIVE, STALLING, FINISHED, KILLED };
//enum msg_t : uint8_t { PROPOSE, UPCAST, UPCAST_GROW, DOWNCAST_ACCEPT, DOWNCAST_KILL, ACCEPT, KILL, UPDATE_ACCEPTED, UPDATE_STALLING, UPDATE_KILLED };
using std::vector;

using tree_idx = int;
using node_id = int;


struct tree_node
{
	int parent;
	int next_sibling;
	int first_child = -1;
	int prop_count = 0;
	int height = 0;
};

using down_tree_node = vector<node_id>;

struct queue_entry
{
	int label;
	int id;
	/*bool operator<(queue_entry& o)
	{
		return height < o.height;
	}*/
};
struct Message
{
	int dst;
	int src;

	bool operator<(const Message& o) const
	{
		return dst < o.dst;
	}
};

struct tree
{
	//using robin_hood::unordered_map;
	//int node_count = 1;
	int tokens = 1;
	robin_hood::unordered_map<int, tree_node> tree_nodes;
	vector<int> q;
	vector<int> leaves;
	vector<Message> proposals;

	auto& operator[](auto i)
	{
		tree_nodes[i];
	}

	void sort_queue()
	{
		if(q.size() < 2) return;
		auto first = q.rbegin(), mid = first, it = first + 1, end = q.rend();
		int x = first->height;
		while(it != end)
		{
			if(it->height < x)
			{
				mid = it;
				it = std::find_if(mid + 1, end, [x](auto& a) { return a.height >= x; });
				int d = std::min(mid - first, it - mid);
				std::swap_ranges(first, (first + d), it - d);
				first += it - mid;
				mid = first;
			}
			else
			{
				if(it->height > x)
				{
					x = it->height;
					first = it;
				}
				++it;
			}
		}
	}
};


//using down_tree = robin_hood::unordered_map<int, down_tree_node>;





struct BaseNode
{
	//const node_id id;
	int label;
	int level;
	//state_t state;

	bool operator<(const BaseNode& o) const
	{
		return level < o.level || (level == o.level && ((label >> level) & 1u) && !((o.label >> o.level) & 1u));
	}
};



class Graph
{
	using Neighbor = BaseNode;
	struct Node : public BaseNode
	{
		int tokens = 1;
		//int color;
		robin_hood::unordered_map<int, Neighbor> active_neighbors;
		vector<Neighbor> neighbors[4];
		/*vector<Neighbor> active_neighbors;
		vector<Neighbor> stalling_neighbors;
		vector<Neighbor> finished_neighbors;
		vector<Neighbor> killed_neighbors;*/
		//vector<Tree> T;
		robin_hood::unordered_map<int, Tree> T;
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
	vector<vector<int>>& adj;
	/*Graph(const vector<vector<int>>& adj)
	{
		nodes.reserve(adj.size());
		for (auto u = adj.begin(), s = adj.end(); u != s; ++u)
		{
			nodes.emplace_back(u-adj.begin(), *u, nodes);
		}
	}*/
	int networkdecomposition();

};
}//namespace sz_nd