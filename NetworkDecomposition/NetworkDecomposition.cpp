#include "structs.hpp"
#include "utils.hpp"
#include <algorithm>
#include <iterator>
#include <numeric>

namespace sz_nd
{ 

int Graph::networkdecomposition()
{
	using std::max;

	const unsigned N = nodes.size();
	const unsigned LOGN = log2(N);
	const int B = LOGN;
	int max_height = 0;
	int new_max_height = 0;
	//int num_unfinished = N;//to end early
	std::vector<int> accepted_nodes;//to send update to neighbors at the end of a step
	std::vector<int> stalled_nodes;//to send update to neighbors at the end of a step
	std::vector<int> finished_nodes;
	std::vector<int> killed_nodes;
	std::vector<int> active_nodes;
	std::vector<int> living_nodes;
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
					Node& u = nodes[idx];

					//check for neighbors
					Neighbor& v = *std::min_element(u.neighbors[ACTIVE].begin(), u.neighbors[ACTIVE].end());
					if (v < u)
					{
						u.new_parent.id = v.id;
						nodes[v.id].proposals.emplace_back(idx,u.T.size());
					}
				}
				///////////////////////////////////////////////////////////////////////
				round = 1;//second round. recieve proposals. leaf nodes begin upcasting.
				for (int& idx : living_nodes)
				{//only PROPOSE messages
					Node& u = nodes[idx];
					Tree& t = u.T.back();//the current tree
					t.prop_count += u.proposals.size();
					t.hmax = 1 * (t.prop_count != 0);
					if (t.height == 0)//u is a leaf
					{
						if (u.T.size() == 1)//u is a terminal root. process proposals
						{
							if (t.prop_count < u.tokens / (28 * (B + LOGN)))
							{//KILL
								u.tokens -= t.prop_count * 14 * (B + LOGN);
								for (auto& [v,vt] : u.proposals)//kill proposals
								{
									nodes[v].kill();
								}
								if (!u.is_proposing())
								{
									stalled_nodes.push_back(idx);
								}
							}
							else
							{//accept
								u.tokens += t.prop_count;
								t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());
								for (auto& [v,vt] : u.proposals)
								{//accept proposals
									nodes[v].accept(u.T.size() - 1);
								}
								t.height = 1 * (t.prop_count != 0);
							}
							u.proposals.clear();
						}
						else//u is terminal non-root leaf. normal upcast.
						{
							t.hmax = (t.prop_count != 0);
							nodes[t.parent.id].mail.upcast.emplace_back(
								t.parent.tree,
								t.prop_count,
								!u.is_proposing(),
								t.hmax + (!u.is_proposing() || t.hmax != 0));
						}
						t.prop_count = 0;
					}			
				}
				///////////////////////////////////////////////////////////////////////////////////////////////////////////

				for (round = 2; round < 2 * (step + 1) + phase * 28 * (B + LOGN); ++round)//2*(step+1) rounds for upcast + downcast
				{
					for(auto& [idx, u] : nodes)
					{
						//process DOWNCAST_KILL messages
						for(DowncastMessage& m : u.mail.downcast_kill)
						{
							Tree& t = u.T[m];
							if(t.height != 0)//u is not a leaf. TODO if may becom redundant if children are removed.
							{
								for(auto& [v, vt] : t.children)//downcast KILL
								{
									nodes[v].mailbox.downcast_kill.push_back(vt);
								}
							}
							if(u.is_terminal(m))//u is terminal
							{
								for(auto& [v, vt] : u.proposals)//kill proposals
								{
									nodes[v].kill();
								}
								u.proposals.clear();

								if(!u.is_proposing())//change state to STALLED/FINISHED
								{
									//++u.level;//interferes with proposing on the next step
									if(u.level < B)//u has not proposed and isn't FINISHED
									{
										stalled_nodes.push_back(idx);//become STALLED
									}
									else
									{
										finished_nodes.push_back(idx);//become FINISHED
									}
								}
							}
							t.height = t.hmin;
							t.hmin = 0;
							t.hmax = 0;
						}
						//process DOWNCAST_ACCEPT messages
						for(DowncastMessage& t_idx : u.mail.downcast_accept)
						{
							Tree& t = u.T[t_idx];
							if(t.height > 0)//u is not a leaf
							{
								for(auto& [v, vt] : t.children)//downcast ACCEPT
								{
									nodes[v].mailbox.downcast_accept.push_back(vt);
								}
							}
							if(u.is_terminal(t_idx))
							{
								t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());//add children
								for(auto& nt : u.proposals)//accept_proposals()
								{
									nodes[nt.id].accept(u.T.size() - 1);
								}
								u.proposals.clear();
							}
							t.height = t.hmax;
							t.hmin = 0;
							t.hmax = 0;
						}
						//process UPCAST messages
						if(!(u.mail.upcast.empty()))
						{
							for(UpcastMessage& m : u.mail.upcast)
							{
								Tree& t = u.T[m.tree];
									t.prop_count += m.p_count;
									t.hmin = max(t.hmin, m.min);
									t.hmax = max(t.hmin, m.min);
									if(round == t.height + 1 && std::ranges::find(u.to_upcast, m.tree) == u.to_upcast.end())
									{
										u.to_upcast.push_back(m.tree);
									}
							}
							u.mail.upcast.clear();
							//upcast
							for(int& t_idx : u.to_upcast)
							{
								Tree t = u.T[t_idx];
								if(t_idx == 0)//u is root
								{
									if(t.prop_count < u.tokens / (28 * (B + LOGN)))
									{//KILL(just like regular downcast, except the tokens change)
										u.tokens -= t.prop_count * 14 * (B + LOGN);
										if(t.height > 0)
										{
											for(auto& [v, vt] : t.children)//downcast KILL
											{
												nodes[v].mailbox.downcast_kill.push_back(vt);
											}
										}
										t.height = t.hmin;
										if(u.is_terminal(t_idx))
										{
											for(auto& [v, vt] : u.proposals)//kill proposals
											{
												nodes[v].kill();
											}
											if(!u.is_proposing())//u has not proposed
											{
												++u.level;//???now or later?
												if(u.level < B)//u is not FINISHED
												{
													stalled_nodes.push_back(idx);//become STALLED
												}
												else//u is FINISHED
												{
													finished_nodes.push_back(idx);//become FINISHED
												}
											}
										}
									}
									else
									{// ACCEPT
										u.tokens += t.prop_count;
										if(t.height > 0)
										{
											for(auto& [v, vt] : t.children)//downcast ACCEPT
											{
												nodes[v].mailbox.downcast_accept.push_back(vt);
											}
										}
										if(u.is_terminal(t_idx))
										{
											for(auto& [v, vt] : u.proposals)//accept proposals
											{
												nodes[v].accept(t_idx);
											}
											t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());//add to children
										}
										t.height = t.hmax;
										max_tree_height = max(max_tree_height, t.height);
									}
									t.hmin = 0;
									t.hmax = 0;
									u.proposals.clear();
								}
								else
								{//upcast
									nodes[t.parent.id].mailbox.upcast.emplace_back(t.parent.tree, t.prop_count,
																				   t.hmin + 1 * (t.hmin != 0 || (u.is_terminal(t_idx) && !u.is_proposing())),
																				   t.hmax + 1 * (t.hmax != 0 || (u.is_terminal(t_idx) && !u.is_proposing())));
								}
								t.prop_count = 0;
							}
							u.to_upcast.clear();
						}
					}//node
					//updates stage: (maybe at end of phase?)

				}//round
			}//step
			for (auto& idx : stalled_nodes)
			{
				Node& u = nodes[idx];

			}
			active_nodes.insert(active_nodes.end(), stalled_nodes.begin(), stalled_nodes.end());
			stalled_nodes.clear();
		}//phase
	}//color
}


}//namespace sz_nd