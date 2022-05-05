#include <TestList.hpp>
#include <iostream>

int main(int argc, char** argv) {
	if (argc == 1) {
		std::cerr << "Enter a test name.\n";
		return EXIT_FAILURE;
	}
	for (auto& test : testFunctions) {
		if (argv[1] == test.name) {
			test.function();
			return 0;
		}
	}
	std::cerr << "Test not found.\n";
	return EXIT_FAILURE;
}