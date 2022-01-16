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
enum state_t : uint8_t { ACTIVE, STALLING, FINISHED, KILLED };

using std::vector;
//using std::cout;
using std::endl;
using uint = uint32_t;
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

class Graph {
	const uint N;
	robin_hood::unordered_map<uint, Node> nodes;
	robin_hood::unordered_map<uint, uint> depths;
	robin_hood::unordered_map<uint, cluster> clusters;
	robin_hood::unordered_map<uint, uint> node_map;
	std::ostream& cout;

public:
	Graph(std::ostream& os, const uint n, const vector<uint>& ids, const vector<bool>& adj) : N(n), cout(os) {
		init(ids, adj);
	}
	Graph(std::ostream& os, const uint n, const vector<uint>& ids) : N(n), cout(os) {
		init(ids, spanning_tree(n, (n* (n - 1)) / 4));
	}
	Graph(std::ostream& os, const uint n) : N(n), cout(os) {
		init(generate_ids(n), spanning_tree(n, (n * (n - 1)) / 4));
	}
	
	void init(const vector<uint>& ids, const vector<bool>& adjmat) {
		//if(N >= MAX_N) return;
		nodes.reserve(N);
		depths.reserve(N);
		clusters.reserve(N);
		node_map.reserve(N);
		for(uint i = 0, s = ids.size(); i < s; ++i) {
			auto [it, b] = nodes.emplace(ids[i], Node{ids[i], {}});
			//std::ranges::transform(adj[i], std::back_inserter(it->second.neighbors), [&](auto x) {return ids[x]; });
			
			for (uint j = 0; j < N; ++j) {
				if (adjmat[i*N+j] == true) {
					it->second.neighbors.push_back(ids[j]);
				}
			}
			depths.emplace(ids[i], 0);
			clusters.emplace(ids[i], vector<uint>(1, ids[i]));
			node_map.emplace(ids[i], i);
		}
	}

	vector<vector<uint>> generate_edges(uint n) {
		vector<vector<uint>> adjlist(n);
		std::mt19937 gen{std::random_device{}()};
		std::bernoulli_distribution coinflip(P);
		cout << "creating adjacency matrix/list..." << std::endl;
		for(uint i = 0; i < n; ++i) {
			/*for(uint j = 0; j < i; ++j) {
				std::cout << "  ";
			}
			std::cout << "0 ";*/
			for(uint j = i + 1; j < n; ++j) {
				if((j == i + 1) || coinflip(gen)) {
					adjlist[i].push_back(j);
					adjlist[j].push_back(i);
				}
			}
			//std::cout << std::endl;
		}
		cout << "done" << std::endl;
		//print_adjlist(adjlist);
		return adjlist;
	}
	vector<vector<uint>> generate_ring(uint n) {
		vector<vector<uint>> adjlist(n, {0, 0});
		std::cout << std::endl << "creating adjacency matrix/list..." << std::endl;
		adjlist[0][0] = n - 1;
		adjlist[0][1] = 1;

		for(uint i = 1; i < n-1; ++i) {
			adjlist[i][0] = i - 1;
			adjlist[i][1] = i + 1;
		}
		adjlist[n-1][0] = n - 2;
		adjlist[n-1][1] = 0;
		cout << "done" << endl;
		//print_adjlist(adjlist);
		return adjlist;
	}

	vector<vector<uint>> generate_chain(uint n) {
		vector<vector<uint>> adjlist(n);
		std::cout << std::endl << "creating adjacency matrix/list..." << std::endl;
		adjlist[0].push_back(1);

		for(uint i = 1; i < n - 1; ++i) {
			adjlist[i].reserve(2);
			adjlist[i].push_back(i - 1);
			adjlist[i].push_back(i + 1);
		}
		adjlist[n - 1].push_back(n - 2);
		cout << "done" << endl;
		//print_adjlist(adjlist);
		return adjlist;
	}

	vector<vector<uint>> generate_clique(uint n) {
		std::cout << std::endl << "creating adjacency matrix/list..." << std::endl;
		vector<vector<uint>> adjlist(n);
		for(uint i = 0; i < n; ++i) {
			uint64_t m = n;
			adjlist[i].reserve(m - 1);
			for(uint j = 0; j < i; ++j) {
				adjlist[i].push_back(j);
			}
			for(uint j = i + 1; j < n; ++j) {
				adjlist[i].push_back(j);
			}
		}
		cout << "done" << endl;
		//print_adjlist(adjlist);
		return adjlist;
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

	vector<uint> generate_ids(uint n) {
		std::mt19937 gen{std::random_device{}()};
		std::uniform_int_distribution<uint> unidist(0, MAX_N - 1);
		vector<uint> idlist;
		idlist.reserve(n);
		uint id = 0;
		for(uint i = 0; i < n; ++i) {
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
	
	vector<bool> spanning_tree(uint n, uint e) {
		cout << "creating adjacency matrix/list..." << std::endl;
		vector<vector<uint>> adj(n);
		//build a spanning tree
		robin_hood::unordered_set<uint> S,T;
		S.reserve(n);
		T.reserve(n);
		for(uint i = 0; i < n;++i) {
			S.insert(i);
		}
		std::mt19937 gen{std::random_device{}()};
		std::uniform_int_distribution unidist1(0u, n-1);
		uint current, neighbor;
		current = unidist1(gen);
		S.erase(current);
		T.insert(current);
		while(!S.empty()) {
			neighbor = unidist1(gen);
			if(!T.contains(neighbor)) {
				adj[current].push_back(neighbor);
				adj[neighbor].push_back(current);
				S.erase(neighbor);
				T.insert(neighbor);
			}
			current = neighbor;
		}
		//build a matrix
		vector<bool> mat((unsigned long long int)n * (unsigned long long int)n);
		std::bernoulli_distribution coinflip(P);
		for(uint i = 0; i < n; ++i) {
			for(uint j = 0; j < i; ++j) {
				mat[i * n + j] = coinflip(gen);
			}
			mat[i * n + i] = false;
			for(uint j = i+1; j < n; ++j) {
				mat[i * n + j] = coinflip(gen);
			}
		}
		//combine
		for(uint i = 0; i < n; ++i) {
			for(uint j : adj[i]) {
				mat[i * n + j] = true;
			}
		}
		//convert to list
		/*for (uint i = 0; i < n; ++i) {
			adj[i].clear();
			for(uint j = 0; j < n; ++j) {
				if(mat[i * n + j] == true) {
					adj[i].push_back(j);
				}
			}
			adj[i].shrink_to_fit();
		}*/
		
		//we have a spanning tree. now we add random edges
		/*std::uniform_int_distribution unidist2(0u, n - 2);
		for(uint count = n - 1; count < e;) {
			current = unidist1(gen);
			neighbor = unidist2(gen);
			neighbor += 1 * (neighbor >= current);
			auto currentit = std::ranges::find(adj[current], neighbor);
			if(currentit == adj[current].end()) {
				adj[current].push_back(neighbor);
				adj[neighbor].push_back(current);
				++count;
			}
		}*/
		//print_adjlist(adj);
		cout << "done" << std::endl;
		return mat;
	}
	
	vector<uint8_t> decompose();

};
}//namespace sz_nd