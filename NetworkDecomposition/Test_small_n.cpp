#include "utils.hpp"
using namespace sz_nd;
int main() {
	std::string type = "chain";
	try {
		test(type, 32, 1);
	} catch (const std::exception& e) {}

	//sz_nd::Graph g(1000, clique_tag{}, f);
	//g.print_nodes();
	//g.decompose();
	//std::cout << "colors:" << std::endl;
	//for(uint i = 0; i < colors.size(); ++i) {
	//	std::cout << colors[i] << " ";
	//}
}//namespace sz_nd