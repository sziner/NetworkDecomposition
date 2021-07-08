#pragma once
#include <vector>
#include <unordered_set>
#include <functional>
#include "robin_hood.h"
using namespace std;
enum state_t : uint8_t { ACTIVE, STALLING, FINISHED, KILLED };
//enum msg_t : uint8_t { PROPOSE, UPCAST, UPCAST_GROW, DOWNCAST_ACCEPT, DOWNCAST_KILL, ACCEPT, KILL, UPDATE_ACCEPTED, UPDATE_STALLING, UPDATE_KILLED };

using DowncastMessage = int;

struct UpcastMessage
{
	int tree;
	int value;
	int grow;
};


struct NodeTree
{
	int id;
	int tree;
};

struct Tree
{
	int prop_count = 0;
	int upcast_count;//???
	int height = 0;
	int grow = 0;
	NodeTree parent;
	vector<NodeTree> children;
};

struct Neighbor
{
	int id;
	int label;
	int level;
	//state_t state;

	bool operator<(const Neighbor& o)
	{
		return label != o.label
			&& (level < o.level || (level == o.level && (label & 1u << level) && !(o.label & 1u << o.level)));
	}
};



class Graph
{
	struct Node : public Neighbor
	{
		/*int label;
		int level = 0;*/
		int tokens = 1;
		int color;
		//robin_hood::unordered_map<int, Neighbor> neighbors;
		vector<Neighbor> active_neighbors;
		vector<Neighbor> stalling_neighbors;
		vector<Neighbor> finished_neighbors;
		vector<Tree> T;
		NodeTree new_parent;
		vector<NodeTree> proposals;

		struct Inbox
		{
			void swap(Inbox& o)
			{
				upcast.swap(o.upcast);
				downcast_accept.swap(o.downcast_accept);
				downcast_kill.swap(o.downcast_kill);
				killed.swap(o.killed);
				stalled.swap(o.stalled);
				std::swap(answer, o.answer);
			}
			int answer=0;
			vector<DowncastMessage> downcast_kill;
			vector<DowncastMessage> downcast_accept;
			vector<UpcastMessage> upcast;
			vector<int> killed;
			vector<int> stalled;
		} mail, mailbox;

		void make_proposals();
		void process_proposals();
		void process_messages();
		void process_upcasts();
		void process_kills();
		void process_accepts();

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
	void process_messages();

};