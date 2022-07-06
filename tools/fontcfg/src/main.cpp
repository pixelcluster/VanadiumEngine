#include <ParsingUtils.hpp>
#include <fstream>
#include <util/WholeFileReader.hpp>
#include <iostream>
#include <json/json.h>
#include <vector>

struct Options {
	std::string outFile;
	std::string inFile;
};

struct Font {
	std::vector<std::string> fontNames;
};

bool checkOption(int argc, char** argv, size_t index, const std::string_view& argName) {
	if (static_cast<size_t>(argc) <= index + 1) {
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
	for (size_t i = 1; i < static_cast<size_t>(argc); ++i) {
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
	if (!outStream.is_open()) {
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

	uint32_t tableSize = 0;

	if (!(rootValue["custom-font-paths"].isArray() || rootValue["custom-font-paths"].isNull()) ||
		!(rootValue["fonts"].isArray() || rootValue["fonts"].isNull()) ||
		!(rootValue["fallback-path"].isString() || rootValue["fallback-path"].isNull())) {
		std::cerr << "Error: " << options.inFile << ": Invalid font configuration!\n";
	}

	// fallback path
	tableSize += 2 * sizeof(uint32_t);
	std::string fallbackPath = rootValue["fallback-path"].asString();

	std::vector<std::string> customFontPaths;
	customFontPaths.reserve(rootValue["custom-font-paths"].size());
	// font path count
	tableSize += sizeof(uint32_t);
	for (auto& path : rootValue["custom-font-paths"]) {
		if (!(path.isString() || path.isNull())) {
			std::cerr << "Error: " << options.inFile << ": Invalid font configuration!\n";
		}
		customFontPaths.push_back(path.asString());
		tableSize += 2 * sizeof(uint32_t); // table entry: offset + size
	}

	// font count
	tableSize += sizeof(uint32_t);
	std::vector<Font> fonts;
	for (auto& name : rootValue["fonts"]) {
		// name count
		tableSize += sizeof(uint32_t);

		if (!name.isObject() || !(name["primary-name"].isString() || name["primary-name"].isNull()) ||
			!(name["fallback-names"].isArray() || name["fallback-names"].isNull())) {
			std::cerr << "Error: " << options.inFile << ": Invalid font configuration!\n";
		}

		Font newFont;
		newFont.fontNames.push_back(name["primary-name"].asString());
		tableSize += 2 * sizeof(uint32_t);
		for (auto& fallbackName : name["fallback-names"]) {
			if (!fallbackName.isString()) {
				std::cerr << "Error: " << options.inFile << ": Invalid font configuration!\n";
			}
			newFont.fontNames.push_back(fallbackName.asString());
			tableSize += 2 * sizeof(uint32_t);
		}
		fonts.push_back(std::move(newFont));
	}

	uint32_t textWriteOffset = tableSize;
	outStream.write(reinterpret_cast<char*>(&textWriteOffset), sizeof(uint32_t));
	uint32_t fallbackPathLength = fallbackPath.size();
	outStream.write(reinterpret_cast<char*>(&fallbackPathLength), sizeof(uint32_t));
	textWriteOffset += fallbackPathLength;

	uint32_t customFontPathCount = customFontPaths.size();
	outStream.write(reinterpret_cast<char*>(&customFontPathCount), sizeof(uint32_t));
	for (auto& path : customFontPaths) {
		outStream.write(reinterpret_cast<char*>(&textWriteOffset), sizeof(uint32_t));
		uint32_t pathLength = path.size();
		outStream.write(reinterpret_cast<char*>(&pathLength), sizeof(uint32_t));
		textWriteOffset += pathLength;
	}

	uint32_t fontCount = fonts.size();
	outStream.write(reinterpret_cast<char*>(&fontCount), sizeof(uint32_t));
	for (auto& font : fonts) {
		uint32_t nameCount = font.fontNames.size();
		outStream.write(reinterpret_cast<char*>(&nameCount), sizeof(uint32_t));
		for (auto& name : font.fontNames) {
			outStream.write(reinterpret_cast<char*>(&textWriteOffset), sizeof(uint32_t));
			uint32_t nameLength = name.size();
			outStream.write(reinterpret_cast<char*>(&nameLength), sizeof(uint32_t));
			textWriteOffset += nameLength;
		}
	}

	outStream.write(fallbackPath.data(), fallbackPath.size());
	for (auto& path : customFontPaths) {
		outStream.write(path.data(), path.size());
	}
	for (auto& font : fonts) {
		for (auto& name : font.fontNames) {
			outStream.write(name.data(), name.size());
		}
	}
	outStream.close();
}