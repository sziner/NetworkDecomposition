#include "structs.hpp"
#include "utils.hpp"
#include <algorithm>
#include <ranges>
#include <array>
//#include <iterator>
#include <numeric>

#define MAX_N UINT32_MAX
namespace sz_nd
{

int Graph::networkdecomposition()
{
	const uint32_t N = nodes.size();
	if(N >= MAX_N) return 1;
	const uint32_t LOGN = log2(N);
	const uint32_t B = LOGN;
	uint32_t max_height = 0;
	uint32_t new_max_height = 0;
	vector<int> colors;
	colors.reserve(N);
	//int num_unfinished = N;//to end early
	vector<vector<int>> new_adj;
	std::array<vector<int>, 4> node_ids;
	vector<int> active_nodes;
	vector<int> stalled_nodes;//to send update to neighbors at the end of a step
	vector<int> finished_nodes;
	vector<int> accepted_nodes;//to send update to neighbors at the end of a step
	vector<int> killed_nodes;
	vector<int> living_nodes;
	vector<queue_entry> leaf_nodes;
	living_nodes.reserve(N);
	for(int i = 0; i < N; ++i)
		living_nodes.push_back(i);

	vector<cluster> clusters;





	vector<queue_entry> Q[2];

	for(vector<queue_entry>& q : Q)
	{
		q.reserve(N);
		for(int i = 0; i < N; ++i)
			q.emplace_back(i, i);
	}

	robin_hood::unordered_map<int, tree> T;
	//robin_hood::unordered_map<tree_idx, down_tree> Tdown;
	vector<Message> proposals;
	long long sum_of_rounds = 0;
	for(int c = 0; c < LOGN; ++c)
	{
		for(int phase = 0; phase < 2 * (B + LOGN); ++phase)
		{
			for(int step = 0; step < 28 * (B + LOGN); ++step)
			{
				int round = 0;//first round, PROPOSE messages
				auto c = {std::ref(active_nodes), std::ref(stalled_nodes)};
				for(auto&& ids : node_ids)
					for(auto uid : ids)
					{
						auto uit = nodes.find(uid);
						Node& u = uit->second;
						//check for neighbors
						auto vit = uit;
						for(auto it : u.active_neighbors | std::ranges::views::transform([&](auto const& x) { return nodes.find(x); }))
						{
							if(it->second < vit->second)
							{
								vit = it;
							}
						}
						if(vit != uit)
						{
							Node& v = (*vit).second;
							tree& tv = T[v.label];
							tv.proposals.emplace_back(uid, v.depth + 1);
							tree& tu = T[u.label];
							tu.node_count -= 1;
							//tu[uid].height = -1;
						}
					}
				round = 1;
				for(auto it = T.begin(), end = T.end(); it != end;)
				{
					auto& [lbl, t] = *it;
					int div = (28 * (B + LOGN));
					int threshold = t.tokens / div, rem = t.tokens % div;
					int p = t.proposals.size();
					bool is_accepting = ((p > threshold) || (p == threshold && 0 == rem));
					t.tokens += (is_accepting) ? p : p * (-14 * (B + LOGN));
					t.node_count += p * is_accepting;
					t.proposals.clear();
					if(t.node_count == 0)
					{
 //do stuff

						it = T.erase(it);
						continue;
					}
					++it;
				}

				{
					int i = round % 2;
					Q[i] = std::move(leaf_nodes);
					while(!Q[i].empty())
					{
						{
							int label = -1;
							tree* t = nullptr;
							for(auto& [lbl, uid] : Q[i])
							{
								if(label != lbl)
								{
									label = lbl;
									t = &T[label];
								}
								tree_node& tn = (*t)[uid];
								tn.height += 2 * (tn.height * tn.prop_count < 0);
								tree_node& ptn = (*t)[tn.parent];
								ptn.prop_count += tn.prop_count;
								ptn.height = std::max(ptn.height, tn.height + 1 * (tn.height != -1));
								if(ptn.parent != lbl)
								{
									Q[1 - i].emplace_back(lbl, tn.parent);
								}
							}
						}
						Q[i].clear();
						++round;
						i = 1 - i;
					}
					for(auto it = T.begin(), end = T.end(); it != end;)
					{
						auto& [lbl, t] = *it;
						tree_node& r = t[lbl];
						if(-1 == r.height)
						{
							//do stuff

							it = T.erase(it);
							continue;
						}
						int divid = (28 * (B + LOGN));
						int threshold = t.tokens / divid, rem = t.tokens % divid;
						bool is_accepting = ((r.prop_count > threshold) || (r.prop_count == threshold && 0 == divid));
						//t.node_count += r.prop_count;
						int v = r.first_child;
						if(v == -1)
						{
							leaf_nodes.emplace_back(lbl, lbl);
						}
						else
						{
							do
							{
								tree_node& vtn = t[v];
								vtn.prop_count = is_accepting;
								Q[1 - i].emplace_back(lbl, v);
							} while(v != -1);
						}
						r.height = 0;
						r.prop_count = 0;
						++it;
					}

				}
				//downcast
				///////////////////////////////////////////////////////////////////////
				round = 1;//second round. recieve proposals. leaf nodes begin upcasting.
				{
					if(!proposals.empty())
					{
						auto first = proposals.begin(), last = proposals.end();
						int sum = 1;
						int lbl = first->label;
						int id = first->id;
						tree& t = T[lbl];
						++first;
						for(; first != last; ++first)
						{
							if(id != first->id)
							{
								tree_node parent = t[id];
								if(parent_id == MAX_N)
								{
									t[parent_id] += sum;
								}
								else
									Q[(round + 1) % 2].emplace_back(lbl, parent_id, sum);
								if(lbl != first->label)
								{
									lbl = first->label;
									t = T[lbl];
								}
								id = first->id;
								sum = 1;
							}
							else
							{
								++sum;
							}
						}
						Q[(round + 1) % 2].emplace_back(lbl, t[id], sum);
						++round;
					}
				}
				for(int i = round % 2; !Q[i].empty(); ++round, i = 1 - i)
				{
					auto first = Q[i].begin(), last = Q[i].end();
					int lbl = first->label;
					int id = first->id;
					int sum = first->val;
					up_tree& t = Tup[lbl];
					++first;
					for(; first != last; ++first)
					{
						if(id != first->id)
						{
							Q[(round + 1) % 2].emplace_back(lbl, t[id], sum);
							if(lbl != first->label)
							{
								lbl = first->label;
								t = Tup[lbl];
							}
							id = first->id;
							sum = 1;
						}
						else
						{
							++sum;
						}
					}
					Q[1 - i].emplace_back(lbl, t[id], sum);
				}
				for(auto& [lbl, id, src] : proposals)
				{//only PROPOSE messages


					Node& u = nodes[id];
					Tree& t = u.T.back();//the current tree
					t.prop_count += u.proposals.size();
					t.hmax = 1 * (t.prop_count != 0);
					if(t.height == 0)//u is a leaf
					{
						if(u.T.size() == 1)//u is a terminal root leaf. process proposals
						{
							if(t.prop_count < u.tokens / (28 * (B + LOGN)))
							{//KILL
								u.tokens -= t.prop_count * 14 * (B + LOGN);
								for(auto& [v, vt] : u.proposals)//kill proposals
								{
									nodes[v].kill();
								}
								if(!u.is_proposing())//u has not proposed
								{
									if(u.level < B - 1)//u is not FINISHED
									{
										stalled_nodes.push_back(id);//become STALLED
									}
									else//u is FINISHED
									{
										finished_nodes.push_back(id);//become FINISHED
									};
								}
							}
							else
							{//accept
								u.tokens += t.prop_count;
								t.children.insert(t.children.end(), u.proposals.begin(), u.proposals.end());
								for(auto& [v, vt] : u.proposals)
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

				for(round = 2; round < max_height + 1; ++round)//upcast
				{
					for(auto& [idx, u] : nodes)
					{
						//process UPCAST messages
						if(!(u.mail.upcast.empty()))
						{
							for(UpcastMessage& m : u.mail.upcast)
							{
								Tree& t = u.T[m.tree];
								t.prop_count += m.p_count;
								t.hmin = std::max(t.hmin, m.min);
								t.hmax = std::max(t.hmin, m.min);
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
												if(u.level < B - 1)//u is not FINISHED
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
										max_height = std::max(max_height, t.height);
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
				for(; round < 2 * (max_height + 1); ++round)//downcast
				{
					for(auto& [idx, u] : nodes)
					{
						//process DOWNCAST_KILL messages
						for(DowncastMessage& m : u.mail.downcast_kill)
						{
							Tree& t = u.T[m];
							if(t.height != 0)//u is not a leaf. TODO: may becom redundant if children are removed.
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
									if(u.level < B - 1)//u has not proposed and isn't FINISHED
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
					}//node
				}//round

			}//step
			for(auto& idx : stalled_nodes)
			{
				Node& u = nodes[idx];

			}
			active_nodes.insert(active_nodes.end(), stalled_nodes.begin(), stalled_nodes.end());
			stalled_nodes.clear();
		}//phase
	}//color
}


}//namespace sz_nd