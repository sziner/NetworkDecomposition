#include <random>
#include <vector>
#include "../NetworkDecomposition/structs.hpp"

using namespace std;
using sz_nd::uint;
vector<vector<uint>> generate(uint N) {
	vector<vector<uint>> adjlist;
	adjlist.reserve(N);
	adjlist.emplace_back(1, 1);
	for(uint i = 1; i < N - 2; ++i) {
		adjlist.emplace_back(2);
		adjlist[i][0] = i - 1;
		adjlist[i][1] = i + 1;
	}
	adjlist.emplace_back(1, N-2);
	default_random_engine defeng;
	uniform_int_distribution<uint> distr(0, 1);
	for(uint i = 0; i < N; ++i) {
		for(uint j = 0; j < i; ++j) {
			if(distr(defeng)) {
				adjlist[i].push_back(j);
			}
		}
		for(uint j = i + 1; j < N; ++j) {
			if(distr(defeng)) {
				adjlist[i].push_back(j);
			}
		}
	}
	return adjlist;
}