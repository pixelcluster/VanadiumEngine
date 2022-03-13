#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include "generate.hpp"
#include "parsing_utils.hpp"

struct Argument
{
	std::string name;
	std::string additionalData;
};

std::vector<Argument> parseArguments(int argc, char** argv) {
	Argument currentArgument;
	std::vector<Argument> arguments;
	bool hasWrittenArguments = false;

	for (int i = 1; i < argc; ++i) {
		hasWrittenArguments = true;
		bool isNewArgument = false;
		while (*argv[i] == '-')
		{
			++argv[i];
			isNewArgument = true;
		}

		if (isNewArgument) {
			arguments.push_back(currentArgument);
			currentArgument = Argument();
			currentArgument.name = argv[i];
		}
		else {
			if (!currentArgument.additionalData.empty()) currentArgument.additionalData += " ";
			currentArgument.additionalData += argv[i];
		}
	}
	if(hasWrittenArguments)
		arguments.push_back(currentArgument);
	return arguments;
}

int main(int argc, char** argv) {
	if (argc == 1) {
		std::cout << "Error: No input file specified!\n";
		return EXIT_FAILURE;
	}

	std::string xmlPath = argv[1];

	tinyxml2::XMLDocument vkXml;
	vkXml.LoadFile(xmlPath.c_str());
	if (vkXml.Error()) {
		std::cout << "Error: Error opening or parsing " << xmlPath <<  "! Does the file exist?\n";
		return EXIT_FAILURE;
	}

	std::ofstream cppFile = std::ofstream("EnumMatchTable.hpp", std::ios::trunc);
	if (!cppFile.is_open()) {
		std::cout << "Error: Error opening the output file! Is it in use?\n";
		return EXIT_FAILURE;
	}
	generateFromDocument(vkXml, cppFile);
}