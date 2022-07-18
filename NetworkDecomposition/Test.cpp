#include "NetworkDecomposition.hpp"
using namespace sz_nd;
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

void small_n() {
	std::string type = "chain";
	try {
		test(type, 8, 1);
		test(type, 16, 1);
		test(type, 32, 1);
	} catch (const std::exception& e) {}

	type = "ring";
	try {
		test(type, 8, 1);
		test(type, 16, 1);
		test(type, 32, 1);
	} catch (const std::exception& e) {}

	type = "clique";
	try {
		test(type, 8, 1);
		test(type, 16, 1);
		test(type, 32, 1);
	} catch (const std::exception& e) {}

	type = "random";
	try {
		test(type, 8, 1);
		test(type, 16, 1);
		test(type, 32, 1);
	} catch (const std::exception& e) {}
}

void big_n() {
	std::string type;
	type = "chain";
	try {
		test(type, 1000, 5);
		test(type, 10'000, 5);
		test(type, 100'000, 5);
		test(type, 500'000, 5);
		test(type, 1'000'000, 5);
	} catch (const std::exception& e) {}

	type = "ring";
	try {
		test(type, 1000, 5);
		test(type, 10'000, 5);
		test(type, 100'000, 5);
		test(type, 500'000, 5);
		test(type, 1'000'000, 5);
	} catch (const std::exception& e) {}

	type = "clique";
	try {
		test(type, 1000, 5);
		test(type, 10'000, 5);
		test(type, 100'000, 5);
		test(type, 500'000, 5);
		test(type, 1'000'000, 5);
	} catch (const std::exception& e) {}
	
	type = "random";
	try {
		test(type, 1000, 5);
		test(type, 10'000, 5);
		test(type, 100'000, 5);
		test(type, 500'000, 5);
		test(type, 1'000'000, 5);
	} catch (const std::exception& e) {}
}

int main() {
	small_n();
	big_n();
	
}//namespace sz_nd