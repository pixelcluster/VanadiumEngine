/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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
		std::cout << "Error: No input file specified!
";
		return EXIT_FAILURE;
	}

	std::string xmlPath = argv[1];

	tinyxml2::XMLDocument vkXml;
	vkXml.LoadFile(xmlPath.c_str());
	if (vkXml.Error()) {
		std::cout << "Error: Error opening or parsing " << xmlPath <<  "! Does the file exist?
";
		return EXIT_FAILURE;
	}

	std::ofstream cppFile = std::ofstream("EnumMatchTable.hpp", std::ios::trunc);
	if (!cppFile.is_open()) {
		std::cout << "Error: Error opening the output file! Is it in use?
";
		return EXIT_FAILURE;
	}
	generateFromDocument(vkXml, cppFile);
}
