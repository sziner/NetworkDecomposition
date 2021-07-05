#include "structs_vectoring.hpp"
#include "utils.hpp"
#include <algorithm>
#include <iterator>
#include <numeric>
using namespace std;


int Graph::networkdecomposition()
{
	const unsigned N = nodes.size();
	const unsigned LOGN = log2(N);
	const int B = LOGN;
	//int num_unfinished = N;//to end early
	vector<int> accepted_nodes;//to send update to neighbors at the end of a step
	vector<int> stalled_nodes;//to send update to neighbors at the end of a step
	vector<int> killed_nodes;
	vector<int> living_nodes;
	living_nodes.reserve(N);
	for (int i = 0; i < N; ++i)
		living_nodes.push_back(i);
	for (int c = 0; c < LOGN; ++c)
	{
		for (int phase = 0; phase < 2 * (B + LOGN); ++phase)
		{
			for (int step = 0; step < 28 * (B + LOGN); ++step)
			{
				int round = 0;//first round, PROPOSE messages
				for (int& idx : living_nodes)
				{
					auto& u = nodes[idx];

					/*check for neighbors NEW*/
					auto& v = *std::min_element(u.active_neighbors.begin(), u.active_neighbors.end());
					if (v < u)
					{
						u.new_parent = v.id;
						u.new_label = v.label;
						nodes[v.id].proposals.push_back(idx);
					}

					/*************************/
					
					
					/*auto& [id, v] = *std::min_element(u.neighbors.begin(), u.neighbors.end(), [&](auto& kvl, auto& kvr) { return kvl.second < kvr.second; });
					if (v.label != u.label
						&& v.state == ACTIVE
						&& (v.level < u.level
							|| (v.level == u.level
								&& (v.label & 1u << v.level)
								&& !(u.label & 1u << u.level))))
					{
						u.new_parent = id;
						u.new_label = v.label;
						nodes[id].proposals.push_back(idx);
					}*/
				}
				round = 1;//second round. recieve proposals. nodes begin upcasting
				for (int& idx : active_nodes)
				{//only PROPOSE messages
					Node& u = nodes[idx];
					Tree& t = u.T[u.label];
					t.prop_count += u.proposals.size();
					if (t.height == 0)//u is a leaf
					{
						if (u.label == idx)//u is a terminal root. precess proposals
						{
							if (t.prop_count < u.tokens / (28 * (B + LOGN)))
							{//KILL
								u.tokens -= t.prop_count * 14 * (B + LOGN);
								//u.level += 1;
								for (auto& v : u.proposals)//kill proposals
								{
									nodes[v].inbox[(round + 1) % 2].kill.emplace_back(Message{ idx, u.label });
								}
								stalled_nodes.push_back(idx);
								for (auto& v : t.children)//downcast KILL
								{
									nodes[v].inbox[(round + 1) % 2].kill.emplace_back(Message{ idx, u.label });
								}
							}
							else
							{// ACCEPT
								u.tokens += t.prop_count;
								t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());
								for (auto& v : t.children)//accept proposals and downcast ACCEPT
								{
									nodes[v].inbox[(round + 1) % 2].accept.emplace_back(Message{ idx, u.label });
								}
								t.height += 1 * (t.prop_count != 0);
							}
							u.proposals.clear();
						}
						else//normal upcast. hold proposals
						{
							t.grow += 1 * (t.prop_count != 0);
							nodes[t.parent].inbox[(round + 1) % 2].upcast.emplace_back(
								UpcastMessage{ u.label, t.prop_count, 1 });
						}
						t.prop_count = 0;
					}
				}
				for (int& idx : killed_nodes)
				{
					Node& u = nodes[idx];
					Tree& t = u.T[u.label];

				}

				for (round = 2; round < 2 * (step + 1)+ phase * 28 * (B + LOGN); ++round)//2*(step+1) rounds for upcast + downcast
				{
					for (auto& [idx, u] : nodes)
					{
						//process KILL messages
						for (Message& m : u.inbox[round % 2].kill)
						{
							if (m.id == u.new_parent && m.label == u.new_label)//getting killed
							{
								for (auto& [id, v] : u.neighbors)//broadcast KILLED
								{
									nodes[id].inbox[(round + 1) % 2].killed.push_back(idx);
								}
								u.new_parent = idx;
							}
							else//u is in tree
							{
								if (m.label == u.label)//u is terminal
								{
									for (auto& v : u.proposals)//kill proposals
									{
										nodes[v].inbox[(round + 1) % 2].kill.emplace_back(Message{ idx, m.label });
									}
									u.proposals.clear();

									if (u.label == u.new_label)
										stalled_nodes.push_back(idx);//become STALLED. need to increase u.level later					
								}

								for (auto& v : u.T[m.label].children)//downcast KILL
								{
									nodes[v].inbox[(round + 1) % 2].kill.emplace_back(Message{ idx, m.label });
								}
							}
						}
						//process ACCEPT messages
						for (Message& m : u.inbox[round % 2].accept)
						{
							if (m.id == u.new_parent && m.label == u.new_label)//u is accepted
							{
								//to do at the end of the step, when updating:
								/*u.label = u.new_label;
								u.level = u.neighbors[u.new_parent].level;
								u.T.emplace(m.label, Tree{ 0, 0, -1, u.new_parent });
								u.new_parent = idx;*/
								accepted_nodes.push_back(idx);
							}
							else//u is in tree
							{
								Tree t = u.T[m.label];
								if (m.label == u.label)//u is terminal
								{
									t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());
									u.proposals.clear();
								}
								t.height += t.grow;
								t.grow = -1;
								for (auto& v : t.children)//downcast ACCEPT (and accept proposals if u is terminal)
								{
									nodes[v].inbox[(round + 1) % 2].accept.emplace_back(Message{ idx, u.label });
								}
							}
						}
						//process upcasts
						for (UpcastMessage& m : u.inbox[round % 2].upcast)
						{
							Tree t = u.T[m.label];
							t.prop_count += m.value;
							if (round == t.height + 1 && t.grow < m.grow)
							{
								if (t.grow == -1)
									u.upcast.push_back(m.label);

								t.grow = m.grow;
							}
						}
						//send upcasts (or a downcast if u is root)
						u.inbox[round % 2].upcast.clear();
						for (int& lbl : u.upcast)
						{
							Tree t = u.T[lbl];
							if (idx == lbl)//u is root, downcast
							{
								if (t.prop_count < u.tokens / (28 * (B + LOGN)))//kill
								{
									if (u.label == lbl)//u is terminal
									{
										for (auto& v : u.proposals)//kill proposals
										{
											nodes[v].inbox[(round + 1) % 2].kill.emplace_back(Message{ idx, lbl });
										}
										u.proposals.clear();

										if (u.label == u.new_label)
											stalled_nodes.push_back(idx);//need to increase u.level later					
									}

									for (auto& v : t.children)//downcast KILL
									{
										nodes[v].inbox[(round + 1) % 2].kill.emplace_back(Message{ idx, lbl });
									}
									u.tokens -= t.prop_count * 14 * (B + LOGN);
								}
								else//accept
								{
									if (u.label == lbl)//u is terminal
									{
										t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());
										u.proposals.clear();
										t.height += t.grow;
										t.grow = -1;
									}
									for (auto& v : t.children)//downcast ACCEPT (and accept proposals if u is terminal)
									{
										nodes[v].inbox[(round + 1) % 2].accept.emplace_back(Message{ idx, u.label });
									}
									u.tokens += t.prop_count;
								}
								t.grow = -1;
							}
							else// u is not root
							{
								nodes[t.parent].inbox[(round + 1) % 2].upcast.emplace_back(
									UpcastMessage{ lbl, t.prop_count, t.grow });
							}
							t.prop_count = 0;
						}
						u.upcast.clear();

						//send ACCEPTED messages

						//process ACCEPTED messages
						for (AcceptedMessage& m : u.inbox[round % 2].accepted)
						{
							Neighbor& v = u.neighbors[m.id];
							v.label = m.label;
							v.level = m.level;
							v.state = ACTIVE;
						}

						//process STALLED messages
						for (int& m : u.inbox[round % 2].stalled)
						{
							Neighbor& v = u.neighbors[m];
							v.level += 1;
							v.state = STALLING;
						}
						//process KILLED messages
						for (int& m : u.inbox[round % 2].killed)
						{
							u.neighbors[m].state = KILLED;
						}

					}//node
					//updates stage:

				}//round
				for (auto& idx : stalled_nodes)
				{
					Node& u = nodes[idx];
					
					
				}
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
1. eliminate dead branches by upcasting from dead leaves in round 1. each round, every dead node checks its max height.
if it's 0, it upcasts height of 0 and deletes itself from the tree.
*/
void Graph::Node::make_proposals()
{

}