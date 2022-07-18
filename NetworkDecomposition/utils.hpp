#pragma once
#include "NetworkDecomposition.hpp"
namespace sz_nd{
void test(const std::string& type, size_t n, size_t count) {
	for (size_t i = 1; i <= count; ++i) {
		std::ofstream f(type + '_' + std::to_string(n) + '#' + std::to_string(i) + ".txt");
		try {
			Graph g(n, type, f);
			g.decompose();
			f.close();
		} catch (const std::exception& e) {
			f << type << " n = " << n << ": " << e.what() << std::endl;
			f.close();
			std::cout << type << " n = " << n << ": " << e.what() << std::endl;
			throw;
		}
	}
}

unsigned int log2(unsigned int v)
{
	unsigned int r = 0;
	while (v >>= 1) r++;
	return r;
}
}