#include <fstream>
#include <helper/WholeFileReader.hpp>
#include <iostream>
#include <json/json.h>
#include <ParsingUtils.hpp>
#include <vector>

struct Options {
	std::string outFile;
	std::string inFile;
};

struct StringHeaderEntry {
	uint32_t offset;
	uint32_t size;
};

struct Font
{
	std::string name;
	std::vector<std::string> fontNames;
	StringHeaderEntry nameEntry;
	std::vector<StringHeaderEntry> fontEntries;
};


bool checkOption(int argc, char** argv, size_t index, const std::string_view& argName) {
	if (argc <= index + 1) {
		std::cout << "Warning: Not enough arguments given to " << argName << ".\n";
		return false;
	} else if (*argv[index + 1] == '-') {
		std::cout << "Warning: " << argv[index + 1] << "Invalid argument for " << argName << ".\n";
		return false;
	}
	return true;
}

size_t parseOption(int argc, char** argv, size_t index, Options& options) {
	if (argv[index] == std::string_view("-o")) {
		if (checkOption(argc, argv, index, "-o")) {
			options.outFile = argv[index + 1];
			return index + 1;
		}
	} else {
		std::cout << "Warning: Skipping unknown command line option " << argv[index] << ".\n";
	}
	return index;
}

Options parseArguments(int argc, char** argv) {
	Options options;
	for (size_t i = 1; i < argc; ++i) {
		if (*argv[i] == '-') {
			i = parseOption(argc, argv, i, options);
			continue;
		} else if (!options.inFile.empty()) {
			std::cout << "Warning: Multiple input files specified, overwriting first choice.\n";
		}
		options.inFile = argv[i];
	}
	return options;
}

int main(int argc, char** argv) {
	if (argc == 1) {
		std::cerr << "Error: Enter a path to a font configuration JSON file." << std::endl;
		return 1;
	}

	Options options = parseArguments(argc, argv);
	size_t fileSize;

    std::ofstream outStream = std::ofstream(options.outFile, std::ios_base::binary | std::ios_base::trunc);
    if(!outStream.is_open()) {
		std::cerr << "Error: " << options.outFile << ": Couldn't open file." << std::endl;
		return 2;
    }

	char* data = reinterpret_cast<char*>(readFile(options.inFile.c_str(), &fileSize));
	if (!data) {
		std::cerr << "Error: " << options.inFile << ": Couldn't open file." << std::endl;
		return 2;
	}

	Json::Value rootValue;

	Json::Reader reader;
	reader.parse(data, data + fileSize, rootValue);

    uint32_t tableSize;

	if(!rootValue["custom-font-paths"]) {}

    std::vector<std::string> customFontPaths;
    customFontPaths.reserve(rootValue["custom-font-paths"].size());
    for(auto& path : rootValue["custom-font-paths"]) {
        customFontPaths.push_back(path.asString());
		tableSize += 2 * sizeof(uint32_t); //table entry: offset + size
    }

    for(auto& name : rootValue["fonts"]) {

	}
}