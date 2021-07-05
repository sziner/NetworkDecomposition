#include "structswithTreeMap.hpp"
#include "utils.hpp"
#include <algorithm>
#include <iterator>
using namespace std;


int Graph::networkdecomposition()
{
	const unsigned N = nodes.size();
	const unsigned LOGN = log2(N);
	const int B = LOGN;
	robin_hood::unordered_set<int> living_nodes;
	living_nodes.reserve(N);
	living_nodes.insert(nodes.begin(), nodes.end());
	vector<int> killed_nodes;
	for (int c = 0; c < LOGN; ++c)
	{
		for (int phase = 1; phase <= 2 * (B + LOGN); ++phase)
		{
			for (int step = 0; step < 28 * (B + LOGN); ++step)
			{
				int round = 0;//first round, PROPOSE messages
				for (int& idx : living_nodes)
				{
					auto& u = nodes[idx];
					auto& [minkey, minval] = *std::min_element(u.neighbors.begin(), u.neighbors.end(), [](auto& kvl, auto& kvr) { return kvl.second < kvr.second; });
					if (minval.label != u.label
						&& minval.state == ACTIVE
						&& (minval.level < u.level
							|| (minval.level == u.level
								&& (minval.label & 1u << minval.level)
								&& !(u.label & 1u << u.level))))
					{
						nodes[minkey].inbox[(round + 1) % 2].emplace_back(Message{ PROPOSE, idx, minval.label });
					}
				}
				round = 1;//second round. recieve proposals. nodes begin upcasting
				for (int& idx : active_nodes)//only ACTIVE nodes?? use SET?
				{//only PROPOSE messages
					Node& u = nodes[idx];
					Tree& t = u.T[u.label];
					t.prop_count = u.inbox[round % 2].size();
					for (Message& m : u.inbox[round % 2])
						u.proposals.push_back(m.id);
					if (t.height == 0)
					{//u is a leaf
						t.grow = 1 * (t.prop_count != 0);
						if (u.label == idx)//u is root. precess proposals
						{
							if (t.prop_count < u.tokens / (28 * (B + LOGN)))//kill proposing neighbors
							{
								u.tokens -= t.prop_count * 14 * (B + LOGN);
								u.level += 1;
								for (int& v : u.proposals)
								{
									nodes[v].inbox[(round + 1) % 2].emplace_back(Message{ KILL });
								}
								for (auto& [id, v] : u.neighbors)//update STALLING
								{
									nodes[id].inbox[(round + 1) % 2].emplace_back(Message{ UPDATE_STALLING, idx, u.label, u.level });
								}
							}
							else
							{//accept proposing neighbors
								u.tokens += t.prop_count;
								t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());
								for (int& v : u.proposals)
								{
									nodes[v].inbox[(round + 1) % 2].emplace_back(Message{ ACCEPT, idx, u.label, u.level });
								}
								u.proposals.clear();
								t.height += t.grow;
							}
							u.proposals.clear();
							t.grow = -1;
						}
						else
						{//normal upcast
							nodes[t.parent].inbox[(round + 1) % 2].emplace_back(
								Message{ static_cast<msg_t> (UPCAST + 1 * (t.grow == 1)), idx, u.label, t.prop_count });
						}
						t.prop_count = 0;
					}
				}
				for (round = 2; round < 2 * step + 4; ++round)//2*(step+1) rounds for upcast + downcast, and two more for updates 
				{
					for (auto& [idx, u] : nodes)
					{
						for (Message& m : u.inbox[round % 2])
						{
							switch (m.type)
							{
							case UPCAST:
							{//normal upcast
								Tree t = u.T[m.label];
								t.prop_count += m.value;
								if (round == t.height + 1 && t.grow == -1)
								{
									u.upcast.push_back(m.label);
									t.grow = 0;
								}
								break;
							}
							case UPCAST_GROW:
							{//normal upcast
								Tree t = u.T[m.label];
								t.prop_count += m.value;
								if (round == t.height + 1)
								{
									if (t.grow == -1)
									{
										u.upcast.push_back(m.label);
									}
									t.grow = 1;
								}
								break;
							}
							case ACCEPT:
							{
								u.level = m.value;
								u.T.emplace(m.label, Tree{ 0,0,-1,m.id,{} });
								for (auto& [id, v] : u.neighbors)
									nodes[id].inbox[(round + 1) % 2].emplace_back(
										Message{ UPDATE_ACCEPTED, idx, m.label, u.level });
								break;
							}
							case DOWNCAST_ACCEPT:
							{
								Tree t = u.T[m.label];
								if (m.label == u.label)//u belongs to the tree actively
								{
									for (auto& v : u.proposals)//accept proposing neighbors
									{
										nodes[v].inbox[(round + 1) % 2].emplace_back(
											Message{ ACCEPT, idx, m.label, m.value });
									}
									t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());//add them to tree
									for (int& c : t.children)//downcast to children
									{
										nodes[c].inbox[(round + 1) % 2].emplace_back(
											Message{ DOWNCAST_ACCEPT,idx,m.label,u.level });
									}
								}
								else
								{
									for (int& c : t.children)//downcast to children
									{
										nodes[c].inbox[(round + 1) % 2].emplace_back(
											Message{ DOWNCAST_ACCEPT,idx,m.label,m.value });
									}
								}

								u.proposals.clear();
								t.height += t.grow;
								t.grow = -1;
								break;
							}
							case KILL:
							{
								for (auto& [id, v] : u.neighbors)
									nodes[id].inbox[(round + 1) % 2].emplace_back(Message{ UPDATE_KILLED, idx });
								living_nodes.erase(idx);
								killed_nodes.push_back(idx);
								break;
							}
							case DOWNCAST_KILL:
							{
								Tree t = u.T[m.label];

								if (m.label == u.label)
								{
									for (int& c : t.children)
									{//downcast to children
										nodes[c].inbox[(round + 1) % 2].emplace_back(DOWNCAST_KILL, idx, m.label, u.level);
									}									for (int& v : u.proposals)
									{//kill proposing neighbors
										nodes[v].inbox[(round + 1) % 2].emplace_back(Message{ KILL, idx, m.label, u.level });
									}
									u.proposals.clear();
									for (auto& [id, v] : u.neighbors)
									{
										nodes[id].inbox[(round + 1) % 2].emplace_back(Message{ UPDATE_STALLING, idx, m.label, u.level });
									}
								}
								else
								{
									for (int& c : t.children)
									{//downcast to children
										nodes[c].inbox[(round + 1) % 2].emplace_back(DOWNCAST_KILL, idx, m.label, m.value);
									}
								}
								t.grow = -1;
								break;
							}
							case UPDATE_ACCEPTED:
							{
								Neighbor& v = u.neighbors[m.id];
								v.label = m.label;
								v.level = m.value;
								v.state = ACTIVE;
								break;
							}
							case UPDATE_KILLED:
							{
								u.neighbors[m.id].state = KILLED;
								break;
							}
							case UPDATE_STALLING:
							{
								u.neighbors[m.id].state = STALLING;
								u.neighbors[m.id].level = m.value;
								break;
							}
							/*
							case Message::FINISHED?:
							{
								break;
							}
							*/
							}
						}
						u.inbox[round % 2].clear();
						for (int& lbl : u.upcast)
						{
							Tree t = u.T[lbl];
							if (idx == lbl)//u is root
							{
								if (t.prop_count < u.tokens / (28 * (B + LOGN)))
								{
									for (int& c : t.children)
									{//downcast KILL message
										nodes[c].inbox[(round + 1) % 2].emplace_back(
											Message{ DOWNCAST_KILL, idx, lbl, u.level });//level may be unknown! problem??? nahh..
									}
									if (u.label == lbl)//current tree
									{
										for (int& v : u.proposals)//kill proposing neighbors
										{
											nodes[v].inbox[(round + 1) % 2].emplace_back(Message{ KILL });
										}
										for (auto& [id, v] : u.neighbors)//update STALLING
											nodes[id].inbox[(round + 1) % 2].emplace_back(
												Message{ UPDATE_STALLING, idx, lbl, u.level });
										u.level += 1;
									}
									u.tokens -= t.prop_count * 14 * (B + LOGN);
								}
								else
								{
									for (int& c : t.children)
									{//downcast ACCEPT message
										nodes[c].inbox[(round + 1) % 2].emplace_back(
											Message{ DOWNCAST_ACCEPT, idx, lbl, u.level });//level unknown! problem???
									}
									if (u.label == lbl)//current tree
									{
										for (int v : u.proposals)
										{//accept proposing neighbors and add them to tree
											nodes[v].inbox[(round + 1) % 2].emplace_back(
												Message{ ACCEPT, idx, lbl, u.level });
										}
										t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());
									}
									u.tokens += t.prop_count;
									t.height += t.grow;
								}
								u.proposals.clear();
								t.grow = -1;
							}
							else
							{
								nodes[t.parent].inbox[(round + 1) % 2].emplace_back(
									Message{ (msg_t)(PROPOSE + t.grow),idx,lbl,t.prop_count });
							}
							t.prop_count = 0;
						}
						u.upcast.clear();
					}//node
				}//round
			}//step
			for (int& idx : living_nodes)//change STALLING to ACTIVE
			{
				for (auto& [id, v] : nodes[idx].neighbors)
				{
					v.state = (state_t)(v.state - (1 * (v.state == STALLING)));
				}
			}
		}//phase
	}//color
}
/*TODO:
* 1. eliminate branches that were killed from tree?
* 2. */