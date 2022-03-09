#include <iostream>
#include <string_view>
#include <vector>
#include <fstream>

#include <PipelineArchetypeRecord.hpp>
#include <ProjectDatabase.hpp>
#include <SysUtils.hpp>

#include <unordered_map>

struct Options {
	std::string_view fileName;
	std::string_view projectDir = ".";
	
};

size_t parseOption(int argc, char** argv, size_t index, Options& options) { 
	if (argv[index] == std::string_view("--working-dir")) {
		if (argc <= index + 1) {
			std::cout << "Warning: Not enough arguments given to --working-dir.\n";
		} else if (*argv[index + 1] == '-') {
			std::cout << "Warning: " << argv[index + 1] << "Invalid argument for --working-dir.\n ";
		} else {
			options.projectDir = argv[index + 1];
			return index + 1;
		}
	}
	return index;
}

Options parseArguments(int argc, char** argv) { 
	Options options;
	for (size_t i = 0; i < argc; ++i) {
		if (*argv[i] == '-') {
			i = parseOption(argc, argv, i, options);
			continue;
		} else if (!options.fileName.empty()) {
			std::cout << "Warning: The input filename was specified more than once. The last name will be chosen.\n";
		}
		options.fileName = argv[i];
	}
	return options;
}

int main(int argc, char** argv) {
	Options options = parseArguments(argc, argv);

	auto stream = std::ifstream(std::string(options.projectDir) + "/project.json", std::ios_base::binary);
	if (!stream.is_open()) {
		std::cout << "Error: No project.json found in the working directory!" << std::endl;
		return EXIT_FAILURE;
	}

	stream.ignore(std::numeric_limits<std::streamsize>::max());
	auto fileSize = stream.gcount();
	stream.clear();
	stream.seekg(0, std::ios_base::beg);

	char* data = reinterpret_cast<char*>(malloc(fileSize));
	stream.read(data, fileSize);

	Json::Value rootValue;

	Json::Reader reader;
	reader.parse(data, data + fileSize, rootValue);

	free(data);

	ProjectDatabase database = ProjectDatabase(rootValue);

	PipelineArchetypeRecord archetypeRecord;

	char* argument = new char[]{ "--help" };
	char* name = new char[]{ "glslc" };
	SubprocessID glslcId = startSubprocess(name, { argument });
	std::cout << "Successfully started glslc.\n";
	waitForSubprocess(glslcId);

	delete[] name;
	delete[] argument;
}