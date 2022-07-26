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
#include <TestList.hpp>
#include <iostream>

int main(int argc, char** argv) {
	if (argc == 1) {
		std::cerr << "Enter a test name.
";
		return EXIT_FAILURE;
	}
	for (auto& test : testFunctions) {
		if (argv[1] == test.name) {
			test.function();
			return 0;
		}
	}
	std::cerr << "Test not found.
";
	return EXIT_FAILURE;
}
