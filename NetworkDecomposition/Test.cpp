constexpr unsigned int VAR = 90000;
#include "structs.hpp"
std::ofstream f("log.txt");
int main() {
	using namespace sz_nd;
	vector<uint> ids;
	ids.reserve(VAR);
	for(uint i = 0; i < VAR; ++i) {
		ids.push_back(i);
	}
	std::mt19937 engine{std::random_device{}()};
	std::ranges::shuffle(ids, engine);
	
	sz_nd::Graph g(f, VAR, ids);
	//g.print_nodes();
	auto colors = g.decompose();
	//std::cout << "colors:" << std::endl;
	//for(uint i = 0; i < colors.size(); ++i) {
	//	std::cout << colors[i] << " ";
	//}
}//namespace sz_nd