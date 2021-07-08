#include "structs.hpp"
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
	vector<int> active_nodes;
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
						u.new_parent.id = v.id;
						nodes[v.id].proposals.emplace_back(idx,u.T.size());
					}
				}
				///////////////////////////////////////////////////////////////////////
				round = 1;//second round. recieve proposals. nodes begin upcasting
				for (int& idx : leaf_nodes)
				{//only PROPOSE messages
					Node& u = nodes[idx];
					Tree& t = u.T.back();
					t.prop_count += u.proposals.size();
					/*if (t.height == 0)//u is a leaf
					{*/
						if (u.T.size() == 1)//u is a terminal root. process proposals
						{
							if (t.prop_count < u.tokens / (28 * (B + LOGN)))
							{//KILL(just like regular downcast, except the tokens change)
								u.tokens -= t.prop_count * 14 * (B + LOGN);
								for (auto& [v,vt] : u.proposals)//kill proposals
								{
									nodes[v].mailbox.downcast_kill.push_back( v, vt );
								}
								stalled_nodes.push_back(idx);
								for (auto& [v, vt] : t.children)//downcast KILL
								{
									nodes[v].mailbox.downcast_kill.emplace_back( v, vt );
								}
							}
							else
							{// ACCEPT(just like regular downcast, except the tokens change)
								u.tokens += t.prop_count;
								t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());
								for (auto& [v,vt] : t.children)//accept proposals and downcast ACCEPT
								{
									nodes[v].mailbox.downcast_accept.emplace_back(vt);
								}
								t.height += 1 * (t.prop_count != 0);
							}
							u.proposals.clear();
						}
						else//normal upcast. hold proposals
						{
							t.grow = 2 * (t.prop_count != 0) + 1 * (t.prop_count == 0 && u.id == u.new_parent.id);
							nodes[t.parent.id].mail.upcast.emplace_back(t.parent.tree, t.prop_count, t.grow + 1/**/);
						}
						t.prop_count = 0;
					/*}*/				
				}
				///////////////////////////////////////////////////////////////////////////////////////////////////////////
				for (int& idx : killed_nodes)//all inactive nodes. maybe none. tree control.
				{
					Node& u = nodes[idx];
					for (Tree& t : u.T)
					{
						if (t.height == 0)
							nodes[t.parent.id].mailbox.upcast.emplace_back( t.parent.tree, 0, 0 );
					}
				}
				/// ///////////////////////////////////////////////////////////////////////////////////////////////////////

				for (round = 2; round < 2 * (step + 1) + phase * 28 * (B + LOGN); ++round)//2*(step+1) rounds for upcast + downcast
				{
					for (auto& [idx, u] : nodes)
					{
						//process KILL messages
						for (DowncastMessage& m : u.mail.downcast_kill)
						{
							if (m == u.T.size() - 1)//u is terminal
							{
								for (auto& [v, vt] : u.proposals)//kill proposals
								{
									nodes[v].new_parent.id = -1;
								}
								u.proposals.clear();

								if (u.id == u.new_parent.id && u.level < B)//u has not proposed and isn't FINISHED
								{
									++u.label;
									stalled_nodes.push_back(idx);//become STALLED. need to increase u.level later
								}
							}

							if (m == u.T.size())
							{


							}
							else
							{
								for (auto& [v, vt] : u.T[m].children)//downcast KILL
								{
									nodes[v].mailbox.downcast_kill.push_back(vt);
								}
							}

							
						}
						//process ACCEPT messages
						for (DowncastMessage& m : u.mail.downcast_accept)
						{
							Tree& t = u.T[m];
							if (m == u.T.size() - 1)//u is terminal
							{
								t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());
								for (auto& [v, vt] : u.proposals)
								{
									nodes[v].new_parent.tree = u.T.size() - 1;
								}
								u.proposals.clear();
							}
							t.height = t.grow;
							for (auto& [v, vt] : t.children)//downcast ACCEPT (and accept proposals if u is terminal)
							{
								nodes[v].mailbox.downcast_accept.push_back(vt);
							}
						}
						//process upcasts
						for (UpcastMessage& m : u.mail.upcast)//TODO need revision
						{//TODO enter root logic!!!!
							Tree& t = u.T[m.tree];
							t.prop_count += m.value;
							//next line is equivalent to: if(m.grow > t.grow) t.grow = m.grow;
							t.grow = m.grow * (m.grow > t.grow) + t.grow * !(m.grow > t.grow);
							if (m.tree)
								nodes[t.parent.id].mailbox.upcast.emplace_back(t.parent.tree, m.value, t.grow + 1);
							else//u is root
							{

							}
						}
						u.mail.upcast.clear();
						//check the tree that u is his root
						Tree& t = u.T.front();
						if (round == t.height + 1)
						{
							if (t.prop_count < u.tokens / (28 * (B + LOGN)))//kill
							{
								if (u.T.size() == 1)//u is terminal
								{
									for (auto& [v, vt] : u.proposals)//kill proposals
									{
										nodes[v].mailbox.downcast_kill.emplace_back(v, vt);
									}
									u.proposals.clear();									
									if (u.id == u.new_parent.id)
										stalled_nodes.push_back(idx);//need to increase u.level later					
								}
								for (auto& [v, vt] : t.children)//downcast KILL
								{
									nodes[v].mailbox.downcast_kill.emplace_back(idx, vt);
								}
								u.tokens -= t.prop_count * 14 * (B + LOGN);
								t.height = t.grow * (t.grow < t.height) + t.height * !(t.grow < t.height);
							}
							else//accept
							{
								for (auto& [v, vt] : t.children)//downcast ACCEPT (and accept proposals if u is terminal)
								{
									nodes[v].mailbox.downcast_accept.emplace_back(idx, vt);
								}
								u.tokens += t.prop_count;
								if (u.T.size() == 1)//u is terminal
								{
									for (auto& [v, vt] : u.proposals)
										nodes[v].mailbox.downcast_accept.push_back(0);
									t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());//accept children
									u.proposals.clear();
									t.height = t.grow;//????????????problem
								}
							}
							t.grow = 0;
							t.prop_count = 0;
						}
						//send upcasts (or a downcast if u is root)
						u.mail.upcast.clear();

					}//node
					//updates stage: (maybe at end of phase?)
					//send ACCEPTED messages

						//process ACCEPTED messages
					for (AcceptedMessage& m : u.mail.accepted)
					{
						Neighbor& v = u.neighbors[m.id];
						v.label = m.label;
						v.level = m.level;
						v.state = ACTIVE;
					}

					//process STALLED messages
					for (int& m : u.mail.stalled)
					{
						Neighbor& v = u.neighbors[m];
						v.level += 1;
						v.state = STALLING;
					}
					//process KILLED messages
					for (int& m : u.mail.killed)
					{
						u.neighbors[m].state = KILLED;
					}

				}//round
			}//step
			for (auto& idx : stalled_nodes)
			{
				Node& u = nodes[idx];


			}
		}//phase
	}//color
}
/*TODO:
1. eliminate non-terminal branches by upcasting from non-terminal leaves in round 1.
each round, every dead node checks its max height.
if it's 0, it upcasts height of 0 and deletes itself from the tree.
*/
void Graph::Node::make_proposals()
{

}