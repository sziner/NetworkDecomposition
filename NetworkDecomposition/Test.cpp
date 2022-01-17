#include "NetworkDecomposition.hpp"
using namespace sz_nd;
constexpr size_t VAR = 1000;
std::ofstream f("log.txt");
int main() {
	sz_nd::Graph<VAR> g(f, chain_tag{});
	//g.print_nodes();
	auto colors = g.decompose();
	//std::cout << "colors:" << std::endl;
	//for(uint i = 0; i < colors.size(); ++i) {
	//	std::cout << colors[i] << " ";
	//}
}//namespace sz_nd