#include "structs.hpp"
#include "utils.hpp"
//#include <numeric>

namespace sz_nd {

vector<uint8_t> Graph::decompose() {
	const uint B = std::bit_width(std::ranges::max(node_map | std::views::transform([](auto x) {return x.first; })));
	const uint LOGN = std::bit_width(N);
	const uint B_LOGN = B + LOGN;
	cout << "N= " << N << ", B= " << (uint) B << ", LOGN= " << (uint) LOGN << endl << "56 * (B + LOGN) ^ 2 = " << 56 * B_LOGN * B_LOGN;
	vector<uint8_t> colors(N, uint8_t(-1));
	vector<uint> color_count;
	uint active_clusters = N;
	uint stalling_clusters = 0;
	unsigned long long messages = 0;
	unsigned long long calculated_sum_of_rounds = 0;
	unsigned long long actual_sum_of_rounds = 0;
	uint max_height = 0;
	uint new_max_height = 0;
	uint highest_cluster = 0;
	unsigned long long rounds = 0;

	for(uint8_t color = 0; !nodes.empty() && (color < LOGN); ++color) {
		cout << endl << "color " << (uint) color << ":" << endl;
		for(uint phase = 0; (active_clusters + stalling_clusters) && clusters.size() > 1 && (phase < 2 * B_LOGN); ++phase) {
			cout << endl << "phase " << phase << ":" << endl
				<< "clusters: " << clusters.size() << endl << "max height: " << max_height << " id: " << highest_cluster << endl;
			std::cout << endl << "phase " << phase << ":" << endl
				<< "clusters: " << clusters.size() << endl << "max height: " << max_height << " id: " << highest_cluster << endl;
			//print_clusters();
			for(uint step = 0; active_clusters && (step < 28 * B_LOGN); ++step) {
				/*cout << endl << "step " << step << ":" << std::endl
					<< "clusters: " << clusters.size() << std::endl;
				print_clusters();*/
				max_height = 0;
				for(auto ucit = clusters.begin(); ucit != clusters.end(); ++ucit) {
					std::erase_if(ucit->second.nodes, [&](uint uid) {
						auto uit = nodes.find(uid);
						auto vit = uit;
						auto vcit = ucit;
						//choose a neighbor to propose to
						for(auto tmpid : uit->second.neighbors) {
							auto tmpit = nodes.find(tmpid);
							if((tmpit->second.label != MAX_N) && (tmpit->second.label != vit->second.label)) {
								auto tmpcit = clusters.find(tmpit->second.label);
								if((tmpcit->second.state == ACTIVE)
								   && (tmpcit->second.level < vcit->second.level
									   || (tmpcit->second.level == vcit->second.level
										   && !((vcit->first >> vcit->second.level) & 1u)
										   && ((tmpcit->first >> tmpcit->second.level) & 1u)))) {
									vit = tmpit;
									vcit = tmpcit;
								}
							}
						}
						if(vit != uit) {
							vcit->second.proposals.emplace_back(uit->first, depths[vit->first] + 1);
							return true;
						} else {
							return false;
						}
					});
				}

				//checking clusters			
				for(auto it = clusters.begin(); it != clusters.end();) {
					auto& [lbl, c] = *it;
					if(c.proposals.empty()) {
						if(c.state == ACTIVE) {
							c.state = STALLING;
							--active_clusters;
							++stalling_clusters;
						}
						if(c.nodes.empty()) {
							if(c.state == STALLING) {
								--stalling_clusters;
							}
							it = clusters.erase(it);
							continue;
						}
					} else {
						const uint p = c.proposals.size();
						const uint div = 28 * B_LOGN;
						const uint threshold = c.tokens / div;
						const uint rem = c.tokens % div;
						if(p > threshold || (p == threshold && 0 == rem)) {//accept
							c.tokens += p;
							c.nodes.reserve(c.nodes.size() + p);
							for(auto& [cld, dpth] : c.proposals) {
								c.nodes.push_back(cld);
							}
							for(auto& [cld, dpth] : c.proposals) {
								nodes[cld].label = lbl;
							}
							for(auto& [cld, dpth] : c.proposals) {
								depths[cld] = dpth;
								if(c.depth < dpth) {
									c.depth = dpth;
								}
							}
							
						} else {//kill
							for(auto& [cld, dpth] : c.proposals) {
								nodes[cld].label = MAX_N;
							}
							if(c.nodes.empty()) {
								it = clusters.erase(it);
								--active_clusters;
								continue;
							}
							c.tokens -= p * 14 * B_LOGN;
							c.state = STALLING;
							--active_clusters;
							++stalling_clusters;
						}
						c.proposals.clear();
					}
					if(new_max_height < c.depth) {
						new_max_height = c.depth;
						highest_cluster = lbl;
					}
					++it;
				}
				rounds += (max_height + 1) * 2 + 1;//proposals + upcast + downcast + kill/accept + updates
				max_height = new_max_height;
				new_max_height = 0;
			}//step
			//print_clusters();
			for(auto& [lbl, c] : clusters) {
				if(c.state == STALLING) {
					--stalling_clusters;
					if(++c.level == B) {
						c.state = FINISHED;
					}
					else {
						c.state = ACTIVE;
						++active_clusters;
					}
				}
			}
		}//phase
		//print_clusters();
		for(auto& [uid, u] : nodes | std::views::filter([](auto& x) {return x.second.label == MAX_N; })) {
			std::erase_if(u.neighbors, [&](auto x) {return nodes[x].label != MAX_N; });
		}
		clusters.clear();
		active_clusters = 0;
		stalling_clusters = 0;
		depths.clear();
		uint count = 0;
		for(auto it = nodes.begin(); it != nodes.end();) {
			if(it->second.label == MAX_N) {
				it->second.label = it->first;
				depths.emplace(it->first, 0);
				clusters.emplace(it->first, vector<uint>(1, it->first));
				++active_clusters;
				++it;
			} else {
				colors[node_map[it->first]] = color;
				it = nodes.erase(it);
				++count;
			}
		}
		color_count.push_back(count);
	}//color
	cout << endl << "colors:" << endl;
	for(uint i = 0; i < color_count.size(); ++i) {
		cout << i << ": " << color_count[i] << endl;
	}
	cout << "rounds: " << rounds << endl;
	return colors;
}

}//namespace sz_nd