#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <ranges>
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "robin_hood.hpp"
constexpr double P = 0.5;


namespace sz_nd {
enum state_t: uint8_t{ ACTIVE, STALLING, FINISHED, KILLED };
using std::vector;
//using std::cout;
using std::endl;
using uint = size_t;
constexpr uint MAX_N = std::numeric_limits<uint>::max();


struct Proposal {
	uint id;
	uint depth;
};

struct cluster {
	vector<uint> nodes;
	state_t state = ACTIVE;
	uint level = 0;
	uint depth = 0;
	uint tokens = 1;
	vector<Proposal> proposals;

};

struct Node {
	uint label;
	vector<uint> neighbors;
};
template <uint N>
class Graph {
	robin_hood::unordered_map<uint, Node> nodes;
	robin_hood::unordered_map<uint, uint> depths;
	robin_hood::unordered_map<uint, cluster> clusters;
	robin_hood::unordered_map<uint, uint> node_map;
	std::ostream& cout;

public:
	Graph( std::ostream& os, const vector<uint>& ids, const vector<bool>& adjmat) : cout(os) {
		init(ids, adjmat);
	}
	Graph(std::ostream& os, const vector<uint>& ids) : cout(os) {
		init(ids, spanning_tree());
	}
	Graph( std::ostream& os) : cout(os) {
		init(generate_ids(), spanning_tree());
	}
	
	void init(const vector<uint>& ids, const vector<bool>& adjmat) {
		nodes.reserve(N);
		depths.reserve(N);
		clusters.reserve(N);
		node_map.reserve(N);
		for (uint i = 0; i < N; ++i) {
			auto id = ids[i];
			depths.emplace(id, 0);
			clusters.emplace(id, vector<uint>(1, id));
			node_map.emplace(id, i);
			auto [it, b] = nodes.emplace(id, Node{ id, {} });
			for (uint j = 0; j < N; ++j) {
				if (adjmat[i * N + j] == true) {
					it->second.neighbors.push_back(ids[j]);
				}
			}
		}
	}

	vector<bool> generate_chain() {
		vector<bool> adjmat(N * N, false);
		cout << "creating adjacency matrix/list..." << std::endl;
		adjmat[1] = true;
		for(uint i = 1; i < N-1; ++i) {
			adjmat[i * N + i - 1] = true;
			adjmat[i * N + i + 1] = true;
		}
		adjmat[(N * N) - 2] = true;
		cout << "done" << std::endl;
		//print_adjlist(adjlist);
		return adjmat;
	}
	vector<bool> generate_ring() {
		vector<bool> adjmat(N * N, false);
		cout << "creating adjacency matrix/list..." << std::endl;
		adjmat[1] = true;
		adjmat[N - 1] = true;
		for (uint i = 1; i < N - 1; ++i) {
			adjmat[i * N + i - 1] = true;
			adjmat[i * N + i + 1] = true;
		}
		adjmat[N * (N - 1)] = true;
		adjmat[(N * N) - 2] = true;
		cout << "done" << std::endl;
		//print_adjlist(adjlist);
		return adjmat;
	}


