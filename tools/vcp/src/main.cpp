#include <iostream>
#include <string_view>
#include <vector>

#include <PipelineArchetypeRecord.hpp>
#include <ProjectDatabase.hpp>

#include <unordered_map>

std::vector<std::vector<std::string_view>> parseCommandLineArguments(int argc, char** argv) {
	std::vector<std::vector<std::string_view>> result;
	for (int i = 1; i < argc; ++i) {
		if (*(argv[i]) == '-') {
			char* argStart = argv[i];
			while (*argStart == '-')
				++argStart;
			result.push_back({ argStart });
		} else {
			if (result.empty()) {
				std::cout << "ERROR: The first argument must be prefixed with \'-\'\n";
				return {};
			}
			result.back().push_back(argv[i]);
		}
	}
	return result;
}

int main(int argc, char** argv) {
	auto arguments = parseCommandLineArguments(argc, argv);
	if (arguments.empty()) {
		std::cout << "No arguments found, exiting..." << std::endl;
		return EXIT_FAILURE;
	}

	PipelineType pipelineType = PipelineType::Graphics;
	PipelineArchetypeRecord archetypeRecord;

	for (auto& argument : arguments) {
	}
}