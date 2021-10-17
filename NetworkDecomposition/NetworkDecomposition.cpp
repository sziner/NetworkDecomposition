#include "structs.hpp"
#include "utils.hpp"
#include <algorithm>
#include <ranges>
#include <array>
//#include <iterator>
#include <numeric>

#define MAX_N UINT_MAX
namespace sz_nd {

int Graph::networkdecomposition(vector<uint>& colors) {
	const uint N = nodes.size();
	if(N >= MAX_N) return 1;
	const uint32_t LOGN = log2(N);
	const uint32_t B_LOGN = LOGN + LOGN;
	uint32_t max_height = 0;
	colors.reserve(N);
	//uint num_unfinished = N;//to end early
	//vector<vector<uint>> new_adj;
	std::array<vector<uint>, 4> node_ids;
	vector<uint> active_nodes;
	vector<uint> killed_nodes;
	vector<uint> living_nodes;
	living_nodes.reserve(N);
	for(uint i = 0; i < N; ++i)
		living_nodes.push_back(i);
	living_nodes.insert(std::views::iota(0u, N));

	robin_hood::unordered_map<uint, cluster> clusters;
	bool growth;

	long long sum_of_rounds = 0;
	for(uint c = 0; c < LOGN; ++c) {
		for(uint phase = 0; phase < 2 * (B_LOGN); ++phase) {
			for(uint step = 0; step < 28 * (B_LOGN); ++step) {
				growth = false;
				uint round = 0;//first round, PROPOSE messages
				for(auto&& ids : node_ids | std::views::take(node_ids.size() - 1)) {
					Node v;
					for(auto uit : std::ranges::transform(ids,[&](auto x){return nodes.find(x);})) {
						//check for neighbors
						uint uid = uit->first;
						Node& u = uit->second;
						auto vit = uit;
						
						for(auto id : adj[uid]) {
							auto tmp = nodes.find(id);
							if(tmp->second < vit->second) {
								vit = tmp;
							}
						}

						if(vit->first != uid) {
							v = vit->second;
							u = v;
							u.depth++;
							cluster& cv = clusters[v.label];
							cv.proposals.emplace_back(uid, u.depth);
							cluster& cu = clusters[u.label];
							cu.node_count--;
						}
					}
				}
				for(auto it = clusters.begin(), end = clusters.end(); it != end;) {
					auto& [lbl, c] = *it;
					const uint div = 28 * B_LOGN;
					const uint threshold = c.tokens / div, rem = c.tokens % div;
					const uint p = c.proposals.size();
					if((p > threshold) || (p == threshold && 0 == rem)) {
						c.tokens += p;
						if(!growth) {
							const uint new_max_height = max_height + 1
							for(auto& [cld , dpth] : c.proposals) {
								Node& child = nodes[cld];
								if(new_max_height == dpth) {
									max_height = new_max_height;
									growth = true;
									break;
								}
							}
						}
					} else {

						if(c.node_count == 0) {
							it = clusters.erase(it);
							continue;
						}
						c.tokens -= p * 14 * B_LOGN;
					}
					c.proposals.clear();
					++it;
				}
			}//step
			for(auto& idx : stalled_nodes) {
				Node& u = nodes[idx];
			}
			active_nodes.insert(active_nodes.end(), stalled_nodes.begin(), stalled_nodes.end());
			stalled_nodes.clear();
		}//phase
	}//color
}

}//namespace sz_nd