	vector<bool> generate_clique() {
		std::cout << std::endl << "creating adjacency matrix/list..." << std::endl;
		vector<bool> adjmat(N * N, true);
		for (auto i: std::ranges::iota_view(0,N)) {//uint i = 0; i < N; ++i
			adjmat[i * N + i] = false;
		}
		cout << "done" << endl;
		//print_adjlist(adjlist);
		return adjmat;
	}
	void print_adjlist(const vector<vector<uint>>& adjlist) {
		std::cout << std::endl << "adjacency lists:" << std::endl;
		for(auto& i : adjlist) {
			for(auto&& j : i) {
				std::cout << j << " ";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	vector<uint> generate_ids() {
		std::mt19937 gen{std::random_device{}()};
		std::uniform_int_distribution<uint> unidist(0, MAX_N - 1);
		vector<uint> idlist;
		idlist.reserve(N);
		uint id = 0;
		for (uint i = 0; i < N; ++i) {
			do {
				id = unidist(gen);
			} while(std::ranges::find(idlist, id) != idlist.end());
			idlist.push_back(id);
		}
		return idlist;
	}

	void print_nodes() {
		std::cout << "nodes: (node : label -> neighbors)" << std::endl;
		for(auto& [uid,u] : nodes) {
			std::cout << node_map[uid] << " : " << u.label << " -> ";
			for(auto& v : u.neighbors) {
				std::cout << " " << node_map[v];
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	void print_clusters() {
		cout << "----------------------" << endl << "cluster\t" << "size\t" << "tokens\t" << "state" << endl <<"----------------------" << endl;
		for(auto& [lbl,c] : clusters) {
			cout << lbl << "\t" << c.nodes.size() << "\t" << c.tokens << "\t" << (((uint) c.state) ? "STALLED / FINISHED" : "ACTIVE") << endl;
			/*for(uint u : c.nodes) {
				cout << u << " ";
			}
			cout << endl;*/
		}
		cout << endl;
	}
	
	vector<bool> spanning_tree() {
		cout << "creating adjacency matrix/list..." << std::endl;
		//build a spanning tree adjacency list
		vector<vector<uint>> adjlist(N);
		std::mt19937 gen{ std::random_device{}() };
		{
			robin_hood::unordered_set<uint> S, T;
			S.reserve(N);
			T.reserve(N);
			for (uint i = 0; i < N; ++i) {
				S.insert(i);
			}
			std::uniform_int_distribution unidist1(0ULL, N - 1);
			uint current, neighbor;
			current = unidist1(gen);
			S.erase(current);
			T.insert(current);
			while (!S.empty()) {
				neighbor = unidist1(gen);
				if (!T.contains(neighbor)) {
					adjlist[current].push_back(neighbor);
					adjlist[neighbor].push_back(current);
					S.erase(neighbor);
					T.insert(neighbor);
				}
				current = neighbor;
			}
		}
		//build a matrix
		vector<bool> adjmat(N*N);
		std::bernoulli_distribution coinflip(P);
		for (auto i : std::ranges::iota_view(0ull, N)) {
			for(size_t j = 0; j < i; ++j) {
				adjmat[i * N + j] = coinflip(gen);
			}
			adjmat[i * N + i] = false;
			for(size_t j = i+1; j < N; ++j) {
				adjmat[i * N + j] = coinflip(gen);
			}
		}
		//combine
		for(uint i = 0; i < N; ++i) {
			for(uint j : adjlist[i]) {
				adjmat[i * N + j] = true;
			}
		}
		//print_adjlist(adj);
		cout << "done" << std::endl;
		return adjmat;
	}
	/// ///////////////////////////////////////////////////////////////////////
	vector<uint8_t> decompose() {
		const uint B = std::bit_width(std::ranges::max(node_map | std::views::transform([](auto x) {return x.first; })));
		const uint LOGN = std::bit_width(N);
		const uint B_LOGN = B + LOGN;
		cout << "N= " << N << ", B= " << (uint)B << ", LOGN= " << (uint)LOGN << endl << "56 * (B + LOGN) ^ 2 = " << 56 * B_LOGN * B_LOGN;
		vector<uint8_t> colors(N, uint8_t(-1));
		vector<uint> color_count;
		uint active_clusters = N;
		uint stalling_clusters = 0;
		unsigned long long messages = 0;
		unsigned long long calculated_sum_of_rounds = 0;
		unsigned long long actual_sum_of_rounds = 0;
		uint max_height = 0;
		uint new_max_height = 0;
		unsigned long long rounds = 0;

		for (uint8_t color = 0; !nodes.empty() && (color < LOGN); ++color) {
			cout << endl << "color " << (uint)color << ":" << endl;
			for (uint phase = 0; (active_clusters + stalling_clusters) && clusters.size() > 1 && (phase < 2 * B_LOGN); ++phase) {
				cout << endl << "phase " << phase << ":" << endl
					<< "clusters: " << clusters.size() << endl << "max height: " << max_height << endl;
				//print_clusters();
				for (uint step = 0; active_clusters && (step < 28 * B_LOGN); ++step) {
					/*cout << endl << "step " << step << ":" << std::endl
						<< "clusters: " << clusters.size() << std::endl;
					print_clusters();*/
					max_height = 0;
					for (auto ucit = clusters.begin(); ucit != clusters.end(); ++ucit) {
						std::erase_if(ucit->second.nodes, [&](uint uid) {
							auto uit = nodes.find(uid);
							auto vit = uit;
							auto vcit = ucit;
							//choose a neighbor to propose to
							for (auto tmpid : uit->second.neighbors) {
								auto tmpit = nodes.find(tmpid);
								if ((tmpit->second.label != MAX_N) && (tmpit->second.label != vit->second.label)) {
									auto tmpcit = clusters.find(tmpit->second.label);
									if ((tmpcit->second.state == ACTIVE)
										&& (tmpcit->second.level < vcit->second.level
											|| (tmpcit->second.level == vcit->second.level
												&& !((vcit->first >> vcit->second.level) & 1u)
												&& ((tmpcit->first >> tmpcit->second.level) & 1u)))) {
										vit = tmpit;
										vcit = tmpcit;
									}
								}
							}
							if (vit != uit) {
								vcit->second.proposals.emplace_back(uit->first, depths[vit->first] + 1);
								return true;
							}
							else {
								return false;
							}
						});
					}

					//checking clusters			
					for (auto it = clusters.begin(); it != clusters.end();) {
						auto& [lbl, c] = *it;
						if (c.proposals.empty()) {
							if (c.state == ACTIVE) {
								c.state = STALLING;
								--active_clusters;
								++stalling_clusters;
							}
							if (c.nodes.empty()) {
								if (c.state == STALLING) {
									--stalling_clusters;
								}
								it = clusters.erase(it);
								continue;
							}
						}
						else {
							const uint p = c.proposals.size();
							const uint div = 28 * B_LOGN;
							const uint threshold = c.tokens / div;
							const uint rem = c.tokens % div;
							if (p > threshold || (p == threshold && 0 == rem)) {//accept
								c.tokens += p;
								c.nodes.reserve(c.nodes.size() + p);
								for (auto& [cld, dpth] : c.proposals) {
									c.nodes.push_back(cld);
								}
								for (auto& [cld, dpth] : c.proposals) {
									nodes[cld].label = lbl;
								}
								for (auto& [cld, dpth] : c.proposals) {
									depths[cld] = dpth;
									if (c.depth < dpth) {
										c.depth = dpth;
									}
								}

							}
							else {//kill
								for (auto& [cld, dpth] : c.proposals) {
									nodes[cld].label = MAX_N;
								}
								if (c.nodes.empty()) {
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
						if (new_max_height < c.depth) {
							new_max_height = c.depth;
						}
						++it;
					}
					rounds += (max_height + 1) * 2 + 1;//proposals + upcast + downcast + kill/accept + updates
					max_height = new_max_height;
					new_max_height = 0;
				}//step
				//print_clusters();
				for (auto& [lbl, c] : clusters) {
					if (c.state == STALLING) {
						--stalling_clusters;
						if (++c.level == B) {
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
			for (auto& [uid, u] : nodes | std::views::filter([](auto& x) {return x.second.label == MAX_N; })) {
				std::erase_if(u.neighbors, [&](auto x) {return nodes[x].label != MAX_N; });
			}
			clusters.clear();
			active_clusters = 0;
			stalling_clusters = 0;
			depths.clear();
			uint count = 0;
			for (auto it = nodes.begin(); it != nodes.end();) {
				if (it->second.label == MAX_N) {
					it->second.label = it->first;
					depths.emplace(it->first, 0);
					clusters.emplace(it->first, vector<uint>(1, it->first));
					++active_clusters;
					++it;
				}
				else {
					colors[node_map[it->first]] = color;
					it = nodes.erase(it);
					++count;
				}
			}
			color_count.push_back(count);
		}//color
		cout << endl << "colors:" << endl;
		for (uint i = 0; i < color_count.size(); ++i) {
			cout << i << ": " << color_count[i] << endl;
		}
		cout << "rounds: " << rounds << endl;
		return colors;
	}

};
}//namespace sz_nd